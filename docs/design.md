# Implementation notes

The [API reference](api.md) defines caller-visible behavior. These notes explain
the private implementation; they are not additional public entry points.

Zlib decodes exactly one gzip member into a buffer bounded by `tar_bytes + 1`.
The extra byte detects an exceeded cap. Libarchive reads uncompressed tar from
that buffer, without subprocesses or external filters. CRC, terminal zero blocks,
trailing data and libarchive warnings are checked.

Inspection prepares a packed manifest, then copies paths into owned Goose
entries. The gzip trailer supplies only a bounded preparation hint: native
validation still checks actual sizes and available capacity. A sorted pointer
index detects duplicates and binary-searches path ancestors.

Extraction first validates every entry and payload. A second tar traversal over
the same immutable buffer creates a new directory and writes files in chunks of
at most 64 KiB. Descriptor-relative traversal uses no-follow and close-on-exec
flags; files use exclusive creation. No archive owners, timestamps, ACLs or
special mode bits are applied.

Filesystem failures can leave a partial tree. The library does not attempt
recursive deletion, fsync or atomic publication. The caller controls the parent,
keeps input immutable during each call and handles cleanup. Processes with equal
filesystem privileges can still mutate the tree; this is outside the contract.

Tests and their instrumentation limits are described in [validation](validation.md).
