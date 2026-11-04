# Inspection measurement, 2026-11-04

[Raw samples and provenance](inspection.json) were measured on clean commit
06cf713ad6b80366d312fe5b56697970e92b0030 using200 iterations and7 samples per
workload, with the exact pinned Goose compiler. This evidence-only commit does
not change the measured implementation. Reproduce with:

```sh
make bench GOOSE=/path/to/pinned/goose BENCH_ITERATIONS=200
```

The report includes all sampled ns/call values, medians, deterministic checksums,
CPU affinity/model, platform, dependency versions and compiler/build flags.
See ../../README.md for workload scope and limitations. No cross-library or
machine-independent performance claim is made.

At this implementation, all14 native test groups pass normally and with both
GCC and Clang ASan/UBSan including leak checks. Public Goose tests pass via run
and build, and a fresh local Git clone passes check/test with the same external
pinned compiler. Two10,000-input libFuzzer campaigns completed without a finding;
the final campaign used the expanded corpus and completed10000 inputs in32s.
System zlib/libarchive were not rebuilt with instrumentation. CI is configured
but not executed remotely. See ../../../docs/validation.md for precise scope.
