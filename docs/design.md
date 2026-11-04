# Archive contract and implementation plan

Experimental Linux library, Apache-2.0. API: inspect gzip-compressed tar bytes into
owned entries; extract those same bytes into a fresh absolute destination directory.
The caller owns reading/downloading and authorizing input. No authenticity claims.

Use system zlib to decode exactly one gzip member (CRC/truncation checked; no
trailing compressed bytes). Allocate at most the configured decompressed tar cap
plus one sentinel byte to distinguish an exact boundary. System libarchive reads
only uncompressed tar from that buffer, with no external programs or filters.
Require two terminal zero blocks and zero trailing tar padding. Strictly reject
parser warnings, unsupported types, symlinks/hardlinks, sparse entries, absolute
paths, dot/dot-dot components, backslashes, colons, control/non-ASCII path bytes,
duplicates, and file/directory conflicts. Allow a single trailing slash for dirs.
GNU/PAX tar decoding is delegated to libarchive, not reimplemented here.

Limits: compressed input <=16 MiB; decompressed tar <=64 MiB; entries <=4096;
path <=1024 bytes; depth <=32; total file bytes <=64 MiB; file <=64 MiB;
packed manifest <=4 MiB. User limits may lower these. Fixed 64 KiB copy buffer.
Limits bound library work/storage, not strict wall-clock time or total RSS inside
system dependencies. Header metadata is covered by decompressed tar cap.

Validate all entries and data before any filesystem mutation. Retain validated
file offsets in immutable decompressed storage. Sort a pointer index to detect
duplicates and binary-search ancestors to reject files used as directories.
Extraction traverses destination ancestors with directory descriptors and
O_NOFOLLOW|O_CLOEXEC. Exclusively create root mode 0700; implicit directories 0700;
regular files O_EXCL|O_NOFOLLOW, 0600 (0700 if any archive execute bit).
No archive ownership, timestamps, special permission bits, ACLs or xattrs applied.
Never overwrite existing destination. Extraction failures may leave a partial
private tree: no unsafe automatic recursive deletion. No atomic publication or
crash durability. Caller must control destination parent and not concurrently
mutate input or output tree. Hostile same-UID mutation is outside the contract.

Native private slice ABI checks lengths/pointers/options. Goose returns owned
paths and clears inspection output on errors. Status includes OS errno only for
system failures. No global mutable state. Independent reviews at design, native
implementation, and final integration; adversarial public/native tests, GCC and
Clang sanitizers, fuzzing and reproducible benchmarks before completion.
