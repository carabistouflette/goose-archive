# Paired inspection audit, 2026-11-04

[Raw paired samples and provenance](comparison.json) compare clean candidate
463a18cb4f6b37d940810216e8a346cd0cbcbf32 against a freshly rebuilt public baseline
fa0629139705edb5ac5dac4e077536b8ba43c16e. Both use the identical Goose harness,
compiler binary and native Release configuration. The report records these
identities, fixture/binary hashes, dependencies and selected compiler toolchain.
This evidence-only commit does not change the measured implementation.

Nine randomized paired samples of 200 iterations ran on CPU0. The shared host
had other activity; pinning does not establish exclusive isolation.
Input reading is outside timing; complete inspection and owned output are timed.

| Workload / limits | Baseline median µs | Candidate median µs | Baseline / candidate |
| --- | ---: | ---: | ---: |
| Tiny / tight | 29.74 | 29.53 | 1.01 |
| Tiny / default | 77.93 | 30.46 | 2.56 |
| Tiny / maximum | 372.15 | 52.66 | 7.07 |
| 1000 files / default | 3006.27 | 3006.31 | 1.00 |
| 4 MiB payload / default | 5363.42 | 5417.06 | 0.99 |

The large-payload median is about 1% higher in this run; this is not a universal
speedup claim. Raw sample variation and shared-host scheduling/frequency effects
limit interpretation. See ../../README.md for the comparison command and scope.

The improvement bounds preparatory manifest clearing using the advisory gzip
trailer, the maximum per-record size and the entry cap. It does not skip gzip
integrity validation, entry validation or native actual-output-capacity checks.
The larger workloads already reach the configured preparation cap.

At this production code, all 18 test groups pass normally and through GCC and
Clang ASan/UBSan with leak checks, including test-only injected I/O failures.
A fresh independent Git clone passes check/test. Three independent reviewers
examined security, performance and extraction fault handling; no new production
bug or bypass was verified, and all review suggestions were addressed.

A 100,000-input checksum-repair/capacity-canary fuzz campaign completed in419s
without a finding. After tightening the success metadata assertion, the final
harness passed a further1,000-input smoke campaign. Installed system zlib and
libarchive are not coverage/sanitizer-instrumented by this harness. These finite
checks do not prove absence of bugs; see ../../../docs/validation.md.
