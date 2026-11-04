# Contributing

Keep the public API small, explicit and bounded. Applications own file/network
I/O and trust decisions. Native code stays private; use maintained system
libraries rather than implementing compression or archive formats here. Preserve
validation, owned outputs and prevalidation before filesystem mutation.

Use focused branches (`feat/...`, `fix/...`, `docs/...`) and Conventional Commits:
`feat: add ...`, `fix: reject ...`, `test: cover ...`, `docs: clarify ...`,
`chore: update ...`. Use imperative subjects and one coherent change per commit.
No stability guarantee before1.0. Release only after review.

Run `make format check test`, `git diff --check`, GCC and Clang `make sanitize`,
and bounded `make fuzz` for native behavior changes. Add public-API regressions
for malformed input, ownership, resource bounds and every confirmed defect.
Request independent security/correctness and quality/performance reviews, resolve
actionable findings, and keep CI passing. Never commit compiler output,
credentials or private workspace notes. Compiler changes require a reviewed full
upstream commit in `GOOSE_REVISION` and complete validation.

Profile before optimizing. Benchmark complete public calls as well as relevant
kernels; preserve all checks. Keep raw samples, source revision, flags, dependency
versions and platform provenance. Report regressions and limits. CI checks
benchmark execution, not timing thresholds. No universal speed rankings.

Report vulnerabilities privately using the guidance in SECURITY.md.
