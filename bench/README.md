# Inspection measurements

These measurements cover complete warmed `inspect` calls: gzip decoding, tar
parsing, path/conflict checks, manifest preparation, output clearing and owned
path copies. Fixture generation and compressed-file reads are outside timing.
They do not measure extraction, cold startup, allocation counts or another library.

## Source and environment

The latest retained candidate is `c4d85defc66ff59227c986069971677e8e0d4abd`.
Its implementation, build files, compiler pin, benchmark harness and fixture
generator match public main `cd831e45dcfdbb4dcfa37ab147410e2248741912` byte for
byte; see the [checked source bridge](../docs/benchmark-source-bridge.json).
The final record marked source dirty during documentation edits; the retained
clean initial record has the same candidate and baseline binary hashes.

The recorded machine is an Intel Core Ultra 7 258V, x86-64 Linux
7.2.11-arch1-1 with glibc 2.44. Dependencies were libarchive 3.8.11 and zlib
1.3.2. Goose revision `eadca409f68210893f366e60d0c2f0dc29a339be` selected GCC
16.2.1 (20260810); CMake was 4.4.5. Goose used `-O2`; its native CMake target
used Release (`-O3 -DNDEBUG`). Full compiler/tool/binary/fixture hashes are in the
[raw record](results/2026-11-04-padding/comparison.json).

Two warmup calls per workload and binary preceded nine seeded randomized paired
rounds of 200 calls per sample on CPU0. The host was shared, including fuzzing on
CPU5; pinning does not establish isolation or fixed frequency. Each sample is a
batch average. Below are candidate medians and full min–max ranges across the
nine samples, in **microseconds per call**; outliers are retained.

| Workload / limits | Median µs/call | Sample range µs/call |
| --- | ---: | ---: |
| small/tight | 26.39 | 25.77–98.52 |
| small/default | 26.60 | 25.55–52.10 |
| small/max | 48.90 | 47.93–98.26 |
| wide/default | 3181.44 | 3084.41–9839.26 |
| large/default | 5810.62 | 5642.18–11586.61 |
| padded/default | 6046.84 | 5730.63–11155.58 |

The tiny fixture has two entries; `tight/default/max` vary configured capacity.
`wide` has 1,000 files, `large` has a 4 MiB compressible payload, and `padded`
extends the tiny tar to 4 MiB with zeros. Checksums verify every measured result.
These values describe this code on the recorded host/dependencies, not promised
latency or a universal speedup.

## Reproduce

After `make setup`, run:

```sh
make bench BENCH_ITERATIONS=200
```

This builds `.build/bench`, generates fixtures and writes `.build/benchmark.json`
with seven rounds by default. To use nine rounds or a paired baseline:

```sh
python3 bench/measure.py --goose .toolchain/goose/build/goose \
  --iterations 200 --runs 9 --output .build/benchmark.json
python3 bench/measure.py --goose .toolchain/goose/build/goose \
  --baseline /absolute/path/to/baseline-bench --baseline-revision FULL_40_CHARACTER_SHA \
  --iterations 200 --runs 9 --output .build/comparison.json
```

Build the baseline with the same `bench/bench.goose`, exact Goose binary,
native compiler/flags and system dependency builds. Keep fixture hashes equal.
The harness selects the lowest CPU allowed by its affinity; constrain the parent
process affinity if a specific available CPU is needed. Run comparisons only in
a coordinated finite measurement window on a shared machine.

## Historical comparisons

[Initial inspection](results/2026-11-04/README.md),
[preparation changes](results/2026-11-04-audit/README.md) and
[padding scan](results/2026-11-04-padding/README.md) retain all raw samples and
before/after tables, including the slower large-payload preparation result.
The padding report records the candidate compiler binary but not an independent
baseline compiler-binary identity. Its ratios remain historical observations;
they are not presented here as a newly verified matched-binary speedup. The
absolute candidate results above have the explicit current-source bridge.
No raw artifact was replaced and no new timing campaign was run for this audit.
