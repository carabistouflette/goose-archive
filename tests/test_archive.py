# SPDX-License-Identifier: Apache-2.0
"""Public native boundary tests; fixture producer is Python's independent tarfile."""
import ctypes as C
import gzip
import io
import os
import random
from pathlib import Path
import stat
import struct
import subprocess
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent

class Slice(C.Structure):
    _fields_ = [('data', C.c_void_p), ('len', C.c_int64)]

def archive(entries=(), fmt=tarfile.USTAR_FORMAT):
    out = io.BytesIO()
    with tarfile.open(fileobj=out, mode='w', format=fmt) as tar:
        for name, data, kind in entries:
            info = tarfile.TarInfo(name)
            info.mode = 0o755 if kind == 'x' else 0o644
            if kind == 'd':
                info.type = tarfile.DIRTYPE
            elif kind == 's':
                info.type = tarfile.SYMTYPE
                info.linkname = '../../outside'
            elif kind == 'h':
                info.type = tarfile.LNKTYPE
                info.linkname = 'first'
            elif kind == 'p':
                info.type = tarfile.FIFOTYPE
            else:
                info.size = len(data)
            tar.addfile(info, io.BytesIO(data))
    return gzip.compress(out.getvalue(), mtime=0)

class Adapter:
    def __init__(self):
        if os.environ.get('ARCHIVE_DRIVER'):
            self.lib = None
            return
        self.lib = C.CDLL(str(Path(os.environ.get('ARCHIVE_SO', ROOT / '.build/libarchive-test.so')).resolve()))
        self.lib.ga_call.argtypes = [C.c_int64, Slice, Slice, Slice, Slice, Slice]
        self.lib.ga_call.restype = C.c_int64

    def call(self, data, destination=None, limits=None):
        values = limits or [16777216, 4096, 16777216, 16777216, 32, 1048576]
        options = (C.c_int64 * 6)(*values)
        meta = (C.c_int64 * 3)(-1, -1, -1)
        packed = C.create_string_buffer(values[5] if destination is None else 0)
        source = C.create_string_buffer(data)
        dest = os.fsencode(destination) if destination is not None else b''
        target = C.create_string_buffer(dest)
        if self.lib is None:
            request = struct.pack('<9q', int(destination is not None), len(data), len(dest), *values)
            response = subprocess.run([os.environ['ARCHIVE_DRIVER']], input=request + data + dest,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10, check=True).stdout
            code, *metadata = struct.unpack('<4q', response[:32])
            meta = metadata
            packed = C.create_string_buffer(response[32:])
        else:
            code = self.lib.ga_call(int(destination is not None), Slice(C.addressof(source), len(data)),
            Slice(C.addressof(target), len(dest)), Slice(C.addressof(options), 6),
            Slice(C.addressof(packed), len(packed)), Slice(C.addressof(meta), 3))
        records = []
        offset = 0
        if not code and destination is None:
            for _ in range(meta[2]):
                kind = packed.raw[offset]
                size = int.from_bytes(packed.raw[offset + 1:offset + 9], 'little')
                end = packed.raw.index(b'\0', offset + 9)
                records.append((packed.raw[offset + 9:end].decode('ascii'), size, kind))
                offset = end + 1
            assert offset == meta[1]
        return code, list(meta), records

