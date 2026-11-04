# API contract

Declare the package import in your application's root source. Only `Error`,
`Kind`, `Entry`, `Status`, `Limits`, `inspect` and `extract` are supported API.
Helpers and the native slice ABI are private.

`inspect(input, limits, entries)` accepts gzip bytes and replaces `entries` with
owned `{kind, size, path}` records in archive order. `Kind` is File or Directory;
size is decoded file size (directories zero). Paths have no terminal slash.
On every reported error `entries` is empty. Destroying/changing the compressed
input after the call cannot change returned paths. Goose arrays of variable-size
elements must be iterated; direct random indexing is unavailable.

`extract(input, destination, limits)` validates all headers, paths, conflicts and
file data before filesystem mutation. It then decodes tar again from the same
immutable decompressed buffer and writes the files. Destination must be an
absolute, canonical-component path of 2..4095 bytes: no NUL, empty components,
`.`/`..`, or trailing slash. All ancestor directories must already exist and be
accessible; every ancestor is opened with no-follow descriptor traversal. Root
must not exist. An existing file, directory or symlink is never replaced.

Root and intermediate directories use mode0700. Files use0600, or0700 if the
archive has any execute bit. These are **maximum permissions**, reduced by umask;
restrictive umasks may make creation/traversal fail. Ownership is the extracting
process's; archive owners, dates, ACLs, xattrs and special mode bits are ignored.
Files use exclusive creation and no-follow flags. Every descriptor is close-on-exec.
A system error may leave a partial tree at destination; caller owns cleanup and
publication. No fsync or rollback guarantee. Validation failures create nothing.
The parent must be controlled by the caller; concurrent mutation by a process
with equivalent filesystem privileges is outside the contract. Inputs must stay
immutable during calls. No pointers or descriptors are retained after return.

## Limits

Compressed input must contain1..16MiB. Every configurable limit accepts zero and
has a hard maximum. Zero means no capacity, not unlimited.

| Limits field | Default | Maximum | Counts |
| --- | ---: | ---: | --- |
| tar_bytes |16MiB|64MiB| All decompressed bytes, including headers/extensions/padding |
| entries |4096|4096| Regular-file and directory entries delivered by libarchive |
| file_bytes |16MiB|64MiB| Declared and observed size of each regular file |
| total_bytes |16MiB|64MiB| Sum of regular-file sizes |
| depth |32|32| Path components |
| manifest_bytes |1MiB|4MiB| Sum of normalized path bytes +10 per entry, for both APIs |

Paths have at most1024 bytes, accept bytes32..126, and reject backslash, colon,
absolute prefixes, empty components and `.`/`..`. One directory trailing slash is
accepted. Duplicate decoded paths and regular files used as ancestors are rejected,
including out-of-order entries. Implicit directories are allowed.

Decompression allocates `tar_bytes +1` bytes without clearing them; the sentinel
permits exact cap-boundary detection. Validation allocates entry and sort tables
proportional to the configured entry cap (roughly4.3MiB at4096), without zeroing
the entire tables. Goose inspection prepares/zeroes `manifest_bytes` and copies
paths into owned entries. Peak storage also includes caller input, previous output
capacity and dependency allocations. These caps are not a total RSS or wall-clock
limit. Tar extensions are bounded by `tar_bytes`, not the public entry count.
Name sorting costs O(entries log entries) string comparisons; ancestor checks use
at most `depth` binary searches per entry. Decompression is performed once;
extraction adds a second tar traversal and file writes.

Symlinks, hardlinks, special files, sparse extents and discontinuous file data are
rejected. Degenerate GNU sparse representations exposing no extents can be
accepted only when their data decodes as an ordinary contiguous file; this is not
support for sparse filesystem output. Libarchive warnings are failures. Gzip CRC,
truncation, extra gzip members and compressed trailing bytes are checked. The
actual post-payload boundary must be followed by at least two zero blocks and
only zero tar padding; concatenated tar archives are rejected. Terminal extension
records after the last delivered entry may be rejected by this strict policy.

## Status

`Status.error` is None, InvalidInput (API argument/cap), LimitExceeded,
Malformed (decode/header/data/integrity/warning failure), UnsafeEntry (path/type/
conflict policy), or SystemError (allocation/filesystem failure).
`os_error` carries errno only for SystemError, otherwise zero. libarchive setup
failures are reported as Malformed; decoder allocation failures can surface as
Malformed inside libarchive. Exact error classification can depend on where the
parser rejects input (e.g. non-ASCII paths may be Malformed in the C locale).
No exceptions, global mutable library state, retained pointers or borrowed
output. Allocator failure within Goose follows the compiler runtime policy.
