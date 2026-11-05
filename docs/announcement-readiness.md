# Documentation audit: archive

Task: `GL-DOCSARCHIVE-01`. Audited public source:
`cd831e45dcfdbb4dcfa37ab147410e2248741912`. No tagged release existed at the
audit; main is experimental. The documentation changes leave implementation,
build files, compiler/dependency configuration and historical raw artifacts intact.

## Coverage

Reviewed every tracked Markdown file: README; API, implementation and validation
guides; security and third-party notices; contributing guide; benchmark guide;
and the three historical result READMEs. Also checked the maintained
`examples/inspect.goose`, the benchmark runner and its documented commands.
The security, license and contributor contracts remain in their dedicated files.

README now leads with purpose, a runnable fixture example, imports, default
limits and unsupported operations. Implementation detail and historical review/
measurement records are linked separately. API restrictions, ownership, failure
semantics, caller-controlled extraction parent and partial-output behavior remain
explicit. The benchmark guide shows complete-call scope and sample ranges;
historical tables/raw files remain available with clear historical labels.

## Example and command checks

A fresh anonymous HTTPS clone was checked out at the exact public source above.
Using an existing clean compiler at `GOOSE_REVISION`, this command passed:

```sh
make fixtures check test GOOSE=/absolute/path/to/pinned/goose
```

It ran all 19 native test groups, interpreted and compiled public API tests, and
the README file-reading example. Example output was `src 0` followed by
`src/main.goose 11`. Fixtures are generated locally; the example's unbounded file
read is explicitly limited to trusted local inputs. Application I/O limits are
still caller-owned. The pinned source was
`eadca409f68210893f366e60d0c2f0dc29a339be`; the compiler reported
`goose eadca40 (runtime 3025e5810379)`, with executable SHA-256
`3cad45a18356bf97f4732f1da2012252afd77a652932ddb9c91c5551c04904be`.

The setup script/Make recipes were inspected; a second compiler bootstrap,
sanitizer/fuzz campaign and measurement run were not needed for documentation.
The existing hosted CI still checks setup and the regular validation targets.
Remote import uses the audited full commit; the executed example used that
commit's anonymous clone and its local package import.

## Measurement currentness

The [source bridge](benchmark-source-bridge.json) compares the latest measured
implementation `c4d85defc66ff59227c986069971677e8e0d4abd` with the audited public
source: all listed implementation/build files, compiler pin, harness and fixture
generator are byte-identical. The clean initial record and final docs-dirty
record retain identical candidate/baseline benchmark binary hashes. This supports
reusing the candidate measurements, without claiming every later build or host
has the same timing.

The recorded compiler executable differs from the one used for this audit's
example checks; both have the same pinned source revision. Example execution is
not a new performance result. Measurements apply to the recorded GCC, OS,
libarchive/zlib versions and host, all listed in the [benchmark guide](../bench/README.md).
That guide retains all candidate workloads and min–max dispersion. Older
comparison ratios stay historical; the padding report does not independently
record the baseline compiler-binary identity. No new matched-binary comparative
claim or universal speedup is made. No timing rerun is required for the current
absolute inspection results because their implementation and harness match.

## Remaining limits

Linux only; printable ASCII paths; no ZIP, archive creation, streaming,
authenticity verification, atomic publication or crash durability. System
dependency internals are not sanitizer/coverage-instrumented by these harnesses.
Host sharing and frequency variation limit timing interpretation. Extraction is
not benchmarked. Tests are finite and do not establish absence of defects.
