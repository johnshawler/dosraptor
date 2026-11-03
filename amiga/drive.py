#!/usr/bin/env python3
"""Drive the Raptor Amiga build in FS-UAE (under Xvfb) with a small script.
   drive.py SCRIPT [--keep]    script lines:  wait=<regex in dbg.log> [timeout]   sleep=<s>   key=<xdotool key>   keys=<text>
                                              click   shot=<name>   mouse=<x> <y>   (screenshots land in $WORK/<name>.png)"""
import os, re, subprocess, sys, time, shutil
D = os.path.dirname(os.path.abspath(__file__))
W = os.environ.get('WORK', '/tmp/raptor-hd')
KICK = os.environ.get('KICKSTART', '/root/.claude/uploads/387e5900-dc29-591a-9a5a-61a50108a5bd/d049846c-Kickstart-3.1-A1200-40.68.rom')
DATA = os.environ.get('RAPTOR_DATA', '/tmp/claude-0/-home-user/387e5900-dc29-591a-9a5a-61a50108a5bd/scratchpad/rz/raptor')
env = dict(os.environ, DISPLAY=':96')
def sh(cmd, **kw): return subprocess.run(cmd, shell=True, env=env, **kw)
shutil.rmtree(W, ignore_errors=True); os.makedirs(W + '/hd/S')
for f in os.listdir(DATA):
    if f.upper().endswith('.GLB'): shutil.copy(os.path.join(DATA, f), W + '/hd/' + f)
ini = open(DATA + '/SETUP.INI').read().replace('CardType=5', 'CardType=0').replace('Volume=85', 'Volume=0')
open(W + '/hd/SETUP.INI', 'w').write(ini)
shutil.copy(D + '/build/raptor', W + '/hd/raptor')
if os.environ.get('INJECT'): shutil.copy(os.environ['INJECT'], W + '/hd/inject.txt'); open(W + '/hd/S/startup-sequence', 'w').write('raptor\n')
open(W + '/test.fs-uae', 'w').write(f'''[fs-uae]
amiga_model = A1200
kickstart_file = {KICK}
fast_memory = 8192
hard_drive_0 = {W}/hd
hard_drive_0_label = SYS
window_width = 640
window_height = 512
sound_output = 0
automatic_input_grab = {os.environ.get('GRAB','1')}
fullscreen = 0
accuracy = {os.environ.get('ACC', '-1')}
console_debugger = 1
cpu_speed = {os.environ.get('CPUSPEED', 'max')}
joystick_port_0 = none
joystick_port_1 = none
''')
xv = subprocess.Popen('Xvfb :96 -screen 0 800x600x24', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL); time.sleep(1)
fs = subprocess.Popen(f'fs-uae {W}/test.fs-uae', shell=True, env=env, stdin=subprocess.PIPE, stdout=open(W + '/fsuae.log', 'w'), stderr=subprocess.STDOUT)
dbg_active = False
t0 = time.time()
def log(): 
    try: return open(W + '/hd/dbg.log', errors='replace').read()
    except OSError: return ''
def shot(name): sh(f'import -window root {W}/{name}.png')
try:
    for line in open(sys.argv[1]):
        line = line.strip()
        if not line or line.startswith('#'): continue
        cmd, _, arg = line.partition('=')
        if cmd == 'sleep': time.sleep(float(arg))
        elif cmd == 'wait':
            pat, _, to = arg.rpartition(' ')
            if not to.isdigit(): pat, to = arg, '150'
            end = time.time() + float(to)
            while not re.search(pat, log()) and time.time() < end: time.sleep(1)
            print('%5.0fs wait %s -> %s' % (time.time() - t0, pat, 'ok' if re.search(pat, log()) else 'TIMEOUT'))
        elif cmd == 'key': sh(f'xdotool key --delay 120 {arg}')
        elif cmd == 'keys': sh(f'xdotool type --delay 150 "{arg}"')
        elif cmd == 'click': sh('xdotool click 1')
        elif cmd == 'mrel': sh(f'xdotool mousemove_relative -- {arg}')
        elif cmd == 'mouse': sh(f'xdotool mousemove {arg}')
        elif cmd == 'dbg':
            if not dbg_active:
                sh('xdotool keydown F12 key d keyup F12'); time.sleep(3); dbg_active = True
            fs.stdin.write((arg + '\n').encode()); fs.stdin.flush(); time.sleep(2)
        elif cmd == 'brk': sh('xdotool keydown F12 key d keyup F12'); time.sleep(3)
        elif cmd == 'shot': shot(arg)
        elif cmd == 'end': break
finally:
    shot('final')
    fs.terminate(); time.sleep(1); xv.terminate()
    print('log tail:'); print(''.join(log().splitlines(True)[-6:]))
