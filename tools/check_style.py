"""Portable source hygiene and architecture gate; compiler warnings remain errors."""
from pathlib import Path
import re
root=Path(__file__).resolve().parent.parent
for directory in ('include','src','tests','tools'):
    for file in (root/directory).rglob('*'):
        if file.suffix not in ('.hpp','.cpp'):continue
        text=file.read_text(encoding='utf-8')
        if '\t' in text:raise RuntimeError(f'Tab indentation: {file}')
        if re.search(r'[ \t]+$',text,re.M):raise RuntimeError(f'Trailing whitespace: {file}')
        if directory=='include' and re.search(r'#include [<"]raylib',text):raise RuntimeError('Engine depends on graphics')
if len((root/'include/geoviz.hpp').read_text().splitlines())>30:raise RuntimeError('Aggregate header is growing again')
if len((root/'src/main.cpp').read_text().splitlines())>100:raise RuntimeError('Entry point is growing again')
print('PASS: source hygiene, graphics boundary and focused entry points.')