class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = Adapter()

    def test_valid_formats_and_owned_metadata(self):
        for fmt in (tarfile.USTAR_FORMAT, tarfile.GNU_FORMAT, tarfile.PAX_FORMAT):
            data = archive([('src/', b'', 'd'), ('src/main.goose', b'hello\0world', 'f')], fmt)
            code, meta, records = self.api.call(data)
            self.assertEqual(code, 0)
            self.assertEqual(records, [('src', 0, 1), ('src/main.goose', 11, 0)])
            self.assertEqual(meta[0], 0)
        self.assertEqual(self.api.call(archive())[0], 0)

    def test_unsafe_paths_and_types(self):
        for name in ('/absolute', '../outside', 'a/../b', './file', 'a//b', 'a\\b',
                     'C:file', 'a/./b', 'a\nb', 'é', 'x' * 1025):
            with self.subTest(name=name):
                code = self.api.call(archive([(name, b'a', 'f')], tarfile.PAX_FORMAT))[0]
                # In the C locale libarchive rejects Unicode before path policy.
                self.assertIn(code, (3, 4) if name == 'é' else (4,))
        for kind in ('s', 'h', 'p'):
            self.assertEqual(self.api.call(archive([('entry', b'', kind)]))[0], 4)

    def test_duplicates_and_ancestor_conflicts(self):
        for entries in (
            [('a', b'', 'f'), ('a', b'', 'f')],
            [('a', b'', 'f'), ('a-', b'', 'f'), ('a/b', b'', 'f')],
            [('a/b', b'', 'f'), ('a', b'', 'f')],
            [('a', b'', 'f'), ('a/b', b'', 'd')],
            [('a/', b'', 'd'), ('a', b'', 'd')],
        ):
            self.assertEqual(self.api.call(archive(entries))[0], 4)

    def test_limits_exact_and_below(self):
        data = archive([('a/b', b'hello', 'f')])
        length = len(gzip.decompress(data))
        exact = [length, 1, 5, 5, 2, 13]
        self.assertEqual(self.api.call(data, limits=exact)[0], 0)
        for i in range(6):
            limits = exact.copy()
            limits[i] -= 1
            self.assertEqual(self.api.call(data, limits=limits)[0], 2, i)
        for i in range(6):
            limits = exact.copy()
            limits[i] = -1
            # Avoid negative output allocation in wrapper; direct ABI tested separately.
            if i != 5:
                self.assertEqual(self.api.call(data, limits=limits)[0], 1)

    def test_gzip_corruption_truncation_and_extra_members(self):
        data = archive([('file', b'hello', 'f')])
        for sample in (b'', b'bad', data[:-1], data + b'junk', data + data,
                       data[:-8] + b'\0' * 8, gzip.compress(b'not tar')):
            self.assertNotEqual(self.api.call(sample)[0], 0)
        for i in range(len(data)):
            self.assertNotEqual(self.api.call(data[:i])[0], 0)

    def test_tar_termination_and_hidden_content(self):
        raw = gzip.decompress(archive([('a', b'a', 'f')]))
        for sample in (raw[:1024], raw[:1536], raw + b'x', raw[:2048] + raw,
                       raw[:1024] + b'\0' * 512 + raw, raw + raw):
            with self.subTest(length=len(sample)):
                self.assertEqual(self.api.call(gzip.compress(sample))[0], 3)
        zero_payload = gzip.decompress(archive([('zero', b'\0' * 1024, 'f')]))
        for sample in (zero_payload[:1536], zero_payload[:2048]):
            self.assertEqual(self.api.call(gzip.compress(sample))[0], 3)
        corrupt = bytearray(raw)
        corrupt[0] ^= 1
        self.assertEqual(self.api.call(gzip.compress(corrupt))[0], 3)

    def test_extraction_permissions_content_and_no_overwrite(self):
        data = archive([('src/run', b'#!/bin/sh\n', 'x'), ('src/', b'', 'd'),
                        ('data', b'contents', 'f')])
        with tempfile.TemporaryDirectory() as temp:
            destination = Path(temp) / 'out'
            self.assertEqual(self.api.call(data, destination)[0], 0)
            self.assertEqual((destination / 'data').read_bytes(), b'contents')
            self.assertEqual((destination / 'src/run').read_bytes(), b'#!/bin/sh\n')
            for path, mode in ((destination, 0o700), (destination / 'src', 0o700),
                               (destination / 'src/run', 0o700), (destination / 'data', 0o600)):
                self.assertEqual(stat.S_IMODE(path.stat().st_mode), mode)
            self.assertEqual(self.api.call(data, destination)[0], 5)
            self.assertEqual((destination / 'data').read_bytes(), b'contents')

    def test_generated_valid_documents(self):
        rng = random.Random(219)
        for sample in range(80):
            entries = []
            expected = []
            for i in range(rng.randrange(1, 20)):
                name = f'dir-{i}/file-{sample}'
                payload = rng.randbytes(rng.randrange(0, 2048))
                entries.append((name, payload, 'f'))
                expected.append((name, len(payload), 0))
            fmt = (tarfile.USTAR_FORMAT, tarfile.GNU_FORMAT, tarfile.PAX_FORMAT)[sample % 3]
            code, _, records = self.api.call(archive(entries, fmt))
            self.assertEqual(code, 0)
            self.assertEqual(records, expected)

    def test_long_paths_and_pax_override(self):
        name = 'a' * 100 + '/' + 'b' * 100 + '/' + 'c' * 100
        for fmt in (tarfile.GNU_FORMAT, tarfile.PAX_FORMAT):
            code, _, entries = self.api.call(archive([(name, b'x', 'f')], fmt))
            self.assertEqual(code, 0)
            self.assertEqual(entries, [(name, 1, 0)])
        out = io.BytesIO()
        with tarfile.open(fileobj=out, mode='w', format=tarfile.PAX_FORMAT) as tar:
            info = tarfile.TarInfo('safe')
            info.pax_headers = {'path': '../escape'}
            tar.addfile(info)
        self.assertEqual(self.api.call(gzip.compress(out.getvalue()))[0], 4)

    def test_decompression_bomb_and_payload_truncation(self):
        data = archive([('large', b'0' * 2097152, 'f')])
        self.assertEqual(self.api.call(data, limits=[1048576, 10, 4194304, 4194304, 32, 1024])[0], 2)
        raw = gzip.decompress(archive([('large', b'x' * 2048, 'f')]))
        self.assertEqual(self.api.call(gzip.compress(raw[:1024] + b'\0' * 1024))[0], 3)

    def test_native_invalid_and_overlap(self):
        if self.api.lib is None:
            subprocess.run([os.environ['ARCHIVE_DRIVER'], '--selftest'], check=True, timeout=10)
            return
        options = (C.c_int64 * 6)(10240, 1, 10, 10, 2, 100)
        meta = (C.c_int64 * 3)(11, 22, 33)
        source = C.create_string_buffer(100)
        empty = Slice(None, 0)
        code = self.api.lib.ga_call(0, Slice(C.addressof(source), 100), empty,
            Slice(C.addressof(options), 6), Slice(C.addressof(source), 100), Slice(C.addressof(meta), 3))
        self.assertEqual(code, 1)
        self.assertEqual(list(meta), [11, 22, 33])

    def test_prevalidation_and_symlink_parent(self):
        with tempfile.TemporaryDirectory() as temp:
            destination = Path(temp) / 'out'
            data = archive([('valid', b'abc', 'f'), ('../bad', b'', 'f')])
            self.assertEqual(self.api.call(data, destination)[0], 4)
            self.assertFalse(destination.exists())
            link = Path(temp) / 'link'
            link.symlink_to(temp, target_is_directory=True)
            self.assertEqual(self.api.call(archive(), link / 'out')[0], 5)
            self.assertFalse(destination.exists())
            self.assertEqual(self.api.call(archive(), 'relative')[0], 1)
            self.assertEqual(self.api.call(archive(), temp + '/a/../out')[0], 1)

if __name__ == '__main__':
    unittest.main()
