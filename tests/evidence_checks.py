"""Validate structured evidence and unsigned consistency hashes from adjacent snapshots."""
from pathlib import Path
import json,sys,shlex
def reject_constant(value):
    raise ValueError('Non-finite JSON constant: ' + value)
root=Path(sys.argv[1])
count=0
for path in root.rglob('*.txt.json'):
    doc=json.loads(path.read_text(encoding='utf-8'), parse_constant=reject_constant)
    if doc.get('schema')!='geoviz.evidence.v1':continue
    assert doc['input_hash'].startswith('fnv1a64:')
    snapshot=Path(str(path)[:-5]+'.security')
    tokens=shlex.split(snapshot.read_text(encoding='utf-8'),comments=False,posix=True)
    index=17;fence_count=int(tokens[index]);index+=1+fence_count*2
    event_count=int(tokens[index]);index+=1
    canonical=''
    quote=lambda v:'"'+v.replace(chr(92),chr(92)*2).replace('"',chr(92)+'"')+'"'
    for _ in range(event_count):
        v=tokens[index:index+10];index+=10
        canonical+=quote(v[0])+str(int(v[1]))+quote(v[2])+quote(v[3])+format(float(v[4]),'.17g')+','+format(float(v[5]),'.17g')+','+str(int(v[6]))+','+str(int(v[7]))+quote(v[8])+quote(v[9])
    digest=14695981039346656037
    for byte in canonical.encode('utf-8'):digest=((digest^byte)*1099511628211)&((1<<64)-1)
    assert doc['input_hash']==f'fnv1a64:{digest:016x}',f'Canonical snapshot hash mismatch: {path}'
    assert len(doc['views'])==2
    for view in doc['views']:
        assert 'REFERENCE-VERIFIED' in view['check']
        for observation in view['observations']:
            for event in observation['sources']:
                assert event['candidate']==(event['feature_noise'] or event['outside_region'])
    count+=1
assert count,'No structured evidence found'
print(f'PASS: {count} structured evidence generations; schema, finite JSON, snapshot hashes, reference checks and candidate reasons.')
