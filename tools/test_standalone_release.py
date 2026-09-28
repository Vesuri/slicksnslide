"""Verify installed Play, or directly launch on the default 4 KiB CLI stack.

Use a disposable installed directory for --default-stack scripted checks:
the game really saves profiles, records and championships there.
"""
import os
import argparse
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('installed',type=Path)
    parser.add_argument('--default-stack',action='store_true')
    parser.add_argument('--args',default='')
    parser.add_argument('--checks',type=Path)
    parser.add_argument('--marker',default='STANDALONE_RELEASE_TITLE_OK')
    args=parser.parse_args()
    if (args.args or args.checks) and not args.default_stack:
        parser.error('--args and --checks require --default-stack')
    installed=args.installed.resolve()
    base=Path(tempfile.mkdtemp(prefix='standalone-release-',dir=ROOT/'tmp'))
    boot=base/'boot';(boot/'s').mkdir(parents=True);(boot/'c').mkdir();(base/'state').mkdir();(base/'home').mkdir()
    shutil.copyfile(ROOT/'tmp/SetPatch',boot/'c/SetPatch')
    launch='Execute Play'
    if args.default_stack:
        # Bypass Play's explicit stack increase. The caller supplies a private
        # fixture and can reuse it for save/restart verification.
        launch='C:SetPatch QUIET\nCD data\nSlicks '+args.args
    (boot/'s/startup-sequence').write_text('DF0:C/Assign C: DH0:c\nDF0:C/Assign C: DF0:C ADD\nDF0:C/Assign LIBS: DF0:Libs\nCD DH1:\n'+launch+'\n')
    sock=socket.socket();sock.bind(('127.0.0.1',0));port=sock.getsockname()[1];sock.close()
    script=f'''set pagination off
set confirm off
target remote 127.0.0.1:{port}
break slicks_amiga_platform_wait_vblank if g_slicks_diag_ready == 1
continue
if g_slicks_diag_ingame || !g_slicks_diag_profile_platform->active || g_slicks_registration_status != 0
  quit 1
end
printf "STANDALONE_RELEASE_TITLE_OK\\n"
detach
quit
'''
    if args.default_stack:
        prefix=f'''set pagination off
set confirm off
target remote 127.0.0.1:{port}
tbreak *main
continue
# ExecBase.ThisTask and Task.tc_SPLower/tc_SPUpper (standard 32-bit ABI).
set $task = *(unsigned int *)(*(unsigned int *)4 + 276)
set $lower = *(unsigned int *)($task + 58)
set $upper = *(unsigned int *)($task + 62)
if $upper - $lower != 4096 || $sp <= $lower+64 || $sp > $upper
  printf "DEFAULT_STACK_BOUNDS_FAILED\\n"
  quit 1
end
printf "DEFAULT_STACK_CONFIRMED bytes=4096\\n"
'''
        if args.checks:
            checks=args.checks.resolve().read_text()
            # Existing checks dump only beneath .run; isolate those outputs.
            import re
            for name in re.findall(r'dump binary memory (\.run/\S+)',checks):
                (base/name).parent.mkdir(parents=True,exist_ok=True)
            script=prefix+checks
        else:
            script=prefix+script.split(f'target remote 127.0.0.1:{port}\n',1)[1].replace('detach\n','')
    (base/'test.gdb').write_text(script)
    env=dict(os.environ,SDL_AUDIODRIVER='dummy',HOME=str(base/'home'),XDG_CACHE_HOME=str(base/'home'))
    with (base/'emulator.log').open('w') as log:
        emu=subprocess.Popen(['fs-uae','--amiga_model=A1200','--chip_memory=2048','--fast_memory=0',
          '--kickstart_file='+os.environ['KICKSTART'],'--hard_drive_0='+str(boot),'--hard_drive_0_priority=10','--hard_drive_1='+str(installed),
          '--floppy_drive_0='+str(Path.home()/'Documents/Vette/tmp/Workbenchv2.04rev37.67Workbench.adf'),
          '--remote_debugger=20','--remote_debugger_port='+str(port),'--remote_debugger_trigger=Slicks',
          '--warp_mode=1','--fullscreen=0','--state_dir='+str(base/'state')],stdout=log,stderr=log,env=env)
        try:
            for _ in range(150):
                if emu.poll() is not None:raise RuntimeError('Emulator exited')
                if subprocess.run(['lsof','-nP',f'-iTCP:{port}','-sTCP:LISTEN'],capture_output=True).returncode==0:break
                time.sleep(.2)
            with (base/'debug.log').open('w') as debug:
                result=subprocess.run(['m68k-amiga-elf-gdb','-q','-batch','-x',str(base/'test.gdb'),str(ROOT/'amiga/out/SlicksDiag.elf')],stdout=debug,stderr=debug,env=env,cwd=base,timeout=600)
            output=(base/'debug.log').read_text()
            assert result.returncode==0 and args.marker in output,str(base)
            if args.default_stack:
                assert 'DEFAULT_STACK_CONFIRMED bytes=4096' in output,str(base)
            print('PASS:',args.marker,'on 2 MiB/no-Fast A1200;',base)
        finally:
            emu.terminate()
            try:emu.wait(timeout=5)
            except subprocess.TimeoutExpired:emu.kill();emu.wait()
if __name__=='__main__':main()
