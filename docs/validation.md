# Validation

Validated locally on Linux x86-64 with the exact compiler in GOOSE_REVISION.
The public [CI workflow](https://github.com/carabistouflette/goose-archive/actions/workflows/ci.yml)
shows the current remote validation status; the results below describe local runs.

Commands:

```sh
make format check test GOOSE=/absolute/path/to/pinned/goose
make sanitize SANITIZER_CC=gcc
make sanitize SANITIZER_CC=clang
make fuzz FUZZ_RUNS=10000
make bench GOOSE=/absolute/path/to/pinned/goose BENCH_ITERATIONS=200
```

The Python suite has 18 test groups. Its independent `tarfile` producer covers
USTAR, GNU and PAX;80 seeded generated documents compare exact returned paths,
sizes and kinds. Further cases cover long paths/PAX overrides, corruption and
truncation at every gzip prefix, duplicate names, out-of-order file ancestors,
unsafe paths/types, sparse extents/all-hole forms, exact and below limits,
cumulative payload limits, decompression bombs, terminal zero blocks, hidden
concatenated archives, native aliasing/invalid arguments, file bytes/modes,
existing destinations and symlink parents. Filesystem checks use private temporary
directories and prove validation failures leave no destination.

Goose public tests run through `goose run` and a built executable. They check
owned metadata after overwriting compressed input, output clearing, malformed
input, limits, extraction/repeated destination failure, binary file contents,
and both independently discovered bug regressions. The runner additionally
asserts those rejected archives created no destination. A standalone local Git
clone passes check/test using the same pinned external compiler, without files
from the coordination workspace.

GCC and Clang ASan/UBSan runs execute the same 18 native test groups through an
instrumented subprocess driver, plus direct invalid/alias ABI checks. Leak
detection is enabled; driver errors print sanitizer diagnostics. These runs
instrument this adapter, not the installed system zlib/libarchive libraries or
the Goose compiler runtime. Ordinary Goose calls exercise real FFI integration;
they are not themselves a whole-program sanitizer build.

The libFuzzer target checks compressed bytes directly and also wraps mutations
as valid gzip so they can reach tar parsing without repairing gzip checksums.
It asserts output record boundaries and error metadata. Default campaign size10000,
seed219, input cap1MiB; expanded tar cap2MiB and128 entries in the harness.
Generated compressed and raw tar fixtures seed it. A persistent corpus means
seed alone does not reproduce every mutation sequence. Fuzzing exercises inspection,
not filesystem extraction; system dependencies are not coverage-instrumented.
This finite campaign and these tests are not proof of exhaustive format/security
compliance, allocation-failure recovery or hostile same-UID race protection.

Three independent reviewers participated across design, implementation and final
integration. They identified an ancestor-index mutation and a missing tar EOF
boundary check; both were corrected with public and native regression tests.
Final security and quality reviews found no remaining implementation blocker;
validation follow-up confirmed all requested test improvements were addressed.

## Additional security and performance audit

Independent adversarial probes exercised every raw tar type byte, long GNU/PAX
extension chains, large PAX xattr/ACL sets, and extraction syscall failures. No
new security bypass or production correctness defect was found in those probes.
These finite probes are not an assurance that all hostile archives are safe.

Permanent tests now cover interrupted/short/zero writes, ENOSPC, file-close
failure, open/mkdir/descriptor-duplication failures, exact error metadata and
stopping before later entries. Repeated native fault calls check descriptor
stability; the instrumented driver also checks its own descriptor count on every
call. Link-time fault wrappers are test-only and absent from the production
CMake target.

Inspection preparation uses a bounded gzip trailer hint, without trusting it
for validation or writes. Regressions check forged ISIZE values, exact and
undersized native buffers, unchanged output canaries on failure, and the Goose
error/ownership contract. The fuzz target applies the same advisory capacity
bound with a tail canary. It also repairs the first tar header checksum in a
separate mutation path so changed paths/types/sizes reach parser policy checks.

Dependency security remains the responsibility of maintained installed packages:
track [libarchive's security advisories](https://github.com/libarchive/libarchive/security/advisories)
and [zlib releases](https://github.com/madler/zlib/releases), including distribution
backports. Build minimums are not declarations of vulnerability-free versions.

The [audit record](../bench/results/2026-11-04-audit/README.md) preserves the
clean-source paired measurements and extended fuzz campaign results.
