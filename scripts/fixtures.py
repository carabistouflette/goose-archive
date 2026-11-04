#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Deterministic independent tar fixtures; no downloads or system tar required."""
import gzip
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'tests'))
from test_archive import archive
root = Path(__file__).resolve().parent.parent / '.build/fixtures'
root.mkdir(parents=True, exist_ok=True)
samples = {
    'small': [('src/', b'', 'd'), ('src/main.goose', b'hello\0world', 'f')],
    'wide': [(f'file-{i:04}', b'hello', 'f') for i in range(1000)],
    'large': [('data', bytes(range(256)) * 16384, 'f')],
    'unsafe': [('first', b'a', 'f'), ('../escape', b'', 'f')],
    'conflict': [('a', b'', 'f'), ('a/b', b'', 'd')],
}
for name, entries in samples.items():
    (root / (name + '.tar.gz')).write_bytes(archive(entries))

zero_raw = gzip.decompress(archive([('zero', b'\0' * 1024, 'f')]))
(root / 'no-eof.tar.gz').write_bytes(gzip.compress(zero_raw[:1536], mtime=0))

# Preserve raw inputs for fuzzing the gzip and tar boundaries.
corpus = root.parent / 'fuzz-corpus'
corpus.mkdir(exist_ok=True)
for path in root.glob('*.tar.gz'):
    (corpus / path.name).write_bytes(path.read_bytes())
    (corpus / (path.stem + '.raw')).write_bytes(gzip.decompress(path.read_bytes()))
