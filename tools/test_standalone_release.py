"""Boot an installed Play script unchanged and verify native title takeover."""
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile
import time
ROOT=Path(__file__).resolve().parents[1]
def main():
    installed=Path(sys.argv[1]).resolve()
    base=Path(tempfile.mkdtemp(prefix='standalone-release-',dir=ROOT/'tmp'))
    boot=base/'boot';(boot/'s').mkdir(parents=True);(boot/'c').mkdir();(base/'state').mkdir();(base/'home').mkdir()
    shutil.copyfile(ROOT/'tmp/SetPatch',boot/'c/SetPatch')
    (boot/'s/startup-sequence').write_text('DF0:C/Assign C: DH0:c\nDF0:C/Assign C: DF0:C ADD\nDF0:C/Assign LIBS: DF0:Libs\nCD DH1:\nExecute Play\n')
    sock=socket.socket();sock.bind(('127.0.0.1',0));port=sock.getsockname()[1];sock.close()
    (base/'test.gdb').write_text(f'''set pagination off
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
''')
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
                result=subprocess.run(['m68k-amiga-elf-gdb','-q','-batch','-x',str(base/'test.gdb'),str(ROOT/'amiga/out/SlicksDiag.elf')],stdout=debug,stderr=debug,env=env,timeout=120)
            assert result.returncode==0 and 'STANDALONE_RELEASE_TITLE_OK' in (base/'debug.log').read_text(),str(base)
            print('PASS: installed Play script reaches native title on 2 MiB/no-Fast A1200;',base)
        finally:
            emu.terminate()
            try:emu.wait(timeout=5)
            except subprocess.TimeoutExpired:emu.kill();emu.wait()
if __name__=='__main__':main()
