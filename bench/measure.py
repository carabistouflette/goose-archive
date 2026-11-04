#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Measure complete public inspection, with input I/O outside the timed loop."""
import argparse
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--iterations', type=int, default=200)
parser.add_argument('--runs', type=int, default=7)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if not 1 <= args.iterations <= 100000 or not 1 <= args.runs <= 100:
    parser.error('invalid iterations or runs')

def command(*parts):
    return subprocess.check_output(parts, cwd=ROOT, text=True).strip()

cpu = min(os.sched_getaffinity(0))
os.sched_setaffinity(0, {cpu})
cases = [('small', 'tight'), ('small', 'default'), ('small', 'max'), ('wide', 'default'), ('large', 'default')]
results = {f'{name}/{limits}': [] for name, limits in cases}
checksums = {}
for name, limits in cases:
    command(str(ROOT / '.build/bench'), f'.build/fixtures/{name}.tar.gz', '2', limits)
rng = random.Random(219)
for _ in range(args.runs):
    rng.shuffle(cases)
    for name, limits in cases:
        elapsed, checksum = map(int, command(str(ROOT / '.build/bench'), f'.build/fixtures/{name}.tar.gz', str(args.iterations), limits).split())
        key = f'{name}/{limits}'
        assert checksum > 0 and checksums.get(key, checksum) == checksum
        checksums[key] = checksum
        results[key].append(elapsed / args.iterations)
report = {
    'source': command('git', 'rev-parse', 'HEAD'),
    'source_dirty': bool(command('git', 'status', '--porcelain')),
    'compiler_revision': (ROOT / 'GOOSE_REVISION').read_text().strip(),
    'platform': platform.platform(), 'machine': platform.machine(), 'cpu_affinity': cpu,
    'cpu_model': next((x.split(':', 1)[1].strip() for x in Path('/proc/cpuinfo').read_text().splitlines() if x.startswith('model name')), 'unknown'),
    'cc': command('cc', '--version').splitlines()[0],
    'dependencies': {'libarchive': command('pkg-config', '--modversion', 'libarchive'), 'zlib': command('pkg-config', '--modversion', 'zlib')},
    'build': 'goose build bench/bench.goose -O2; private C target uses compiler CMake defaults',
    'iterations': args.iterations, 'runs': args.runs, 'units': 'ns per inspect call',
    'samples': results, 'medians': {k: statistics.median(v) for k, v in results.items()},
    'checksums': checksums,
}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report['medians'], indent=2))
