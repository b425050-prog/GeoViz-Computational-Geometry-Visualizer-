"""Bind a built binary to its source revision and recorded toolchain."""
from pathlib import Path
import json,hashlib,subprocess,argparse,platform
root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--compiler',default='g++');args=parser.parse_args()
def run(*cmd):return subprocess.check_output(cmd,cwd=root,text=True).strip()
record={'schema':'geoviz.build.v1','version':'2.0.0','source_revision':run('git','rev-parse','HEAD'),'source_dirty':bool(run('git','status','--porcelain','--untracked-files=no')),'compiler':run(args.compiler,'--version').splitlines()[0],'cmake':run('cmake','--version').splitlines()[0],'raylib_revision':'c1ab645ca298a2801097931d1079b10ff7eb9df8','platform':platform.platform(),'binary_sha256':hashlib.sha256((root/'GeoViz.exe').read_bytes()).hexdigest()}
(root/'BUILD_PROVENANCE.json').write_text(json.dumps(record,indent=2)+'\n')
print('Recorded binary provenance for '+record['source_revision'])
