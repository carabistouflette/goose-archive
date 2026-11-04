# Inspection benchmarks

`make bench` builds the public Goose harness with `-O2` and runs seven samples
per workload in seeded random order, pinned to one allowed CPU. The pinned Goose compiler configures native CMake targets in Release mode
(`-O3 -DNDEBUG` with the measured GCC toolchain). The `-O2` argument is Goose's
optimization level; it is not a native C flag override.
Fixture generation and compressed-file reads occur outside timing. Each measured
call includes gzip decoding, tar parsing, path/conflict validation, manifest
preparation and ownership copies. Each call's old output clearing is included.
Checksums prevent dead results. Two warmup calls precede sampling.

Workloads: tiny two-entry archive with tight/default/max limits;1000 files;
4MiB compressible payload. The cap variants expose preparation/allocation costs.
Results are ns/call with raw samples, medians, CPU/platform, dependency/compiler
versions, the selected Goose toolchain, actual Goose version/binary hash and Git provenance in `.build/benchmark.json`. No competing library
comparison, allocation count, extraction throughput, cold latency, wall-clock
limit, or universal throughput claim. Shared-machine load and CPU frequency can
affect results. Preserve raw results before making performance claims.

A recorded clean-source run is available in
[2026-11-04 results](results/2026-11-04/README.md).

For a before/after comparison, build both revisions with the same
`bench/bench.goose` and pinned compiler, then run:

```sh
python3 bench/measure.py --goose /path/to/pinned/goose \
  --baseline /path/to/baseline-bench --baseline-revision FULL_40_CHARACTER_SHA \
  --iterations 200 --runs 7 --output .build/comparison.json
```

The candidate is `.build/bench`. Each workload is measured as paired calls in
seeded randomized binary order, with expected checksums verified for both.
Raw samples for each binary, full baseline revision, fixture/harness/binary
hashes and the selected toolchain are retained. These are warmed public API
inspection costs, not kernel-only timings. No filesystem extraction is timed.

The [paired performance/security audit](results/2026-11-04-audit/README.md)
records a fresh baseline comparison and strengthened validation.
