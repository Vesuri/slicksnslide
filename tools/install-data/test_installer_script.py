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
    installer=Path(sys.argv[1]).resolve();keep='--keep' in sys.argv
    reinstall='--reinstall' in sys.argv
    no_whd='--no-whd' in sys.argv
    incomplete='--missing-tracks' in sys.argv
    ram_temp='--ram-temp' in sys.argv
    base=Path(tempfile.mkdtemp(prefix='installer-script-',dir=ROOT/'tmp'))
    boot=base/'boot';(boot/'s').mkdir(parents=True);(base/'state').mkdir();(base/'out').mkdir();(base/'scratch').mkdir()
    dest=base/'out/Slicks';expected=expected_files()
    if keep or reinstall or incomplete:
        (dest/'data').mkdir(parents=True)
        for name,data in expected.items():
            if incomplete and name.startswith('TRACKS/'):continue
            p=dest/'data'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        (dest/'data/SLICKS.CFG').write_bytes(b'keep settings')
        (dest/'data/SLICKS.REK').write_bytes(b'private placeholder, not a key')
        (dest/'data/PLAYER.PLR').write_bytes(b'keep profile')
        (dest/'data/TEST.SSS').write_bytes(b'keep championship')
        if not incomplete:(dest/'data/TRACKS/CUSTOM.SS').write_bytes(b'keep custom track')
        (dest/'Play.info').write_bytes(b'legacy standalone icon')
        (dest/'Play').write_bytes(b'legacy standalone script')
        (dest/'SlicksWHDLoad.info').write_bytes(b'legacy WHDLoad icon')
        if not incomplete:(dest/'data/TRACKS/BASIC.SS').write_bytes(b'user modified track and records')
        if keep:expected['TRACKS/BASIC.SS']=b'user modified track and records'
    for source,name in ((installer,'Installer'),(ROOT/'build/install-data/SlicksInstallData.exe','SlicksInstallData'),
      (ROOT/'amiga/out/SlicksDiag.exe','Slicks'),(ROOT/'build/whdload/Slicks.slave','Slicks.slave'),
      (Path.home()/'.local/share/amiga/WHDLoad/C/WHDLoad','WHDLoad'),(ROOT/'release/ReadMe','ReadMe')):
        if not (no_whd and name=='WHDLoad'):shutil.copyfile(source,boot/name)
    (boot/'Install.info').write_bytes(installer_icon());(boot/'Slicks.inf').write_bytes(installer_icon(game=True));(boot/'ReadMe.info').write_bytes(readme_icon())
    script=(ROOT/'release/Install').read_text()
    if '--package' in sys.argv:
        package=Path(sys.argv[sys.argv.index('--package')+1]).resolve()
        for name in ('Slicks','SlicksInstallData','Slicks.slave','Slicks.inf','Install.info','ReadMe.info','ReadMe','Install'):
            data=subprocess.run(['lha','pq',str(package),'Slicks Install/'+name],check=True,capture_output=True).stdout
            (boot/name).write_bytes(data)
        script=(boot/'Install').read_text()
    script=replace_form(script,'(welcome)','(if 0 (welcome))')
    if no_whd:
        # A fatal requester needs a human dismissal. Keep the prerequisite
        # branch intact, recording its entry and exiting without that dialog.
        script=replace_form(script,'(abort "Install WHDLoad 17 or newer first',
            '((run "Echo missing-whd >DH2:missing-whd") (exit (quiet)))')
    assert '(set #whd' not in script and 'Install the optional WHDLoad' not in script
    if '(message "WHDLoad needs' in script:
        # The fixture has no Kickstart image: prove the warning is reached
        # without blocking on its requester.
        script=replace_form(script,'(message "WHDLoad needs','(set #kick-warned 1)')
    script=replace_form(script,'(set #parent','(set #parent "DH2:out")')
    script=replace_form(script,'(set #archive','(abort "Use existing unexpectedly asked for ZIP")' if keep else '(set #archive "DH1:tmp/Slix151-release.zip")')
    script=replace_form(script,'(set #temp','(abort "Use existing unexpectedly asked for scratch")' if keep else '(set #temp "'+('T:' if ram_temp else 'DH2:scratch')+'")')
    script=replace_form(script,'(set #install-data\n    (askbool',f'(set #install-data {int(reinstall)})')
    script=replace_form(script,'(exit)','(exit (quiet))')
    (boot/'Install').write_text(script)
    (boot/'s/startup-sequence').write_text('CD DH0:\nStack 16384\nDF0:C/Assign C: DF0:C\nDF0:C/Assign LIBS: DF0:Libs\nMakeDir RAM:T\nAssign T: RAM:T\nPath DH0: ADD\nC:LoadWB\nInstaller SCRIPT DH0:Install APPNAME Slicks MINUSER AVERAGE DEFUSER AVERAGE LOGFILE DH2:installer.log NOPRETEND >DH2:console.log\nList T: ALL >DH2:temp-after.log\nEcho done >DH2:finished\n')
    print('Fixture:',base,flush=True)
    with (base/'emulator.log').open('w') as log:
        emu=subprocess.Popen(['fs-uae','--audio_driver=dummy','--amiga_model=A1200','--chip_memory=2048','--fast_memory='+('4096' if ram_temp else '0'),
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
            if no_whd:
                assert (base/'missing-whd').exists(),report
                assert not (dest/'data/Slicks').exists()
                print('PASS: missing WHDLoad fails before installation')
                return
            for name,data in expected.items():assert (dest/'data'/name).read_bytes()==data,name
            assert (dest/'data/Slicks').read_bytes()==(boot/'Slicks').read_bytes(),report
            assert not (dest/'Play').exists()
            assert not (dest/'Play.info').exists()
            assert not (dest/'SlicksWHDLoad.info').exists()
            icon=(dest/'Slicks.info').read_bytes()
            assert b'WHDLoad\0' in icon and b'SLAVE=SLICKS.SLAVE\0' in icon.upper() and b'PRELOAD\0' in icon.upper()
            assert (dest/'Slicks.slave').read_bytes()==(boot/'Slicks.slave').read_bytes()
            if keep or reinstall or incomplete:
                assert (dest/'data/SLICKS.CFG').read_bytes()==b'keep settings'
                assert (dest/'data/SLICKS.REK').read_bytes()==b'private placeholder, not a key'
                assert (dest/'data/PLAYER.PLR').read_bytes()==b'keep profile'
                assert (dest/'data/TEST.SSS').read_bytes()==b'keep championship'
                if not incomplete:assert (dest/'data/TRACKS/CUSTOM.SS').read_bytes()==b'keep custom track'
            assert not list((base/'scratch').glob('.slicks-data-*'))
            assert '.slicks-data-' not in (base/'temp-after.log').read_text(errors='replace')
            print(f'PASS: real Installer, 2 MiB Chip, RAM temp={ram_temp}, keep={keep}, reinstall={reinstall}; expected data, executable, one WHDLoad icon, preservation and cleanup')
        finally:
            emu.terminate()
            try:emu.wait(timeout=5)
            except subprocess.TimeoutExpired:emu.kill();emu.wait()
if __name__=='__main__':main()
