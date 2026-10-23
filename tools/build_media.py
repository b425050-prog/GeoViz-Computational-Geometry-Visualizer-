"""Build offline diagrams and animated previews from exported C++ engine states.

Authoring dependency: Pillow. Generated assets are ordinary repository files;
neither this script nor Python is needed to run the desktop application.
"""
from pathlib import Path
from html import escape
import json
import math
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
MEDIA = ROOT / 'docs/media'
MEDIA.mkdir(parents=True, exist_ok=True)
FONT = ROOT / 'assets/Lato-Regular.ttf'
BOLD = ROOT / 'assets/Lato-Bold.ttf'
INK, ACCENT, MUTED = '#273430', '#24674d', '#52655c'
COLORS = ['#236a91', '#9b5e15', '#75477f', '#2d7042', '#a94135', '#6f6f24']


def svg(name, title, subtitle, body, width=920, height=400):
    header = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" role="img" aria-label="{escape(title)}">
    <defs><pattern id="grid" width="28" height="28" patternUnits="userSpaceOnUse"><path d="M28 0H0V28" fill="none" stroke="#e7ebe1"/></pattern>
    <marker id="arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0 0L8 4L0 8" fill="none" stroke="{ACCENT}"/></marker></defs>
    <rect width="100%" height="100%" rx="18" fill="#f7f7ef"/>
    <text x="30" y="40" font-family="Segoe UI,sans-serif" font-size="23" font-weight="600" fill="{INK}">{escape(title)}</text>
    <text x="30" y="68" font-family="Segoe UI,sans-serif" font-size="16" fill="{MUTED}">{escape(subtitle)}</text>
    <g font-family="Segoe UI,sans-serif" font-size="17" fill="{INK}">{body}</g></svg>'''
    (MEDIA / (name + '.svg')).write_text(header, encoding='utf-8')


def line(a, b, color=ACCENT, dash=False, width=3):
    return f'<path d="M{a[0]} {a[1]}L{b[0]} {b[1]}" stroke="{color}" stroke-width="{width}" fill="none"' + (' stroke-dasharray="7 6"' if dash else '') + '/>'


def point(x, y, label='', color='#395d6f'):
    return f'<circle cx="{x}" cy="{y}" r="6" fill="{color}"/><text x="{x+11}" y="{y-10}">{escape(label)}</text>'


def card(x, y, title, detail, width=250):
    return f'<rect x="{x}" y="{y}" width="{width}" height="76" rx="10" fill="white" stroke="#ccd6cc"/><text x="{x+16}" y="{y+29}" font-weight="600">{escape(title)}</text><text x="{x+16}" y="{y+55}" font-size="15" fill="{MUTED}">{escape(detail)}</text>'


def diagrams():
    pts = [(80, 315, 'A'), (365, 315, 'B'), (365, 125, 'C'), (80, 125, 'D'), (200, 240, 'inside'), (230, 315, 'collinear')]
    hull = '<path d="M80 315H365V125H80Z" fill="#dcedde" stroke="#24674d" stroke-width="4"/>'
    hull += ''.join(point(x, y, name) for x, y, name in pts)
    hull += card(470, 125, '01  Sort coordinates', 'Exact ordering, then deduplicate', 400)
    hull += card(470, 225, '02  Keep left turns', 'Pop the middle point on right / flat turns', 400)
    hull += '<text x="470" y="347">Interior and collinear side points are excluded.</text>'
    svg('hull', 'Convex hull / monotone chain', 'Build lower and upper chains; retain only the extreme boundary.', hull)
    fence = '<path d="M75 325V130H370V325Z" fill="#dcedde" stroke="#24674d" stroke-width="3"/>'
    fence += line((190, 230), (435, 230), '#8f5b14', True) + point(190, 230, 'query') + point(370, 230, 'crossing', '#8f5b14')
    fence += card(490, 115, 'Boundary first', 'On an edge or vertex -> BOUNDARY', 370)
    fence += card(490, 215, 'Then crossing parity', 'Odd -> INSIDE; even -> OUTSIDE', 370)
    fence += '<text x="490" y="340">Half-open endpoints avoid double counting.</text>'
    svg('geofence', 'Polygon membership / ray casting', 'A boundary test precedes the rightward ray.', fence)
    A, B, C, D = (90, 290), (290, 290), (90, 135), (290, 135)
    routes = line(A, B) + line(B, D) + line(A, C, '#abb7aa') + line(C, D, '#abb7aa')
    routes += ''.join(point(*p, label) for p, label in [(A, 'A / source'), (B, 'B'), (C, 'C'), (D, 'D / goal')])
    routes += '<text x="180" y="315">2</text><text x="310" y="215">2</text><text x="62" y="220">3</text><text x="180" y="120">2</text>'
    routes += card(470, 110, 'Relax an edge', 'candidate = g[u] + weight(u,v)', 400)
    routes += card(470, 210, 'Choose the next vertex', 'Dijkstra: g     A*: g + compatible h', 400)
    routes += '<text x="470" y="340">Both return A -> B -> D, optimal cost 4.</text>'
    svg('routing', 'Shortest paths / Dijkstra and A*', 'Shared relaxation and predecessor reconstruction; different ranking.', routes)
    db = '<circle cx="190" cy="235" r="85" fill="#e7f0f4" stroke="#8f5b14" stroke-dasharray="6 5"/>'
    for x, y in [(150, 270), (220, 270), (150, 200), (220, 200)]: db += point(x, y, '', COLORS[0])
    db += '<rect x="283" y="193" width="14" height="14" fill="none" stroke="#236a91" stroke-width="3"/>'
    db += line((363, 135), (377, 149), '#ab3e30') + line((363, 149), (377, 135), '#ab3e30')
    db += '<text x="105" y="315">A-D: core</text><text x="275" y="235">E: border</text><text x="340" y="117">F: noise</text>'
    db += card(490, 115, 'Inclusive neighbourhood', 'MinPts counts the point itself', 370)
    db += card(490, 215, 'Queue expansion', 'A previous noise point may become border', 370)
    svg('dbscan', 'DBSCAN / density connectivity', 'Core, border and noise use different shapes; radius 1.1, MinPts 3 in the worked example.', db)
    close = '<rect x="205" y="110" width="85" height="230" fill="#e4eddf"/><path d="M250 100V355" stroke="#8f5b14" stroke-dasharray="7 5"/>'
    for x, y in [(80, 300), (140, 180), (235, 230), (254, 220), (360, 145), (400, 320)]: close += point(x, y)
    close += line((235, 230), (254, 220), '#ab3e30', width=5)
    close += card(490, 115, 'Divide and solve', 'Remember the x split; solve both halves', 370)
    close += card(490, 215, 'Merge vertical order', 'Only a bounded strip can improve the best', 370)
    svg('closest', 'Closest pair / divide and conquer', 'The highlighted pair can straddle the dividing line.', close)
    kd = '<rect x="60" y="110" width="365" height="230" fill="url(#grid)" stroke="#ccd6cc"/>'
    kd += line((235, 110), (235, 340)) + line((60, 225), (235, 225), '#8f5b14') + line((235, 195), (425, 195), '#8f5b14')
    kd += '<rect x="235" y="110" width="190" height="85" fill="#f3ded6" stroke="#ab3e30" stroke-dasharray="7 5"/>'
    for x, y in [(95, 280), (180, 190), (235, 245), (320, 140), (365, 270)]: kd += point(x, y)
    kd += point(135, 290, 'query', '#8f5b14')
    kd += card(490, 115, 'Alternate median axes', 'x, then y, then x ...', 370)
    kd += card(490, 215, 'Use a box lower bound', 'Prune only when no answer can improve', 370)
    svg('kdtree', 'Balanced k-d tree / spatial pruning', 'Near-side traversal tightens the best-distance bound.', kd)
    collision = line((80, 310), (350, 130), '#ab3e30') + line((80, 130), (350, 310), '#ab3e30')
    collision += point(215, 220, 'intersection', '#ab3e30')
    collision += line((75, 355), (360, 355), '#a4b29d')
    collision += card(490, 115, 'Orientation tests', 'Proper crossing: opposite sides', 370)
    collision += card(490, 215, 'Closed endpoints', 'Touch, overlap and degeneracy included', 370)
    svg('collision', 'Segment collision / shared predicates', 'Each consecutive point pair defines an independent segment.', collision)
    pipeline = ''
    for x, title, detail in [(25, 'CSV records', 'IDs, UTC, counts, locations'), (335, 'Filter and transform', 'One window; two unit systems'), (645, 'Shared algorithms', 'Fence, proximity and density')]:
        pipeline += card(x, 110, title, detail)
    pipeline += '<path d="M280 148H325M590 148H635" stroke="#24674d" stroke-width="2" marker-end="url(#arrow)"/>'
    pipeline += card(25, 245, 'Source-linked observations', 'One record or device aggregation', 400)
    pipeline += card(490, 245, 'Inspect and export evidence', 'Truth labels and analyst verdicts stay separate', 400)
    svg('security-flow', 'Security analysis / shared engine integration', 'Geography uses local metres; behaviour uses scaled rates.', pipeline)
    classes = ''
    for x, y, title, detail in [(25, 110, 'App', 'View + Playback + SecurityConsole'), (335, 110, 'AlgorithmRunner', 'execute(scene, mode)'), (645, 110, 'Engine', 'owns Algorithm strategies'),
                                (25, 245, 'Run', 'Scene + Result + Trace snapshots'), (335, 245, 'Algorithm', 'run -> calculate -> verify'), (645, 245, 'SpatialIndex', 'LinearIndex | KDTree')]:
        classes += card(x, y, title, detail)
    classes += '<path d="M280 148H325M590 148H635M770 195V225H460V235M590 282H635" stroke="#24674d" stroke-width="2" fill="none" marker-end="url(#arrow)"/>'
    svg('classes', 'Object model / ownership and substitution', 'Solid links: calls and composition; labelled boxes show interface implementations.', classes)


def recordings():
    text = (MEDIA / 'traces.js').read_text()
    return json.loads(text.removeprefix('window.GEOVIZ_TRACES=').rstrip().removesuffix(';'))


def preview(record, index, width=900, height=540):
    """Render exported coordinates/frames; do not recalculate algorithm answers."""
    image = Image.new('RGB', (width, height), '#f6f5f0')
    d = ImageDraw.Draw(image)
    font = ImageFont.truetype(str(FONT), 18)
    small = ImageFont.truetype(str(FONT), 15)
    bold = ImageFont.truetype(str(BOLD), 25)
    d.text((26, 20), record['title'], font=bold, fill=INK)
    d.text((26, 56), f"C++ recording  /  step {index+1} of {len(record['frames'])}", font=small, fill=MUTED)
    area = (24, 100, width - 24, height - 92)
    d.rounded_rectangle(area, radius=14, fill='#fcfcf7', outline='#ccd6cc')
    points = record['points']
    all_points = points + (record['polygon'] if record['mode'] == 1 else [])
    if record['mode'] in (1, 6, 7): all_points = all_points + [record['query']]
    x0, y0 = map(min, zip(*all_points))
    x1, y1 = map(max, zip(*all_points))
    scale = min((width - 130) / max(1, x1-x0), (height-260) / max(1, y1-y0))
    def screen(p): return (width/2 + (p[0]-(x0+x1)/2)*scale, (area[1]+area[3])/2 - (p[1]-(y0+y1)/2)*scale)
    for x in range(40, width-24, 45): d.line((x, 105, x, height-98), fill='#e7ebe1')
    for y in range(115, height-96, 40): d.line((30, y, width-30, y), fill='#e7ebe1')
    frame = record['frames'][index]
    if record['mode'] == 1:
        poly = [screen(p) for p in record['polygon']]
        d.polygon(poly, fill='#e4eddf', outline=ACCENT, width=3)
        q = screen(record['query']); d.line((q[0], q[1], width-36, q[1]), fill='#8f5b14', width=2)
    for a, b, weight, blocked in record['edges']:
        d.line([screen(points[a]), screen(points[b])], fill='#ab3e30' if blocked else '#a3b6a6', width=2)
    if record['mode'] == 9:
        for i in range(0, len(points)-1, 2): d.line([screen(points[i]), screen(points[i+1])], fill='#71897a', width=2)
    for key, color in [('regions', '#c2a466'), ('pruned', '#cd8074')]:
        for box in frame[key]:
            a, b = screen((max(x0, box[0]), max(y0, box[1]))), screen((min(x1, box[2]), min(y1, box[3])))
            if a[0] <= b[0] and b[1] <= a[1]: d.rectangle((a[0], b[1], b[0], a[1]), outline=color, width=2)
    chain = [screen(points[i]) for i in frame['chain']]
    if len(chain) >= 2: d.line(chain, fill=ACCENT, width=4)
    if frame['radius'] > 0 and 0 <= frame['focus'] < len(points):
        x, y = screen(points[frame['focus']]); r = frame['radius'] * scale
        d.ellipse((x-r, y-r, x+r, y+r), outline='#ad812d', width=2)
    for i, p in enumerate(points):
        x, y = screen(p); color = '#395d6f'; kind = -1
        if record['mode'] in (4, 8) and i < len(frame['labels']):
            label = frame['labels'][i]
            color = COLORS[label % len(COLORS)] if label >= 0 else '#ab3e30' if label == -1 else '#a3ada4'
            if i < len(frame['kind']): kind = frame['kind'][i] if label != -2 else -1
        if i in frame['active']: d.ellipse((x-10, y-10, x+10, y+10), outline='#ad812d', width=2)
        if kind == 0:
            d.line((x-5, y-5, x+5, y+5), fill=color, width=2); d.line((x-5, y+5, x+5, y-5), fill=color, width=2)
        elif kind == 1: d.rectangle((x-5, y-5, x+5, y+5), outline=color, width=2)
        else: d.ellipse((x-5, y-5, x+5, y+5), fill=color)
        if i == frame['focus']: d.rectangle((x-12, y-12, x+12, y+12), outline='#ad812d', width=2)
    if record['mode'] in (1, 6, 7):
        x, y = screen(record['query']);d.line((x-8,y,x+8,y),fill=INK,width=2);d.line((x,y-8,x,y+8),fill=INK,width=2)
    message = frame['text']
    # Wrap rather than crop long explanations.
    words, lines, row = message.split(), [], ''
    for word in words:
        candidate = f'{row} {word}'.strip()
        if d.textlength(candidate, font=font) > width-55 and row: lines.append(row);row=word
        else: row=candidate
    lines.append(row)
    d.multiline_text((26, height-75), '\n'.join(lines[:2]), font=font, fill=INK, spacing=6)
    return image


def animations():
    data = recordings()
    for name, modes in [('overview', [0, 2, 4, 6]), ('hull', [0]), ('routes', [3]), ('density', [8]),
                        ('security', list(range(18, len(data))))]:
        frames = []
        for mode in modes:
            record = data[mode]
            count = min(42, len(record['frames']))
            for j in range(count):
                index = round(j * (len(record['frames'])-1) / max(1, count-1))
                frames.append(preview(record, index))
            frames.extend([frames[-1]] * 8)
        frames[0].save(MEDIA / f'{name}.gif', save_all=True, append_images=frames[1:], duration=110,
                       loop=0, optimize=True)
    # The icon is code-native geometry, with multiple Windows shell sizes.
    icon = Image.new('RGBA', (256, 256), (0, 0, 0, 0)); draw = ImageDraw.Draw(icon)
    draw.rounded_rectangle((8, 8, 248, 248), radius=52, fill='#24674d')
    polygon = [(49, 168), (77, 66), (171, 49), (215, 136), (151, 209)]
    draw.polygon(polygon, fill='#dcedde');draw.line(polygon+[polygon[0]], fill='#ffffff', width=6)
    for x, y in polygon: draw.ellipse((x-8, y-8, x+8, y+8), fill='#ffffff')
    draw.ellipse((113, 120, 131, 138), fill='#8f5b14')
    icon.save(ROOT / 'assets/geoviz.png')
    icon.save(ROOT / 'assets/geoviz.ico', sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])


if __name__ == '__main__':
    diagrams()
    animations()
    print('Built nine diagrams, five engine-recording previews and application icons.')
