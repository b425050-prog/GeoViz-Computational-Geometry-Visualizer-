"""Check delivery hashes and local documentation links with Python's standard library."""
from pathlib import Path
from urllib.parse import unquote, urlparse
import hashlib
import re
import csv
import sys

sys.dont_write_bytecode = True
from package import release_files, FILES

ROOT = Path(__file__).resolve().parent.parent


def main():
    count = 0
    expected_files = {path.relative_to(ROOT).as_posix() for path in release_files()}
    manifest_files = set()
    for row in (ROOT / 'RELEASE_MANIFEST.sha256').read_text().splitlines():
        expected, name = row.split('  ', 1)
        if name not in expected_files or name in manifest_files or not re.fullmatch(r'[a-f0-9]{64}', expected):
            raise RuntimeError(f'Invalid or duplicate manifest entry: {name}')
        manifest_files.add(name)
        file = ROOT / name
        if hashlib.sha256(file.read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'Hash mismatch: {name}')
        count += 1
    if manifest_files != expected_files:
        raise RuntimeError('Release manifest does not cover every delivery file.')
    allowed = set(FILES) | {'RELEASE_MANIFEST.sha256'}
    if any(path.is_file() and path.name not in allowed for path in ROOT.iterdir()):
        raise RuntimeError('Unexpected top-level delivery file.')
    for file in [ROOT / 'README.md', *(ROOT / 'docs').glob('*.md'), ROOT / 'docs/index.html']:
        content = file.read_text(encoding='utf-8')
        links = re.findall(r'\]\(([^)]+)\)', content) if file.suffix == '.md' else re.findall(r'(?:src|href)="([^"]+)"', content)
        for link in links:
            if urlparse(link).scheme or link.startswith('#'):
                continue
            local = unquote(link.split('#')[0].split('?')[0])
            if local and not (file.parent / local).exists():
                raise RuntimeError(f'Broken local link in {file.name}: {link}')
    if len(list((ROOT / 'data').rglob('*.scene'))) != 60:
        raise RuntimeError('Expected 60 editable scenes.')
    catalogue = list(csv.DictReader((ROOT/'data/security_samples.csv').open(encoding='utf-8')))
    if len(catalogue) != 9 or len({row['file'] for row in catalogue}) != 9:
        raise RuntimeError('Expected nine distinct security scenarios.')
    if any(not (ROOT/'data'/row['file']).is_file() for row in catalogue):
        raise RuntimeError('Missing security scenario input.')
    if list((ROOT/'docs/runs').glob('*.txt.scene')):
        raise RuntimeError('Redundant documentation scene snapshots remain.')
    if (ROOT / 'GeoViz.exe').read_bytes()[:2] != b'MZ':
        raise RuntimeError('Executable header is invalid.')
    print(f'PASS: {count} delivery hashes, local document links, 60 scenes, 9 security scenarios and executable header.')


if __name__ == '__main__':
    main()
