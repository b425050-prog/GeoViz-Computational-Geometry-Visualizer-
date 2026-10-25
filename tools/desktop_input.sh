#!/usr/bin/env bash
set -euo pipefail
# Real OS input under Xvfb. Uses an isolated copy, never a user's recovery directory.
app="$(realpath "$1")"
base="$(realpath "$2")"
"$app" --root "$base" > "$base/output/INPUT_APP_LOG.txt" 2>&1 &
pid=$!
trap 'kill "$pid" 2>/dev/null || true' EXIT
window=$(timeout 20 xdotool search --sync --name 'GeoViz | Geometry' | head -n1)
xdotool windowfocus "$window"
xdotool key --window "$window" F2
sleep .2
xdotool key --window "$window" 2
sleep .2
xdotool key --window "$window" s
sleep .3
cp "$base/output/session.scene" "$base/output/input_before.scene"
xdotool mousemove --window "$window" 400 300 click 1
sleep .2
xdotool key --window "$window" s
sleep .2
cp "$base/output/session.scene" "$base/output/input_added.scene"
xdotool mousemove --window "$window" 400 300 mousedown 1
sleep .2
xdotool mousemove --window "$window" 450 340
sleep .2
xdotool mouseup 1
sleep .2
xdotool key --window "$window" Return Right space space
xdotool key --window "$window" s
sleep .2
cp "$base/output/session.scene" "$base/output/input_dragged.scene"
xdotool key --window "$window" ctrl+z
sleep .2
xdotool key --window "$window" s
sleep .2
cp "$base/output/session.scene" "$base/output/input_undo.scene"
xdotool key --window "$window" ctrl+y
sleep .2
xdotool key --window "$window" F3 F3 F3
xdotool key --window "$window" Tab
xdotool key --window "$window" s
sleep .3
python3 - "$base" <<'PY'
from pathlib import Path
import sys
root=Path(sys.argv[1]);before=(root/'output/input_before.scene').read_text().split();after=(root/'output/session.scene').read_text().split()
assert int(after[2])==int(before[2])+1,'Physical click must add exactly one point'
n=int(after[2]);x,y=map(float,after[3+2*(n-1):3+2*n]);assert x!=0 or y!=0
assert (root/'output/recovery.scene').is_file(),'Recovery should follow edit'
assert (root/'output/input_undo.scene').read_bytes()==(root/'output/input_added.scene').read_bytes(),'Ctrl+Z must undo one complete drag'
assert (root/'output/input_dragged.scene').read_bytes()==(root/'output/session.scene').read_bytes(),'Ctrl+Y and workspace roundtrip must restore the dragged geometry'
assert (root/'output/input_added.scene').read_bytes()!=(root/'output/input_dragged.scene').read_bytes(),'Physical drag must move the point'
(root/'output/INPUT_SMOKE_REPORT.txt').write_text('PASS: OS sample selection, click/add, drag, run/play/step, undo/redo, workspace roundtrip, Tab focus, save/recovery.\n')
PY
xdotool key --window "$window" ctrl+q
timeout 10 tail --pid="$pid" -f /dev/null
wait "$pid"
trap - EXIT
