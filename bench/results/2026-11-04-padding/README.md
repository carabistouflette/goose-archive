# Bounded padding scan audit

Historical results for the commits named below. See the [current measurement guide](../../README.md) for source currentness, dispersion and comparison limits.
The raw report records one Goose binary identity, not a separate baseline compiler-binary identity; comparative ratios have that provenance limitation.

Measured candidate `c4d85defc66ff59227c986069971677e8e0d4abd` against
`8444042bc822af273011fc4387546e796009eb91` with the same public Goose harness,
pinned compiler, generated fixtures and native Release flags. The baseline binary
was freshly built before changing native code. The source-dirty flag in the final
run reflects only the in-progress benchmark/validation documentation edits.

The profile of 500 baseline padded calls placed 26.77% of sampled cycles in
`ga_call` (which includes inlined wrapper work), and 44.34% in zlib CRC32.
This is a workload-specific profile, not a general cost breakdown.

Nine randomized paired rounds, 200 calls per sample, on CPU0. Fuzzing was pinned
to CPU5 for the final run. Other projects were active on this shared host; no
exclusive-core or stable-frequency claim. The initial run overlapped our own
sanitizers/fuzzing and is retained separately, without treating it as isolated.

| Workload | Baseline ns/call | Candidate ns/call | Baseline / candidate |
| --- | ---: | ---: | ---: |
| small/tight | 32198 | 26389 | 1.220× |
| small/default | 31688 | 26604 | 1.191× |
| small/max | 56083 | 48905 | 1.147× |
| wide/default | 3294720 | 3181437 | 1.036× |
| large/default | 5885209 | 5810624 | 1.013× |
| padded/default | 8225814 | 6046839 | 1.360× |

All results are warmed complete inspection calls, including gzip/tar decoding,
validation and owned manifest output. The padded case contains the small archive
extended with zeros to 4MiB. All original workloads remain reported. Raw paired
samples, checksums, dependency versions and fixture/harness/binary/compiler
hashes are in [comparison.json](comparison.json). No extraction throughput,
allocation count, cross-library ranking or universal speed claim is made.

Validation: all 19 native groups passed normally and with GCC/Clang ASan/UBSan
(leak detection enabled); format/check, both Goose public API execution modes,
example execution and fresh standalone-clone checks passed. Two independent
reviewers approved the source and boundary regressions without findings.
A 100,000-input local libFuzzer campaign completed without a finding (seed219,
1MiB mutation cap, persistent compressed/raw corpus; system libraries are not
coverage-instrumented). Its [log](fuzz.log) is retained. These finite checks do not
prove absence of defects.

[Hosted CI passed for the production commit](https://github.com/carabistouflette/goose-archive/actions/runs/41830872169),
including public checks/tests, GCC/Clang sanitizers, a fresh 10,000-input fuzz
campaign and benchmark smoke. Only evidence/documentation changed afterward.
Libarchive internals were outside the scope of this audit.
