#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Measure complete public inspection, optionally in randomized baseline pairs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import statistics
import subprocess

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--iterations', type=int, default=200)
parser.add_argument('--runs', type=int, default=7)
parser.add_argument('--goose', type=Path, required=True)
parser.add_argument('--baseline', type=Path)
parser.add_argument('--baseline-revision')
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if not 1 <= args.iterations <= 100000 or not 1 <= args.runs <= 100:
    parser.error('invalid iterations or runs')
if args.baseline and not re.fullmatch('[0-9a-f]{40}', args.baseline_revision or ''):
    parser.error('--baseline requires --baseline-revision with a full commit SHA')
if args.baseline_revision and not args.baseline:
    parser.error('--baseline-revision requires --baseline')

def command(*parts):
    return subprocess.check_output(parts, cwd=ROOT, text=True, timeout=120).strip()

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

cpu = min(os.sched_getaffinity(0))
os.sched_setaffinity(0, {cpu})
cases = [('small', 'tight'), ('small', 'default'), ('small', 'max'), ('wide', 'default'), ('large', 'default')]
expected = {'small': 30, 'wide': 15000, 'large': 4194309}
binaries = {'candidate': ROOT / '.build/bench'}
if args.baseline:
    binaries['baseline'] = args.baseline.resolve()
results = {impl: {f'{name}/{limits}': [] for name, limits in cases} for impl in binaries}
checksums = {}
for binary in binaries.values():
    for name, limits in cases:
        command(str(binary), f'.build/fixtures/{name}.tar.gz', '2', limits)
rng = random.Random(219)
for _ in range(args.runs):
    rng.shuffle(cases)
    for name, limits in cases:
        order = list(binaries)
        rng.shuffle(order)
        key = f'{name}/{limits}'
        for impl in order:
            elapsed, checksum = map(int, command(str(binaries[impl]), f'.build/fixtures/{name}.tar.gz', str(args.iterations), limits).split())
            assert elapsed > 0 and checksum == expected[name] * args.iterations
            checksums[key] = checksum
            results[impl][key].append(elapsed / args.iterations)
# Record the compiler Goose actually chose, not the unrelated shell CC variable.
toolchain = json.loads(command(str(args.goose.resolve()), 'toolchain', '--json'))
toolchain.pop('home', None)
report = {
    'source': command('git', 'rev-parse', 'HEAD'),
    'source_dirty': bool(command('git', 'status', '--porcelain')),
    'compiler_revision': (ROOT / 'GOOSE_REVISION').read_text().strip(),
    'toolchain': toolchain,
    'goose_version': command(str(args.goose.resolve()), 'version'),
    'goose_binary_sha256': digest(args.goose.resolve()),
    'platform': platform.platform(), 'machine': platform.machine(), 'cpu_affinity': cpu,
    'cpu_model': next((x.split(':', 1)[1].strip() for x in Path('/proc/cpuinfo').read_text().splitlines() if x.startswith('model name')), 'unknown'),
    'dependencies': {'libarchive': command('pkg-config', '--modversion', 'libarchive'), 'zlib': command('pkg-config', '--modversion', 'zlib')},
    'build': 'goose build bench/bench.goose -O2; native CMake Release (-O3 -DNDEBUG with GCC)',
    'iterations': args.iterations, 'runs': args.runs, 'units': 'ns per inspect call',
    'samples': results['candidate'],
    'medians': {k: statistics.median(v) for k, v in results['candidate'].items()},
    'checksums': checksums,
    'binary_sha256': {k: digest(v) for k, v in binaries.items()},
    'fixture_sha256': {name: digest(ROOT / f'.build/fixtures/{name}.tar.gz') for name in expected},
    'harness_sha256': digest(ROOT / 'bench/bench.goose'),
}
if args.baseline:
    report['baseline_revision'] = args.baseline_revision
    report['baseline_samples'] = results['baseline']
    report['baseline_medians'] = {k: statistics.median(v) for k, v in results['baseline'].items()}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report['medians'], indent=2))
