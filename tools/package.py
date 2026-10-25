"""Package the reviewed repository without Git internals or generated build caches."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import argparse
import hashlib

ROOT = Path(__file__).resolve().parent.parent
DIRECTORIES = ('include', 'src', 'tests', 'data', 'assets', 'docs', 'tools', '.github')
FILES = ('README.md', 'GeoViz.exe', 'build.ps1', 'CMakeLists.txt', '.gitignore', '.gitattributes', '.clang-format', 'LICENSE', 'CHANGELOG.md', 'CONTRIBUTING.md', 'CMakePresets.json', 'package.json', 'package-lock.json', 'requirements-docs.txt')
LEGACY = {'FULL_GUIDE.html', 'FULL_GUIDE.md', 'GUIDE_TEXT.md', 'DEMO.md', 'REQUIREMENTS.md',
          'SOURCE_MANIFEST.sha256', 'make_guide.py', 'render_guide.mjs'}


def release_files():
    result = [ROOT / name for name in FILES]
    if (ROOT/'BUILD_PROVENANCE.json').is_file():result.append(ROOT/'BUILD_PROVENANCE.json')
    for name in DIRECTORIES:
        result.extend(p for p in (ROOT / name).rglob('*') if p.is_file()
                      and p.name not in LEGACY and '__pycache__' not in p.parts and p.suffix != '.pyc')
    return sorted(set(result))


def manifest(check=False):
    rows = []
    for path in release_files():
        if path.name != 'RELEASE_MANIFEST.sha256':
            rows.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(ROOT).as_posix()}')
    text='\n'.join(rows)+'\n'
    path=ROOT/'RELEASE_MANIFEST.sha256'
    if check:
        if not path.exists() or path.read_text()!=text: raise RuntimeError('Manifest is stale; run package.py --manifest-only')
    else: path.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, default=ROOT.parent / 'GeoViz_Submission.zip')
    parser.add_argument('--manifest-only', action='store_true')
    parser.add_argument('--check',action='store_true')
    args = parser.parse_args()
    manifest(args.check)
    if args.manifest_only or args.check:
        print('Updated release hashes.')
        return
    files = release_files() + [ROOT / 'RELEASE_MANIFEST.sha256']
    with ZipFile(args.destination, 'w', ZIP_DEFLATED, compresslevel=8) as archive:
        for path in files:
            archive.write(path, Path('GeoViz') / path.relative_to(ROOT))
    with ZipFile(args.destination) as archive:
        if archive.testzip() is not None:
            raise RuntimeError('Archive integrity failed.')
    print(f'Packaged {len(files)} files: {args.destination} ({args.destination.stat().st_size:,} bytes).')


if __name__ == "__main__":
    main()
