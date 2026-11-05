#!/bin/sh
S=/tmp/claude-0/-home-user/387e5900-dc29-591a-9a5a-61a50108a5bd/scratchpad
cd /home/user/dosraptor/amiga
for n in ${EPS:-tango outer}; do
  printf 'wait=STATE GAME 900\nsleep=45\nshot=ep_%s\nend=1\n' $n > /tmp/t_ep_$n.txt
  RAPTOR_DATA=$S/regdata MUSICDIR=$S/music_reg CARD=5 VOL=85 CPU=030 ACC=0 CPUSPEED=real INJECT=/tmp/in_$n.txt python3 drive.py /tmp/t_ep_$n.txt > /tmp/ep_$n.out 2>&1
  cp /tmp/raptor-hd/hd/dbg.log /tmp/ep_$n.log; cp /tmp/raptor-hd/ep_$n.png /tmp/ep_$n.png
done
