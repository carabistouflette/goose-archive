# Inspection benchmarks

`make bench` builds the public Goose harness with `-O2` and runs seven samples
per workload in seeded random order, pinned to one allowed CPU. The private C
CMake target uses compiler defaults; no claim that its kernel uses `-O2` is made.
Fixture generation and compressed-file reads occur outside timing. Each measured
call includes gzip decoding, tar parsing, path/conflict validation, manifest
preparation and ownership copies. Each call's old output clearing is included.
Checksums prevent dead results. Two warmup calls precede sampling.

Workloads: tiny two-entry archive with tight/default/max limits;1000 files;
4MiB compressible payload. The cap variants expose preparation/allocation costs.
Results are ns/call with raw samples, medians, CPU/platform, dependency/compiler
versions and Git provenance in `.build/benchmark.json`. No competing library
comparison, allocation count, extraction throughput, cold latency, wall-clock
limit, or universal throughput claim. Shared-machine load and CPU frequency can
affect results. Preserve raw results before making performance claims.

A recorded clean-source run is available in
[2026-11-04 results](results/2026-11-04/README.md).
