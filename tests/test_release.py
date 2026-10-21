import copy
import hashlib
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('verify_release', Path(__file__).parents[1] / 'packaging/verify-release.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.version = '2.0.0-rc6'
        self.names = [f'discforge_{self.version}+ubuntu{os}_{arch}.deb'
                      for os in ('22.04', '24.04', '26.04') for arch in ('amd64', 'arm64')]
        self.digest = hashlib.sha256(b'package').hexdigest()
        self.sums = ''.join(f'{self.digest}  ./{n}\n' for n in self.names).encode()
        self.metadata = {'tag_name': 'v' + self.version, 'draft': False, 'assets': [
            {'name': n, 'state': 'uploaded', 'digest': 'sha256:' + self.digest} for n in self.names]}
        self.metadata['assets'] += [
            {'name': 'SHA256SUMS', 'state': 'uploaded', 'digest': 'sha256:' + hashlib.sha256(self.sums).hexdigest()},
            {'name': 'RELEASE_NOTES.zh-CN.md', 'state': 'uploaded', 'digest': 'sha256:' + self.digest}]

    def test_matching_release(self):
        module.verify(self.metadata, self.sums, self.version)

    def test_renamed_attachment(self):
        self.metadata['assets'][0]['name'] = 'renamed.deb'
        with self.assertRaises(ValueError):
            module.verify(self.metadata, self.sums, self.version)

    def test_missing_attachment(self):
        self.metadata['assets'].pop(0)
        with self.assertRaises(ValueError):
            module.verify(self.metadata, self.sums, self.version)

    def test_wrong_content(self):
        self.metadata['assets'][0]['digest'] = 'sha256:' + '0' * 64
        with self.assertRaises(ValueError):
            module.verify(self.metadata, self.sums, self.version)

    def test_corrupt_checksum_file(self):
        with self.assertRaises(ValueError):
            module.verify(self.metadata, self.sums.replace(b'./discforge', b'./wrong', 1), self.version)

    def test_duplicate_checksum(self):
        with self.assertRaises(ValueError):
            module.verify(self.metadata, self.sums + self.sums.splitlines(keepends=True)[0], self.version)


if __name__ == '__main__':
    unittest.main()
