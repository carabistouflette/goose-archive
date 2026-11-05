# goose-archive

Inspect `.tar.gz` bytes or extract them into a new directory from [Goose](https://github.com/aardappel/goose).
Linux only, Apache-2.0, experimental API. There are no tagged releases yet;
these docs describe main. Use an exact commit for dependencies.

The library validates the whole archive before extraction. It rejects unsafe
paths, links, special files, duplicate names, conflicting paths and configured
limit violations. Existing files are never overwritten. Gzip and tar decoding
use system zlib and libarchive through a private C adapter.

## Try it

Install Linux build tools, Python 3.11+, CMake 3.27+, C/C++20 compilers,
libarchive >=3.6 and zlib >=1.2.11 development packages. On Debian/Ubuntu the
packages are `build-essential cmake git python3 pkg-config libarchive-dev zlib1g-dev`.
Use maintained dependency packages with security fixes; minimum versions only
specify build requirements.

```sh
git clone https://github.com/carabistouflette/goose-archive.git
cd goose-archive
make setup
make fixtures
.toolchain/goose/build/goose run examples/inspect.goose -- .build/fixtures/small.tar.gz
```

This prints the paths and sizes in a generated local fixture. The
[example](examples/inspect.goose) reads a trusted local file; applications must
bound their own file or network reads before calling the library.
`make setup` builds the exact compiler in `GOOSE_REVISION`. To reuse that compiler,
supply `GOOSE=/absolute/path/to/goose` to Make and use that path for `goose run`.

## API

Declare the dependency in the application's root source:

```goose
import archive from "carabistouflette/goose-archive@cd831e45dcfdbb4dcfa37ab147410e2248741912";
```

Or use `import archive from "../goose-archive";` for a sibling checkout.

- `inspect(input, limits, entries)` replaces `entries` with owned path, size and
  kind records. An error clears the output.
- `extract(input, destination, limits)` validates the same bytes, then writes a
  fresh absolute destination directory. Its parent must be caller-controlled.

Both return `Status`, with `Error.None` on success. Compressed input has a fixed
16 MiB cap. `Limits { .. }` defaults to 16 MiB decompressed tar, 4,096 entries,
16 MiB per file and total payload, depth 32, and 1 MiB packed manifest. Configurable caps and
exact errors are in the [API reference](docs/api.md).

Paths support printable ASCII only. Unicode filenames, including otherwise
valid Honk packages containing them, are rejected. Input must be exactly one
gzip member containing tar with two terminal zero blocks and zero trailing
padding. Policy checks use libarchive's effective decoded paths and types.

Extraction can leave a private partial tree after a filesystem failure. The
caller owns cleanup, authorization and publication. There is no archive creation,
ZIP, streaming I/O, signature verification, atomic publication, crash durability
or Windows support. CRC checks detect corruption; they do not establish trust.

## Development and measurements

Run `make check test`. Native behavior changes also need GCC/Clang
`make sanitize SANITIZER_CC=...` and bounded `make fuzz FUZZ_RUNS=10000`
(Clang required). Run Make serially per checkout because native cache paths are
shared. `make format` refreshes the native source fingerprint.

See [validation](docs/validation.md), [implementation notes](docs/design.md),
[inspection measurements](bench/README.md) and [security reporting](SECURITY.md).
Measurements cover warmed inspection calls, not extraction or a cross-library
ranking. The [documentation audit](docs/announcement-readiness.md) records the
checked source bridge and example commands.
