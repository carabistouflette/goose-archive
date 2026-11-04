#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
import os
from pathlib import Path
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parent.parent
compiler = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='goose-archive-') as temp:
    for command in ([compiler, 'run', 'tests/api.goose', '--', temp + '/interpreted'],
                    [compiler, 'build', 'tests/api.goose', '-o', '.build/api'],
                    [str(root / '.build/api'), temp + '/native']):
        subprocess.run(command, cwd=root, check=True, timeout=120)
