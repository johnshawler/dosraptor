#!/bin/sh
# usage: mkhdf.sh out.hdf  (needs: pip install amitools; build/raptor; $RAPTOR_DATA with FILE000x.GLB + SETUP.INI)
set -e
OUT=${1:-build/raptor.hdf}; D=$(cd "$(dirname "$0")"; pwd); DATA=${RAPTOR_DATA:?set RAPTOR_DATA}
rm -f "$OUT"; T=$(mktemp -d)
sed "s/CardType=5/CardType=${CARD:-5}/; s/Volume=85/Volume=${VOL:-85}/" "$DATA/SETUP.INI" > $T/SETUP.INI
V=$(git -C "$D" rev-parse --short HEAD 2>/dev/null || echo dev)
sed -i "0,/^Fire=29/s//Fire=${FIREKEY:-30}/" $T/SETUP.INI   # keyboard fire: PC scan code 30 (0x1e) = the A key (was 29 = Ctrl)
printf 'echo "Raptor A1200 build %s"\nraptor\n' "$V" > $T/startup-sequence
python3 "$D/tools/mus2pcm.py" "$DATA" $T/music > /dev/null
rdbtool "$OUT" create size=${SIZE:-40Mi} + init + add size=100% name=DH0 dostype=DOS1 bootable=True > /dev/null
[ -n "$INJECT" ] && cp "$INJECT" $T/inject.txt
CMDS="open part=DH0 + format Raptor ffs + makedir S + write $T/startup-sequence S/startup-sequence + write $D/build/raptor raptor + write $T/SETUP.INI SETUP.INI"
[ -n "$INJECT" ] && CMDS="$CMDS + write $T/inject.txt inject.txt"
CMDS="$CMDS + makedir MUSIC"
for f in $T/music/*; do CMDS="$CMDS + write $f MUSIC/$(basename $f)"; done
[ -e "$DATA/FILE_ID.DIZ" ] && CMDS="$CMDS + write $DATA/FILE_ID.DIZ FILE_ID.DIZ"
for f in "$DATA"/*.GLB; do CMDS="$CMDS + write $f $(basename $f)"; done
xdftool "$OUT" $CMDS > /dev/null
rm -rf $T; ls -la "$OUT"
