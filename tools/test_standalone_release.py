"""Directly launch the installed executable on the default 4 KiB CLI stack.

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
def load_checks(path, active=()):
    """Resolve diagnostic includes before moving GDB into its isolated cwd."""
    import shlex
    path=Path(path).resolve()
    if path in active:
        raise ValueError('Recursive diagnostic source: '+str(path))
    lines=[]
    for line in path.read_text().splitlines(keepends=True):
        if line.lstrip().startswith('source '):
            words=shlex.split(line,comments=True)
            if len(words)!=2 or words[1].startswith('-') or '$' in words[1]:
                raise ValueError('Unsupported diagnostic source: '+line.strip())
            lines.append(load_checks(path.parent/words[1],active+(path,)))
            lines.append('\n')
        else:
            lines.append(line)
    return ''.join(lines)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('installed',type=Path)
    parser.add_argument('--default-stack',action='store_true',default=True,
                        help='Compatibility option: all runs use the default stack')
    parser.add_argument('--workbench',action='store_true')
    parser.add_argument('--read-only',action='store_true',
                        help='Mount the isolated game-data volume read-only for real DOS failure tests')
    parser.add_argument('--allocation-audit',action='store_true')
    parser.add_argument('--startup-only',action='store_true',
                        help='Fail the allocation audit on any game-owned allocation after startup')
    parser.add_argument('--audit-players',action='store_true')
    parser.add_argument('--expect-failure',action='store_true')
    parser.add_argument('--repeat',type=int,default=1,
                        help='Relaunch in the same OS session (allocation audit only)')
    parser.add_argument('--args',default='')
    parser.add_argument('--checks',type=Path)
    parser.add_argument('--marker',default='STANDALONE_RELEASE_TITLE_OK')
    parser.add_argument('--cpu',default='68020')
    parser.add_argument('--chip-memory',choices=('1024','2048'),default='2048',
                        help='1024 is only for allocation-failure cleanup tests')
    parser.add_argument('--cpu-speed',choices=('real','max'),default='real')
    args=parser.parse_args()
    if args.repeat<1 or (args.repeat!=1 and not args.allocation_audit):
        parser.error('--repeat must be positive and requires --allocation-audit')
    if args.expect_failure and args.repeat>1:
        parser.error('Expected failures are audited one launch at a time')
    if (args.audit_players or args.expect_failure or args.startup_only) and not args.allocation_audit:
        parser.error('--audit-players, --expect-failure and --startup-only require --allocation-audit')
    if args.allocation_audit and args.checks:
        parser.error('--checks and --allocation-audit are separate debugger modes')
    if (args.args or args.checks) and not args.default_stack:
        parser.error('--args and --checks require --default-stack')
    installed=args.installed.resolve()
    base=Path(tempfile.mkdtemp(prefix='standalone-release-',dir=ROOT/'tmp'))
    boot=base/'boot';(boot/'s').mkdir(parents=True);(boot/'c').mkdir();(base/'state').mkdir();(base/'home').mkdir()
    shutil.copyfile(ROOT/'tmp/SetPatch',boot/'c/SetPatch')
    # Never increase the Shell stack or wrap the executable in a Play script.
    launch='C:SetPatch QUIET\n'+('C:LoadWB\nWait 2\n' if args.workbench else '')+'CD data\n'
    for iteration in range(args.repeat):
        launch+='SlicksNSlide '+args.args+'\n'
        if args.repeat>1:
            launch+='If WARN\nEcho failed >DH0:restart-failed\nQuit 20\nEndIf\n'
            launch+='Echo launch-%d >>DH0:restarts.log\n'%(iteration+1)
            launch+='Avail FLUSH >>DH0:restarts.log\n'
    launch+='Echo done >DH0:finished\n'
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
            checks=load_checks(args.checks)
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
        emu=subprocess.Popen(['fs-uae','--audio_driver=dummy','--amiga_model=A1200','--chip_memory='+args.chip_memory,'--fast_memory=0',
          '--cpu='+args.cpu,'--uae_cpu_speed='+args.cpu_speed,
          '--kickstart_file='+os.environ['KICKSTART'],'--hard_drive_0='+str(boot),'--hard_drive_0_priority=10','--hard_drive_1='+str(installed),
          '--hard_drive_1_read_only='+str(int(args.read_only)),
          '--floppy_drive_0='+str(Path.home()/'Documents/Vette/tmp/Workbenchv2.04rev37.67Workbench.adf'),
          '--remote_debugger=20','--remote_debugger_port='+str(port),'--remote_debugger_trigger=SlicksNSlide',
          '--warp_mode=1','--fullscreen=0','--state_dir='+str(base/'state')],stdout=log,stderr=log,env=env)
        try:
            for _ in range(150):
                if emu.poll() is not None:raise RuntimeError('Emulator exited')
                if subprocess.run(['lsof','-nP',f'-iTCP:{port}','-sTCP:LISTEN'],capture_output=True).returncode==0:break
                time.sleep(.2)
            with (base/'debug.log').open('w') as debug:
                if args.allocation_audit:
                    from memory_audit import run
                    run(ROOT/'amiga/out/SlicksDiag.elf',port,base,debug,
                        players=args.audit_players,expect_failure=args.expect_failure,startup_only=args.startup_only)
                    # FS-UAE's load trigger is one-shot after detach. Audit
                    # the first complete execution, then let DOS verify each
                    # subsequent exit status in this same OS session.
                    if args.repeat>1:
                        deadline=time.monotonic()+600
                        while not (boot/'finished').exists():
                            assert not (boot/'restart-failed').exists(),'Relaunch failed: '+str(base)
                            assert time.monotonic()<deadline,'Relaunch timed out: '+str(base)
                            if emu.poll() is not None:raise RuntimeError('Emulator exited')
                            time.sleep(.2)
                        report=(boot/'restarts.log').read_text()
                        for iteration in range(args.repeat):assert 'launch-%d\n'%(iteration+1) in report
                        debug.write('SAME_OS_RESTART_OK launches=%d\n'%args.repeat)
                    result=subprocess.CompletedProcess([],0)
                else:
                    result=subprocess.run(['m68k-amiga-elf-gdb','-q','-batch','-x',str(base/'test.gdb'),str(ROOT/'amiga/out/SlicksDiag.elf')],stdout=debug,stderr=debug,env=env,cwd=base,timeout=600)
            output=(base/'debug.log').read_text()
            assert result.returncode==0 and args.marker in output,str(base)
            if args.default_stack:
                assert 'DEFAULT_STACK_CONFIRMED bytes=4096' in output,str(base)
            print('PASS:',args.marker,'on',args.chip_memory,'KiB/no-Fast A1200, CPU',args.cpu,args.cpu_speed+';',base)
        finally:
            emu.terminate()
            try:emu.wait(timeout=5)
            except subprocess.TimeoutExpired:emu.kill();emu.wait()
if __name__=='__main__':main()
