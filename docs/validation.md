# Validation

The repository pins its Goose compiler in `GOOSE_REVISION`. Current remote
results are in the [CI workflow](https://github.com/carabistouflette/goose-archive/actions/workflows/ci.yml).
The following commands use that compiler; they are not performance thresholds:

```sh
make setup
make check test
make sanitize SANITIZER_CC=gcc
make sanitize SANITIZER_CC=clang
make fuzz FUZZ_RUNS=10000
```

Use `GOOSE=/absolute/path/to/goose` to reuse the exact pinned compiler. Clang is
required for fuzzing. Run targets serially in one checkout.

## What the tests cover

The 19 Python test groups use independent `tarfile` fixtures for USTAR, GNU and
PAX, including 80 seeded generated archives. They check paths, kinds and sizes;
unsafe paths/types, duplicates and ancestor conflicts; gzip prefixes, corruption
and extra members; tar boundaries, hidden content, sparse representations and
padding scan edges; exact/overflow limits; ownership and invalid native calls.
Extraction checks compare contents and permissions, reject existing destinations
and symlink parents, and verify that validation errors create nothing.

Test-only syscall wrappers exercise interrupted/short/zero writes, ENOSPC,
close/open/mkdir/descriptor-duplication failures and descriptor cleanup. They are
not part of the production CMake target. Goose tests run interpreted and compiled
public calls, including ownership after changing the compressed input and output
clearing on errors. The file-reading example is also executed.

## Sanitizer and fuzz scope

GCC/Clang ASan/UBSan runs instrument the native adapter and its subprocess driver,
including invalid/overlapping ABI calls and filesystem fault tests. They do not
instrument the installed system zlib/libarchive libraries or the Goose compiler
runtime. Ordinary Goose tests separately exercise real FFI calls.

The libFuzzer target checks inspection, not filesystem extraction. It tests
compressed input and gzip-wrapped raw tar mutations, with checksum repair and
output canaries. Defaults are 10,000 inputs, seed 219, a 1 MiB input cap, a 2 MiB
tar cap and 128 entries. The persistent corpus affects mutation order, so a seed
alone does not reproduce a campaign. System dependencies are not coverage-
instrumented. Finite tests do not prove absence of defects, allocation-failure
recovery or protection against hostile same-UID filesystem mutation.

## Recorded results

Historical [initial validation](../bench/results/2026-11-04/README.md),
[allocation-preparation audit](../bench/results/2026-11-04-audit/README.md) and
[padding-scan validation](../bench/results/2026-11-04-padding/README.md) retain
source identities, raw measurements, review outcomes and fuzz limits. Their
results apply to the named implementations, not every later build.
The [announcement-readiness audit](announcement-readiness.md) checks which
measured source still matches current main and records the example checks.

Use maintained system dependencies. Track [libarchive advisories](https://github.com/libarchive/libarchive/security/advisories)
and [zlib releases](https://github.com/madler/zlib/releases), including distribution
backports. Minimum build versions do not imply vulnerability-free packages.
