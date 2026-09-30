"""Exercise real Amiga Installer operations; replace only requester answers.
Local OS binaries are test inputs and never packaged. Each run owns its emulator.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from installer_icon import installer_icon,readme_icon
from test_install import expected_files
def replace_form(text,start,replacement):
    a=text.index(start);depth=0;quoted=False;i=a
    while i<len(text):
        c=text[i]
        if quoted and c=='\\':i+=2;continue
        if c=='"':quoted=not quoted
        elif not quoted:
            if c=='(':depth+=1
            elif c==')':
                depth-=1
                if depth==0:return text[:a]+replacement+text[i+1:]
        i+=1
    raise ValueError('Unbalanced Installer form')
def main():
    installer=Path(sys.argv[1]).resolve();whd='--whd' in sys.argv;keep='--keep' in sys.argv
    reinstall='--reinstall' in sys.argv
    base=Path(tempfile.mkdtemp(prefix='installer-script-',dir=ROOT/'tmp'))
    boot=base/'boot';(boot/'s').mkdir(parents=True);(base/'state').mkdir();(base/'out').mkdir();(base/'scratch').mkdir()
    dest=base/'out/Slicks';expected=expected_files()
    if keep or reinstall:
        (dest/'data').mkdir(parents=True)
        for name,data in expected.items():
            p=dest/'data'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        (dest/'data/SLICKS.CFG').write_bytes(b'keep settings')
        (dest/'data/SLICKS.REK').write_bytes(b'private placeholder, not a key')
        (dest/'data/TRACKS/BASIC.SS').write_bytes(b'user modified track and records')
        if keep:expected['TRACKS/BASIC.SS']=b'user modified track and records'
    for source,name in ((installer,'Installer'),(ROOT/'build/install-data/SlicksInstallData.exe','SlicksInstallData'),
      (ROOT/'amiga/out/SlicksDiag.exe','Slicks'),(ROOT/'build/whdload/Slicks.slave','Slicks.slave'),
      (Path.home()/'.local/share/amiga/WHDLoad/C/WHDLoad','WHDLoad'),(ROOT/'release/Play','Play'),(ROOT/'release/ReadMe','ReadMe')):
        shutil.copyfile(source,boot/name)
    (boot/'Install.info').write_bytes(installer_icon());(boot/'Slicks.inf').write_bytes(installer_icon(game=True));(boot/'ReadMe.info').write_bytes(readme_icon())
    script=(ROOT/'release/Install').read_text()
    if '--package' in sys.argv:
        package=Path(sys.argv[sys.argv.index('--package')+1]).resolve()
        for name in ('Slicks','SlicksInstallData','Slicks.slave','Slicks.inf','Install.info','ReadMe.info','Play','ReadMe','Install'):
            data=subprocess.run(['lha','pq',str(package),'Slicks Install/'+name],check=True,capture_output=True).stdout
            (boot/name).write_bytes(data)
        script=(boot/'Install').read_text()
    script=replace_form(script,'(welcome)','(if 0 (welcome))')
    script=replace_form(script,'(set #whd',f'(set #whd {int(whd)})')
    if '(message "The WHDLoad icon needs' in script:
        # The fixture has no Kickstart image: prove the warning is reached
        # without blocking on its requester.
        script=replace_form(script,'(message "The WHDLoad icon needs','(set #kick-warned 1)')
    script=replace_form(script,'(set #parent','(set #parent "DH2:out")')
    script=replace_form(script,'(set #archive','(set #archive "DH1:tmp/Slix151-release.zip")')
    script=replace_form(script,'(set #temp','(set #temp "DH2:scratch")')
    script=replace_form(script,'(set #install-data\n    (askbool',f'(set #install-data {int(reinstall)})')
    script=replace_form(script,'(exit)','(exit (quiet))')
    (boot/'Install').write_text(script)
    (boot/'s/startup-sequence').write_text('CD DH0:\nStack 16384\nDF0:C/Assign C: DF0:C\nDF0:C/Assign LIBS: DF0:Libs\nPath DH0: ADD\nC:LoadWB\nInstaller SCRIPT DH0:Install APPNAME Slicks MINUSER NOVICE DEFUSER NOVICE LOGFILE DH2:installer.log NOPRETEND >DH2:console.log\nEcho done >DH2:finished\n')
    print('Fixture:',base,flush=True)
    with (base/'emulator.log').open('w') as log:
        emu=subprocess.Popen(['fs-uae','--amiga_model=A1200','--chip_memory=2048','--fast_memory=0',
          '--kickstart_file='+os.environ['KICKSTART'],'--hard_drive_0='+str(boot),'--hard_drive_0_priority=10',
          '--hard_drive_1='+str(ROOT),'--hard_drive_2='+str(base),'--floppy_drive_0='+str(Path.home()/'Documents/Vette/tmp/Workbenchv2.04rev37.67Workbench.adf'),
          '--warp_mode=1','--fullscreen=0','--state_dir='+str(base/'state')],stdout=log,stderr=log,env=dict(os.environ,SDL_AUDIODRIVER='dummy'))
        try:
            deadline=time.monotonic()+180
            while time.monotonic()<deadline and not (base/'finished').exists():
                if emu.poll() is not None:raise RuntimeError('Emulator exited')
                time.sleep(.25)
            assert (base/'finished').exists(),'Installer did not finish: '+str(base)
            report=(base/'console.log').read_text(errors='replace')
            for name,data in expected.items():assert (dest/'data'/name).read_bytes()==data,name
            assert (dest/'data/Slicks').read_bytes()==(boot/'Slicks').read_bytes(),report
            assert (dest/'Play').read_bytes()==(boot/'Play').read_bytes()
            assert b'C:IconX\0' in (dest/'Play.info').read_bytes()
            assert (dest/'SlicksWHDLoad.info').exists()==whd
            if whd:assert b'Slicks.slave\0' in (dest/'SlicksWHDLoad.info').read_bytes()
            if keep or reinstall:
                assert (dest/'data/SLICKS.CFG').read_bytes()==b'keep settings'
                assert (dest/'data/SLICKS.REK').read_bytes()==b'private placeholder, not a key'
            assert not list((base/'scratch').glob('.slicks-data-*'))
            print(f'PASS: real Installer, 2 MiB/no Fast, WHDLoad={whd}, keep={keep}, reinstall={reinstall}; expected data, executable, icons and cleanup')
        finally:
            emu.terminate()
            try:emu.wait(timeout=5)
            except subprocess.TimeoutExpired:emu.kill();emu.wait()
if __name__=='__main__':main()
