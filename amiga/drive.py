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
subprocess.run('pkill -9 -x fs-uae; pkill -9 -x Xvfb; rm -f /tmp/.X96-lock /tmp/.X11-unix/X96', shell=True)
time.sleep(1)
def sh(cmd, **kw): return subprocess.run(cmd, shell=True, env=env, **kw)
shutil.rmtree(W, ignore_errors=True); os.makedirs(W + '/hd/S')
for f in os.listdir(DATA):
    if f.upper().endswith('.GLB') or f.upper() == 'FILE_ID.DIZ': shutil.copy(os.path.join(DATA, f), W + '/hd/' + f)
ini = open(DATA + '/SETUP.INI').read().replace('CardType=5', 'CardType=' + os.environ.get('CARD', '0')).replace('Volume=85', 'Volume=' + os.environ.get('VOL', '0'))
open(W + '/hd/SETUP.INI', 'w').write(ini)
shutil.copy(os.environ.get('RAPTOR_BIN', D + '/build/raptor'), W + '/hd/raptor')
if os.environ.get('MUSICDIR'): shutil.copytree(os.environ['MUSICDIR'], W + '/hd/MUSIC')
if os.environ.get('INJECT'): shutil.copy(os.environ['INJECT'], W + '/hd/inject.txt'); open(W + '/hd/S/startup-sequence', 'w').write('raptor\n')
open(W + '/test.fs-uae', 'w').write(f'''[fs-uae]
amiga_model = {os.environ.get('AMODEL', 'A1200')}
kickstart_file = {KICK}
{'' if os.environ.get('AMODEL') else 'fast_memory = 8192'}
{('cpu = 68030' if os.environ.get('CPU') == '030' else ('cpu = ' + os.environ['CPU'] if os.environ.get('CPU') else ''))}
{os.environ.get('EXTRACFG', '').replace(';', chr(10))}
hard_drive_0 = {W}/hd
hard_drive_0_label = SYS
window_width = 640
window_height = 512
sound_output = 0
automatic_input_grab = {os.environ.get('GRAB','1')}
fullscreen = 0
{'' if os.environ.get('ACC') == 'default' else 'accuracy = ' + os.environ.get('ACC', '-1')}
console_debugger = 1
{'' if os.environ.get('CPUSPEED') == 'default' else 'cpu_speed = ' + os.environ.get('CPUSPEED', 'max')}
joystick_port_0 = none
joystick_port_1 = none
''')
xv = subprocess.Popen('Xvfb :96 -screen 0 800x600x24', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
for _ in range(30):
    if subprocess.run('xdpyinfo -display :96', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0: break
    time.sleep(1)
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
