"""Compile the sample catalogue from checked run exports, without hand-copying answers."""
from pathlib import Path
import argparse
import csv
import shutil

ROOT = Path(__file__).resolve().parent.parent
MODES = ['Convex hull', 'Geofence', 'Dijkstra', 'A*', 'DBSCAN scan', 'Closest pair',
         'KD nearest', 'KD radius', 'DBSCAN indexed', 'Segment collision']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=ROOT,
                        help='Root containing checked output/ exports (default: project root).')
    source_root = parser.parse_args().source_root.resolve()
    target = ROOT / 'docs/runs'
    target.mkdir(parents=True, exist_ok=True)
    lines = ['# Sample catalogue', '',
             'The laboratory contains 60 deterministic, editable scenes, arranged as six inputs per algorithm. **Samples / F2** selects a named scenario; **N** cycles the selected mode. Import a scene, select its listed mode and run the calculation. Each checked report links to its corresponding input.', '',
             'The [offline guide](index.html) includes checked algorithm and security recordings. Operation counts exclude verification and playback.', '',
             '| Tidal and lagoon family | Terrace and mosaic family |',
             '| --- | --- |',
             '| ![Lagoon rings with a tidal surface](media/tidal-gallery.png) | ![Terraced route with a limestone surface](media/terrace-gallery.png) |', '']
    titles = ['Compact reference', 'Coastal survey / campus deliveries / festival courtyards',
              'Orbital sensors / warehouse aisles / curved swarms',
              'Orchard inspection / valley detour / mixed-density survey',
              'Tidal archipelago / lagoon survey / island bridges',
              'Terraced botanical garden / planted corridors / mosaic survey']
    for variant, title in enumerate(titles):
        lines.extend([f'## {variant + 1}. {title}', '',
                      '| Mode | Points | Radius / MinPts | Checked result | Input / report |',
                      '| --- | ---: | --- | --- | --- |'])
        for mode, label in enumerate(MODES):
            name = f'sample_{mode}' if variant == 0 else f'mode_{mode}_{variant}'
            src = source_root / 'output' / ('' if variant == 0 else 'gallery') / f'{name}.txt'
            scene = f'data/sample_{mode}.scene' if variant == 0 else f'data/gallery/{name}.scene'
            content = src.read_text()
            fields = dict(row.split(': ', 1) for row in content.splitlines() if ': ' in row)
            snapshot = Path(str(src) + '.scene')
            point_count = snapshot.read_text().splitlines()[1]
            if snapshot.read_bytes() != (ROOT / scene).read_bytes():
                raise RuntimeError(f'Checked report does not match its documented input: {name}')
            shutil.copy2(src, target / src.name)
            params = f"{float(fields['Radius']):g} / {fields['MinPts']}"
            lines.append(f"| {label} | {point_count} | {params} | {fields['Result']} | [Scene](../{scene}) · [Run](runs/{name}.txt) |")
        lines.append('')
    lines.extend(['## Security datasets', '',
                  'In either security view, **Sample / N** cycles the catalogue. Each selection restores its documented radii, MinPts, fence and default window. Changing views preserves the current settings and reviews. All records are synthetic.', '',
                  '| Dataset | File / selected records | Devices | Scenario |', '| --- | ---: | ---: | --- |'])
    catalogue = list(csv.DictReader((ROOT/'data/security_samples.csv').open(encoding='utf-8')))
    for sample in catalogue:
        records = list(csv.DictReader((ROOT/'data'/sample['file']).open(encoding='utf-8')))
        selected = [row for row in records if 1800000000 <= int(row['timestamp']) < 1800000600]
        devices = len({row['device_id'] for row in selected})
        lines.append(f"| [{sample['name']}](../data/{sample['file']}) | {len(records)} / {len(selected)} | {devices} | {sample['purpose']} |")
    lines.extend(['',
                  'Use the source-linked inspector to review a device, then export evidence. [Security processing](SECURITY.md) explains the metric, filters, labels and review rule.', '',
                  '## Additional grid', '',
                  '**Grid** loads 70 vertices with unit-cost four-neighbour connections and a barrier opening. Source 0 to goal 69 has optimal cost **15** in both Dijkstra and A*. Block another passage to test detours or unreachable results.', '',
                  '## Deliberate edge cases', '',
                  'The tests also construct duplicates, collinear hulls, exact-radius queries, nearest ties, overlapping segments, zero-length segments, unreachable graphs and noise-to-border transitions. These cases are separate from the larger visual samples so their expected answers stay easy to inspect.', ''])
    (ROOT / 'docs/SAMPLES.md').write_text('\n'.join(lines), encoding='utf-8')
    print(f'Built catalogue for {len(titles) * len(MODES)} checked scenes and {len(catalogue)} security datasets.')


if __name__ == '__main__':
    main()
