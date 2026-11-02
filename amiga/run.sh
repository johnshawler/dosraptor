#!/bin/bash
# usage: run.sh [seconds] [outprefix]  -- boots build/raptor in FS-UAE (Xvfb), game data from $RAPTOR_DATA, Kickstart from $KICKSTART
SECS=${1:-40}; OUT=${2:-shot}
D=$(cd "$(dirname "$0")"; pwd)
: "${KICKSTART:=/root/.claude/uploads/387e5900-dc29-591a-9a5a-61a50108a5bd/d049846c-Kickstart-3.1-A1200-40.68.rom}"
: "${RAPTOR_DATA:=/tmp/claude-0/-home-user/387e5900-dc29-591a-9a5a-61a50108a5bd/scratchpad/rz/raptor}"
W=${WORK:-/tmp/raptor-hd}
rm -rf $W; mkdir -p $W/hd/S
cp "$RAPTOR_DATA"/*.GLB "$RAPTOR_DATA"/SETUP.INI $W/hd/ 2>/dev/null
cp $D/build/raptor $W/hd/raptor; printf 'raptor\n' > $W/hd/S/startup-sequence
cat > $W/test.fs-uae <<EOF
[fs-uae]
amiga_model = A1200
kickstart_file = $KICKSTART
fast_memory = 8192
hard_drive_0 = $W/hd
hard_drive_0_label = SYS
window_width = 640
window_height = 512
sound_output = 0
automatic_input_grab = 0
fullscreen = 0
EOF
export DISPLAY=:97
Xvfb :97 -screen 0 800x600x24 >/dev/null 2>&1 &
XP=$!; sleep 1
timeout $((SECS+10)) fs-uae $W/test.fs-uae > $W/fsuae.log 2>&1 &
FP=$!
for t in $(seq 5 5 $SECS); do sleep 5; import -window root $W/${OUT}_$t.png 2>/dev/null; done
kill $FP 2>/dev/null; sleep 1; kill $XP 2>/dev/null
ls $W/hd; cat $W/hd/dbg.log 2>/dev/null | head -50
