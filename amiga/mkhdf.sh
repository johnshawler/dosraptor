#!/bin/sh
# usage: mkhdf.sh out.hdf  (needs: pip install amitools; build/raptor; $RAPTOR_DATA with FILE000x.GLB + SETUP.INI)
set -e
OUT=${1:-build/raptor.hdf}; D=$(cd "$(dirname "$0")"; pwd); DATA=${RAPTOR_DATA:?set RAPTOR_DATA}
rm -f "$OUT"; T=$(mktemp -d)
sed "s/CardType=5/CardType=${CARD:-5}/; s/Volume=85/Volume=${VOL:-85}/" "$DATA/SETUP.INI" > $T/SETUP.INI
printf 'raptor\n' > $T/startup-sequence
rdbtool "$OUT" create size=${SIZE:-24Mi} + init + add size=100% name=DH0 dostype=DOS1 bootable=True > /dev/null
CMDS="open part=DH0 + format Raptor ffs + makedir S + write $T/startup-sequence S/startup-sequence + write $D/build/raptor raptor + write $T/SETUP.INI SETUP.INI"
for f in "$DATA"/*.GLB; do CMDS="$CMDS + write $f $(basename $f)"; done
xdftool "$OUT" $CMDS > /dev/null
rm -rf $T; ls -la "$OUT"
