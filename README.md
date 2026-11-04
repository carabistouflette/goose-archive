# goose-archive

Bounded `.tar.gz` inspection and extraction for [Goose](https://github.com/aardappel/goose).
Apache-2.0. Experimental API; Linux only, no release yet.

The API accepts compressed bytes, validates the complete archive, and optionally
extracts into a **fresh directory**. It rejects traversal paths, links, special
files, duplicate names, file/directory conflicts, and archives exceeding explicit
resource limits. Files are never overwritten. System zlib and libarchive handle
decoding through a private C adapter; no subprocesses or external filters run.

```goose
import archive from "../goose-archive";
import std;
fn main() {
    // Applications own bounded file/network I/O and authenticity verification.
    var compressed: u8[>..] = [];
    var entries: archive::Entry[>..] = [];
    let limits = archive::Limits { tar_bytes: 16777216, entries: 4096 };
    let result = archive::inspect(compressed, limits, entries);
    if result.error != archive::Error.None { print(result.error); return; }
    for entry in entries { print(entry.path, " ", entry.size); }
    // archive::extract(compressed, "/absolute/new-directory", limits)
}
```

Use a sibling local checkout with the import above. Publication and remote import
pins are pending; this repository does not claim an available public remote.
A runnable file-reading example is [examples/inspect.goose](examples/inspect.goose).

## Build and validate

Requirements: Linux, Git, Make, Python 3.11+, CMake 3.27+, C/C++20 compilers,
libarchive >=3.6 development files, zlib >=1.2.11 development files. Clang is
required for fuzzing. Use maintained system dependency packages with security
fixes; minimum build versions are not a security guarantee. Debian/Ubuntu:
`build-essential cmake git python3 pkg-config libarchive-dev zlib1g-dev clang`.

From a standalone clone or copy of this directory:

```sh
make setup
make format check test
make sanitize SANITIZER_CC=gcc
make sanitize SANITIZER_CC=clang
make fuzz FUZZ_RUNS=10000
make bench BENCH_ITERATIONS=200
```

An existing compiler at exactly `GOOSE_REVISION` can be supplied using
`GOOSE=/absolute/path/to/goose`. Run Make serially per checkout because the
compiler shares native cache paths. `make format` refreshes the native source
fingerprint. CI pins the compiler and GitHub Actions revisions.

## Scope and limits

See [the API contract](docs/api.md), [validation](docs/validation.md), and
[benchmark methodology](bench/README.md). Input is exactly one gzip member
containing tar with two terminal zero blocks and zero trailing padding. USTAR,
GNU and PAX decoding is delegated to libarchive with strict warning rejection.
Paths currently allow printable ASCII only; Unicode filenames are rejected,
including otherwise valid Honk packages containing them. Paths are normalized
only by removing one directory trailing slash. Effective decoded paths are checked;
this is not a validator of every raw extension field.

No archive creation, ZIP, streaming I/O, authenticity/signature verification,
atomic publication, Windows support, or crash durability in this version.
Extraction requires a caller-controlled parent directory. A filesystem failure
can leave a private partial tree. Validate and authorize artifacts before use.
