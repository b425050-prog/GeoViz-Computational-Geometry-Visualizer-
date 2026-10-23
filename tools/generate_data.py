"""Recreate the synthetic security catalogue, event exports and campus map."""
from pathlib import Path
import csv
import math

ROOT = Path(__file__).resolve().parent.parent


def geographic(x, y):
    """Inverse of the C++ local projection, using the same origin and radius."""
    radius = 6371000
    return (20.2961 + math.degrees(y / radius),
            85.8245 + math.degrees(x / (radius * math.cos(math.radians(20.2961)))))


def security_scenarios():
    header = 'event_id,timestamp,user_id,device_id,latitude,longitude,requests,failures,label,summary'.split(',')
    catalogue = [
        ('sample', 'Compact reference', 'Normal use, approved backup and an isolated failure burst', 150, .8, 3, 'campus'),
        ('campus', 'Campus operations', 'Repeated records, remote transfers and excluded time endpoints', 150, .8, 3, 'campus'),
        ('research', 'Research network', 'Mixed device groups and observations with unknown labels', 150, .8, 3, 'campus'),
        ('perimeter', 'Perimeter inspection', 'Corner, edge, interior and exterior observations', 110, .8, 3, 'event_corners'),
        ('authentication', 'Authentication bursts', 'Clustered failures, an interior burst and an approved backup', 120, .8, 3, 'campus'),
        ('movement', 'Device movement', 'Repeated device locations crossing the permitted boundary', 110, .8, 3, 'campus'),
        ('quiet', 'Low-activity telemetry', 'Zero-count observations and unknown-label evaluation', 150, .8, 3, 'campus'),
        ('windows', 'Time-window boundaries', 'Inclusive start, exclusive end and quoted identity filters', 150, .8, 3, 'campus'),
        ('crowd', 'Dense campus traffic', 'Overlapping observations and several activity-density groups', 100, .8, 3, 'campus'),
    ]
    groups = {name: [] for name, *_ in catalogue[3:]}

    def add(scenario, device, record, x, y, requests, failures, label='benign',
            summary='Synthetic campus activity', timestamp=None, user=None, identity=None):
        lat, lon = geographic(x, y)
        groups[scenario].append([f'{scenario}-{device:02}-{record}',
            timestamp if timestamp is not None else 1800000020 + record * 85 + device,
            user if user is not None else f'user-{device % 4:02}',
            identity if identity is not None else f'device-{device:02}',
            f'{lat:.12f}', f'{lon:.12f}', requests, failures, label, summary])

    positions = [(-300,-250),(350,-250),(350,300),(-300,300),
                 (0,-250),(350,0),(0,300),(-300,0),
                 (-299.5,0),(349.5,0),(0,299.5),(0,-249.5),
                 (-300.5,0),(350.5,0),(0,300.5),(0,-250.5)]
    for i, (x,y) in enumerate(positions):
        add('perimeter',i,0,x,y,100,0 if i<12 else 40,
            'benign' if i<13 else 'suspicious',
            'Fence corner or edge' if i<8 else 'Near-boundary classification')
    for device in range(12):
        for record in range(3):
            burst = 8 <= device < 11
            x,y = (-160 + device*17, 120 + record*9)
            if device in (9,10,11): x,y = 520 + device*8, 100 + record*20
            add('authentication',device,record,x,y,
                100 if burst else 1800 if device==11 else 65+device,
                70 if burst else 0, 'suspicious' if burst else 'benign',
                'Repeated failed authentication' if burst else 'Approved backup' if device==11 else 'Normal portal use')
    for device in range(8):
        for record in range(6):
            x = -210 + record*125 + (device%4)*7
            y = -160 + (device//4)*270 + device*4
            add('movement',device,record,x,y,85+device*3,2,
                'suspicious' if device==7 and record>=4 else 'unknown' if device==6 else 'benign',
                'Mobile device trajectory across the permitted region',timestamp=1800000000+record*100+device)
    for device in range(9):
        add('quiet',device,0,0 if device<8 else 680,0 if device<8 else 390,
            0 if device<8 else 2000,0,'unknown','Unlabelled low-activity observation')
    for device in range(12):
        for record in range(2):
            add('windows',device,record,-100+device*15,80+record*10,
                40 if record==0 else 80,0,timestamp=1800000000+(599 if record else 0),
                user='lab, "night"' if device==0 else None,
                identity='device,"edge"' if device==0 else None,
                summary='Quoted identity, exact filter and fixed-window rate')
    for record,timestamp in enumerate((1799999999,1800000600,1800000601),2):
        add('windows',0,record,0,0,999,0,timestamp=timestamp,
            user='lab, "night"',identity='device,"edge"',summary='Excluded time endpoint')
    for device in range(64):
        for record in range(2):
            group=device%3
            add('crowd',device,record,-190+group*165+(device%4)*4,
                -110+group*130+(device%5)*4,
                (85+device%6) if group==0 else (340+device%6) if group==1 else (600+device%6),
                0 if group!=2 else 100+device%4,
                'suspicious' if device>=60 else 'unknown' if device==59 else 'benign',
                'Dense simultaneous campus traffic', timestamp=1800000100+record*120)
    for name, rows in groups.items():
        with (ROOT/'data'/f'security_{name}.csv').open('w',newline='',encoding='utf-8') as out:
            writer=csv.writer(out);writer.writerow(header);writer.writerows(rows)
    with (ROOT/'data/security_samples.csv').open('w',newline='',encoding='utf-8') as out:
        writer=csv.writer(out)
        writer.writerow(['file','name','purpose','geo_radius','feature_radius','min_pts','fence'])
        for name,*details in catalogue:
            writer.writerow([f'security_{name}.csv',*details])


def main():
    header = 'event_id,timestamp,user_id,device_id,latitude,longitude,requests,failures,label,summary'.split(',')
    for scenario, devices, records in [('campus', 16, 3), ('research', 20, 2)]:
        with (ROOT / 'data' / f'security_{scenario}.csv').open('w', newline='') as out:
            writer = csv.writer(out)
            writer.writerow(header)
            for device in range(devices):
                group = device % 3
                x = -220 + group * 190 + (device // 3) * 14
                y = -130 + group * 110 + (device // 3) * 18
                label = 'benign'
                requests, failures = (100 + device * 2, device % 3) if group != 1 else (410 + device * 3, 25 + device)
                summary = ['Library learning portal', 'Scheduled research data transfer', 'Residence network session'][group]
                if device >= devices - 4:
                    x = 460 + (device % 4) * 90
                    y = 350 if device % 2 else -330
                    if device % 2:
                        label, requests, failures = 'suspicious', 550 + device * 20, 450 + device * 13
                        summary = 'Labelled repeated authentication failures at external gateway'
                    else:
                        requests, failures = 1500 + device * 37, 0
                        summary = 'Approved remote backup; high volume requires contextual review'
                if scenario == 'research' and device == 2:
                    label, summary = 'unknown', 'Unlabelled instrument telemetry for manual review'
                for record in range(records):
                    lat, lon = geographic(x + record * 22, y + record * 16)
                    writer.writerow([f'{scenario}-{device:02}-{record}',
                                     1800000030 + record * 170 + device * 3,
                                     f'user-{device % 5:02}', f'device-{device:02}',
                                     f'{lat:.10f}', f'{lon:.10f}', requests + record * 7,
                                     failures, label, summary])
            # End-exclusive timestamp and an earlier event test the common window.
            lat, lon = geographic(0, 0)
            for suffix, timestamp in [('before', 1799999999), ('after', 1800000600)]:
                writer.writerow([f'{scenario}-{suffix}', timestamp, 'user-00', 'device-00',
                                 lat, lon, 10, 0, 'benign', 'Outside the default analysis window'])
    # All footprints are axis-aligned rectangles, so their CSV vertices can also
    # be filled by the presentation layer without a polygon-tessellation library.
    shapes = {
        'Campus boundary': [(-400, -350), (450, -350), (450, 400), (-400, 400), (-400, -350)],
        'Main avenue': [(-490, 0), (900, 0)],
        'North road': [(-400, 250), (500, 250), (730, 390)],
        'Service lane': [(0, -400), (0, 450)],
    }
    rectangles = {
        'Library': (-310, 80, -140, 190), 'Research labs': (100, 85, 300, 210),
        'Lecture halls': (-300, -260, -95, -145), 'Residence': (100, -270, 300, -150),
        'Garden': (-330, 280, -100, 365), 'Sports field': (100, 275, 365, 375),
        'Pond': (-355, -105, -230, -25), 'Computing': (90, -105, 290, -30),
        'Remote gateway': (610, 350, 760, 490),
    }
    for name, (x0, y0, x1, y1) in rectangles.items():
        shapes[name] = [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)]
    with (ROOT / 'data/campus_map.csv').open('w', newline='') as out:
        writer = csv.writer(out)
        writer.writerow(['name', 'latitude', 'longitude'])
        for name, points in shapes.items():
            for x, y in points:
                lat, lon = geographic(x, y)
                writer.writerow([name, f'{lat:.10f}', f'{lon:.10f}'])
    security_scenarios()


if __name__ == '__main__':
    main()
