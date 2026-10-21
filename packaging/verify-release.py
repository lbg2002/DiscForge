#!/usr/bin/env python3
"""Check GitHub's actual attachment names against its checksum manifest."""
import argparse
import hashlib
import json
from pathlib import Path


def verify(metadata, checksums, version, local_dir=None):
    platforms = ('22.04', '24.04', '26.04')
    arches = ('amd64', 'arm64')
    debs = {f'discforge_{version}+ubuntu{os}_{arch}.deb' for os in platforms for arch in arches}
    expected = debs | {'SHA256SUMS', 'RELEASE_NOTES.zh-CN.md'}
    assets = metadata['assets']
    remote = {a['name']: a for a in assets}
    if len(remote) != len(assets) or set(remote) != expected:
        raise ValueError('Release attachment names differ from the six-platform manifest')
    if metadata['tag_name'] != 'v' + version or metadata['draft']:
        raise ValueError('Release tag or publication state differs')
    listed = {}
    for line in checksums.decode('utf-8').splitlines():
        digest, name = line.split(maxsplit=1)
        name = name.removeprefix('*').removeprefix('./')
        if len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('Invalid checksum digest')
        if name in listed:
            raise ValueError('Duplicate checksum entry')
        listed[name] = digest
    if set(listed) != debs:
        raise ValueError('Checksum filenames differ from published debs')
    for name, asset in remote.items():
        if asset['state'] != 'uploaded' or not asset.get('digest', '').startswith('sha256:'):
            raise ValueError('Attachment not fully uploaded: ' + name)
        if name in listed and asset['digest'] != 'sha256:' + listed[name]:
            raise ValueError('Published content digest differs: ' + name)
    if remote['SHA256SUMS']['digest'] != 'sha256:' + hashlib.sha256(checksums).hexdigest():
        raise ValueError('Downloaded checksum manifest content differs')
    if local_dir is not None:
        for name in expected:
            digest = hashlib.sha256((Path(local_dir) / name).read_bytes()).hexdigest()
            if remote[name]['digest'] != 'sha256:' + digest:
                raise ValueError('Local upload differs from published content: ' + name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--metadata', required=True, type=Path)
    parser.add_argument('--checksums', required=True, type=Path)
    parser.add_argument('--version', required=True)
    parser.add_argument('--local-dir', type=Path)
    args = parser.parse_args()
    verify(json.loads(args.metadata.read_text()), args.checksums.read_bytes(), args.version, args.local_dir)
    print('Published attachment names and SHA-256 verified')
