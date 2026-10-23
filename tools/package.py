"""Package the reviewed repository without Git internals or generated build caches."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import argparse
import hashlib

ROOT = Path(__file__).resolve().parent.parent
DIRECTORIES = ('include', 'src', 'tests', 'data', 'assets', 'docs', 'tools', '.github')
FILES = ('README.md', 'GeoViz.exe', 'build.ps1', 'CMakeLists.txt', '.gitignore', '.gitattributes', '.clang-format')
LEGACY = {'FULL_GUIDE.html', 'FULL_GUIDE.md', 'GUIDE_TEXT.md', 'DEMO.md', 'REQUIREMENTS.md',
          'SOURCE_MANIFEST.sha256', 'make_guide.py', 'render_guide.mjs'}


def release_files():
    result = [ROOT / name for name in FILES]
    for name in DIRECTORIES:
        result.extend(p for p in (ROOT / name).rglob('*') if p.is_file()
                      and p.name not in LEGACY and '__pycache__' not in p.parts and p.suffix != '.pyc')
    return sorted(set(result))


def manifest():
    rows = []
    for path in release_files():
        if path.name != 'RELEASE_MANIFEST.sha256':
            rows.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(ROOT).as_posix()}')
    (ROOT / 'RELEASE_MANIFEST.sha256').write_text('\n'.join(rows) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, default=ROOT.parent / 'GeoViz_Submission.zip')
    parser.add_argument('--manifest-only', action='store_true')
    args = parser.parse_args()
    manifest()
    if args.manifest_only:
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
