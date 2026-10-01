#!/usr/bin/env python3
"""Run isolated WHDLoad tests and retain its own core dumps under tmp/.

Source amiga/env.sh first. ROMs and original data are local inputs, never shipped.
All modes use the production game. Quit/race/save tests use diagnostic-only slave arguments;
timed uses the production slave. No test slave is packaged.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=('smoke', 'boot', 'load', 'quit', 'timed','race','championship','championship-edit','records'), default='quit')
    p.add_argument('--seed-save', type=Path, help='Previous test E2E.SSS for overwrite/restart validation')
    p.add_argument('--whdload', type=Path, default=Path.home()/'.local/share/amiga/WHDLoad/C/WHDLoad')
    p.add_argument('--rom', type=Path)
    p.add_argument('--rtb', type=Path)
    p.add_argument('--exe', type=Path, default=ROOT/'amiga/out/SlicksDiag.exe')
    p.add_argument('--seconds', type=int, default=90, help='host safety ceiling')
    p.add_argument('--ticks', type=int, default=1500, help='WHDLoad timeout in PAL fields')
    p.add_argument('--cpu', default='68020')
    p.add_argument('--fast',type=int,default=4096,help='Fast RAM in KiB')
    p.add_argument('--no-preload', action='store_true')
    p.add_argument('--no-write-cache', action='store_true', help='Diagnostic control for cached file creation')
    p.add_argument('--write-delay', type=int, help='WHDLoad physical-write delay in PAL fields')
    args = p.parse_args()
    if args.mode != 'smoke' and (not args.rom or not args.rtb):
        p.error('--rom and --rtb are required except for smoke mode')
    if args.write_delay is not None and args.write_delay < 0:
        p.error('--write-delay must be nonnegative')
    if args.mode == 'championship-edit' and not args.seed_save:
        p.error('--seed-save is required for championship-edit')
    slave = {'smoke':'Smoke.slave', 'boot':'BootTest.slave', 'load':'LoadTest.slave','race':'RaceTest.slave','quit':'ExitTest.slave','championship':'ChampionshipTest.slave','championship-edit':'ChampionshipEditTest.slave','records':'RecordsTest.slave'}.get(args.mode, 'Slicks.slave')
    base = Path(tempfile.mkdtemp(prefix='whdload-test-', dir=ROOT/'tmp'))
    print('Fixture:', base, flush=True)
    boot, game = base/'boot', base/'game'
    for d in (boot/'s', boot/'devs/Kickstarts', game/'data', base/'state'):
        d.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.whdload, game/'WHDLoad')
    shutil.copyfile(ROOT/'build/whdload'/slave, game/'Slicks.slave')
    if args.mode != 'smoke':
        shutil.copyfile(args.rom, boot/'devs/Kickstarts'/args.rom.name)
        shutil.copyfile(args.rtb, boot/'devs/Kickstarts'/(args.rom.name+'.RTB'))
    if args.mode in ('load', 'quit', 'timed','race','championship','championship-edit','records'):
        shutil.copyfile(args.exe, game/'data/Slicks')
    if args.mode in ('quit', 'timed','race','championship','championship-edit','records'):
        with zipfile.ZipFile(ROOT/'tmp/Slix151-release.zip') as z:
            for name in z.namelist():
                if name in ('SLICKS.000','SLICKS.DAT') or (name.startswith('TRACKS/') and name.endswith('.SS')):
                    target=game/'data'/name; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(z.read(name))
    if args.seed_save:
        shutil.copyfile(args.seed_save, game/'data/E2E.SSS')
    (boot/'s/WHDLoad.prefs').write_text('Expert\nReadDelay=0\n')
    preload = '' if args.no_preload else 'PRELOAD '
    write_options = 'NOWRITECACHE ' if args.no_write_cache else ''
    if args.write_delay is not None:
        write_options += 'WRITEDELAY=%d ' % args.write_delay
    (boot/'s/startup-sequence').write_text(
        'DF0:C/Assign C: DF0:C\nDF0:C/Assign LIBS: DF0:Libs\n'
        'DF0:C/Assign DEVS: DH0:devs\nStack 16384\nFailAt 999\n'
        f'CD DH1:\nWHDLoad Slicks.slave {preload}{write_options}SPLASHDELAY=0 NOREQ COREDUMP FILELOG TIMEOUT={args.ticks} >DH0:result\n'
        'If WARN\nEcho failed >DH0:failed\nElse\nEcho passed >DH0:passed\nEndIf\n')
    with (base/'emulator.log').open('w') as log:
        emu = subprocess.Popen(['fs-uae', '--amiga_model=A1200', '--cpu='+args.cpu,
            '--uae_cpu_model='+args.cpu, '--uae_cpu_24bit_addressing=false',
            '--jit_compiler=0', '--chip_memory=2048', '--fast_memory='+str(args.fast),
            '--kickstart_file='+os.environ['KICKSTART'],
            '--hard_drive_0='+str(boot), '--hard_drive_0_priority=10', '--hard_drive_1='+str(game),
            '--floppy_drive_0='+str(Path.home()/'Documents/Vette/tmp/Workbenchv2.04rev37.67Workbench.adf'),
            '--joystick_port_0=mouse', '--joystick_port_1=nothing', '--warp_mode=1', '--fullscreen=0',
            '--window_width=720', '--window_height=568', '--state_dir='+str(base/'state')], stdout=log, stderr=log, env=dict(os.environ,SDL_AUDIODRIVER='dummy'))
        try:
            deadline = time.monotonic()+args.seconds
            while time.monotonic()<deadline and not any((boot/n).exists() for n in ('passed','failed')):
                if emu.poll() is not None:
                    raise RuntimeError('FS-UAE exited unexpectedly')
                time.sleep(.25)
            output = (boot/'result').read_text(errors='replace') if (boot/'result').exists() else ''
            report = (game/'.whdl_register').read_text(encoding='latin1') if (game/'.whdl_register').exists() else ''
            assert report, f'No WHDLoad core dump: {base}\n{output}'
            if args.mode in ('timed','race'):
                assert 'DEBUG caused.' in report, report + output
                files = (game/'.whdl_log').read_text(encoding='latin1')
                for name in (('SLICKS.000','SLICKS.DAT') if args.mode=='race' else ('SLICKS.000',)):
                    assert any('[ReadOff]' in line and 'name='+name in line for line in files.splitlines()), files
                print('PASS: timed run read original Slicks data under WHDLoad')
            else:
                assert (boot/'passed').exists() and 'Return OK.' in report, report + output
                if args.mode == 'smoke':
                    assert (game/'smoke-passed').read_bytes() == b'PASS'
                if args.mode == 'records':
                    files = (game/'.whdl_log').read_text(encoding='latin1')
                    assert any('[WritOff]' in line and 'name=TRACKS/' in line for line in files.splitlines()), 'No record write exercised'
                    assert (game/'data/SLICKS.CFG').is_file() and (game/'data/SLICKS.PLR').is_file()
                    with zipfile.ZipFile(ROOT/'tmp/Slix151-release.zip') as original:
                        assert any(p.read_bytes()!=original.read('TRACKS/'+p.name)
                                   for p in (game/'data/TRACKS').glob('*.SS')), 'No track records changed'
                    assert not list((game/'data').rglob('*.new'))
                    assert not list((game/'data').rglob('*.bak'))
                if args.mode in ('championship','championship-edit'):
                    assert (game/'data/E2E.SSS').is_file(), 'Championship save missing'
                    assert not list((game/'data').rglob('*.new')), 'Temporary save remains'
                    assert not list((game/'data').rglob('*.bak')), 'Save backup remains'
                    if args.mode == 'championship-edit':
                        assert not (game/'data/TEMP.SSS').exists(), 'Deleted test save remains'
                        files = (game/'.whdl_log').read_text(encoding='latin1')
                        for operation, name in (('[WritOff]', 'TEMP.SSS'), ('[WritOff]', 'E2E.SSS.bak'), ('[Delete]', 'TEMP.SSS')):
                            assert any(operation in line and 'name='+name in line for line in files.splitlines()), (operation,name)
                print(f'PASS: {args.mode} slave returned normally; WHDLoad core saved')
        finally:
            emu.terminate()
            try: emu.wait(timeout=5)
            except subprocess.TimeoutExpired: emu.kill(); emu.wait()

if __name__ == '__main__':
    main()
