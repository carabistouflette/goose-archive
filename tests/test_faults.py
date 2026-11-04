# SPDX-License-Identifier: Apache-2.0
"""Test-only syscall fault injection; the production target contains no hooks."""
import errno
import os
from pathlib import Path
import tempfile
import unittest
from test_archive import Adapter, archive

class FaultTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = Adapter(faults=True)
        cls.payload = bytes(range(256)) * 1024
        cls.data = archive([('nested/data', cls.payload, 'f'), ('later', b'untouched', 'f')])

    def test_short_and_interrupted_writes(self):
        for mode in (1, 2):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(dir='/tmp') as temp:
                target = Path(temp) / 'out'
                code, meta, _ = self.api.call(self.data, target, fault=mode)
                self.assertEqual(code, 0)
                self.assertEqual(meta[0], 0)
                self.assertEqual((target / 'nested/data').read_bytes(), self.payload)
                self.assertEqual((target / 'later').read_bytes(), b'untouched')

    def test_write_and_close_failures(self):
        before = len(os.listdir('/proc/self/fd'))
        for mode, error in ((3, errno.ENOSPC), (4, errno.EIO), (5, errno.EIO)):
            for _ in range(10):
                with self.subTest(mode=mode), tempfile.TemporaryDirectory(dir='/tmp') as temp:
                    target = Path(temp) / 'out'
                    code, meta, _ = self.api.call(self.data, target, fault=mode)
                    self.assertEqual((code, meta), (5, [error, 0, 0]))
                    self.assertFalse((target / 'later').exists())
                    content = (target / 'nested/data').read_bytes()
                    self.assertEqual(content, self.payload[:7] if mode == 3 else b'' if mode == 4 else self.payload)
        self.assertEqual(len(os.listdir('/proc/self/fd')), before)

    def test_open_mkdir_and_dup_failures(self):
        before = len(os.listdir('/proc/self/fd'))
        for mode, indices, error in ((6, range(1, 8), errno.EMFILE),
                                     (7, range(1, 3), errno.ENOSPC),
                                     (8, (1,), errno.EMFILE)):
            for at in indices:
                with self.subTest(mode=mode, at=at), tempfile.TemporaryDirectory(dir='/tmp') as temp:
                    target = Path(temp) / 'out'
                    code, meta, _ = self.api.call(self.data, target, fault=mode, at=at)
                    self.assertEqual((code, meta), (5, [error, 0, 0]))
                    self.assertFalse((target / 'later').exists())
        self.assertEqual(len(os.listdir('/proc/self/fd')), before)
