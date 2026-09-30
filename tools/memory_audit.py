"""Audit actual Exec allocation calls without changing the target binary.

The debugger bundled with the Amiga tools has no Python support, and FS-UAE
limits simultaneous breakpoints. Drive two Exec-vector breakpoints from the
host instead of instrumenting hundreds of call sites or allocating a target
side tracking table. Includes constructors before main and full shutdown.
"""
import os
import re
import select
import subprocess
import time

class Debugger:
    def __init__(self, elf, cwd, log):
        self.proc=subprocess.Popen(['m68k-amiga-elf-gdb','-q','-nx',str(elf)],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,cwd=cwd)
        self.log=log;self.buffer=b'';self.prompt=b'(gdb) '
        self.read()
    def read(self):
        deadline=time.monotonic()+600
        while self.prompt not in self.buffer:
            assert time.monotonic()<deadline,'Debugger timeout'
            if not select.select([self.proc.stdout],[],[],1)[0]:continue
            data=os.read(self.proc.stdout.fileno(),65536)
            assert data,'Debugger exited unexpectedly'
            self.buffer+=data
        data,self.buffer=self.buffer.split(self.prompt,1)
        text=data.decode(errors='replace');self.log.write(text);self.log.flush()
        return text
    def cmd(self,command):
        self.proc.stdin.write((command+'\n').encode());self.proc.stdin.flush()
        return self.read()
    def value(self,expression):
        result=self.cmd('printf "AUDIT_VALUE=%u\\n",(unsigned long)('+expression+')')
        return int(re.search(r'AUDIT_VALUE=(\d+)',result)[1])
    def breakpoint(self,expression,temporary=False):
        result=self.cmd(('tbreak ' if temporary else 'break ')+expression)
        return int(re.search(r'[Bb]reakpoint (\d+)',result)[1])
    def close(self):
        self.proc.terminate()
        try:self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:self.proc.kill();self.proc.wait()

def run(elf,port,cwd,log,players=False,expect_failure=False):
    g=Debugger(elf,cwd,log)
    live={};allocations=0;failed=0;reached_players=False
    def report(text):log.write(text+'\n');log.flush()
    try:
        g.cmd('set pagination off');g.cmd('set confirm off')
        g.cmd('target remote 127.0.0.1:%d'%port)
        start=g.value('&_start');end=g.value('&_end')
        assert g.value('$pc')==start,'Audit must begin before startup constructors'
        ret=g.value('*(unsigned long *)$sp')
        lower=g.value('*(unsigned long *)(*(unsigned long *)(*(unsigned long *)4+276)+58)')
        upper=g.value('*(unsigned long *)(*(unsigned long *)(*(unsigned long *)4+276)+62)')
        assert upper-lower==4096,'Not the default Shell stack'
        report('DEFAULT_STACK_CONFIRMED bytes=4096')
        # This FS-UAE build does not reliably implement target-memory writes.
        # Never claim a debugger-written stack watermark or injected failure.
        sysbase=g.value('*(unsigned long *)4')
        condition=' if *(unsigned long *)$sp >= %d && *(unsigned long *)$sp < %d'%(start,end)
        alloc=g.breakpoint('*%d'%(sysbase-198)+condition)
        free=g.breakpoint('*%d'%(sysbase-210)+condition)
        done=g.breakpoint('*%d'%ret)
        menu=g.breakpoint('slicks_diag_player_menu_ready') if players else None
        while True:
            output=g.cmd('continue')
            match=re.search(r'Breakpoint (\d+),',output)
            assert match,('Unexpected debugger stop',output)
            hit=int(match[1])
            if hit==done:break
            if hit==menu:
                reached_players=True
                continue
            size=g.value('$d0')
            if hit==free:
                ptr=g.value('$a1')
                assert live.get(ptr)==size,('unowned/mismatched free',hex(ptr),size,live.get(ptr))
                del live[ptr]
                report('MEM_FREE ptr=%x size=%d'%(ptr,size))
            else:
                assert hit==alloc
                allocations+=1
                caller=g.value('*(unsigned long *)$sp')
                g.breakpoint('*%d'%caller,temporary=True)
                output=g.cmd('continue')
                assert 'Temporary breakpoint' in output,output
                ptr=g.value('$d0')
                report('MEM_ALLOC ptr=%x size=%d caller=%x'%(ptr,size,caller))
                if ptr:
                    assert ptr not in live,('duplicate allocation',hex(ptr))
                    live[ptr]=size
                else:failed+=1
        result=g.value('$d0')
        if re.search(r'g_slicks_menu_workspace_conflicts;',
                     g.cmd('info variables g_slicks_menu_workspace_conflicts')):
            assert g.value('g_slicks_menu_workspace_conflicts')==0,'Overlapping menu/track workspace ownership'
            report('MENU_WORKSPACE_OWNERSHIP_OK')
        if re.search(r'unsigned long g_slicks_stack_unused;',
                     g.cmd('info variables g_slicks_stack_unused')):
            unused=g.value('g_slicks_stack_unused')
            report('NATIVE_STACK_UNUSED_BOTTOM bytes=%d'%unused)
            assert unused>=64,'Native stack watermark exhausted its safety margin'
        report('MEMORY_RETURN code=%d allocations=%d failed=%d outstanding=%d players=%d'%
               (result,allocations,failed,len(live),reached_players))
        assert not live,('Leaked allocations',live)
        assert result==(20 if expect_failure else 0),('Unexpected exit code',result)
        assert not players or reached_players,'Players was not reached'
        if players:
            assert g.value('demo_players_test')==4,'Native Players open/close did not complete'
            assert g.value('g_slicks_demo_natural_returns')==2,'Both demos must finish naturally'
            assert g.value('g_slicks_demo_test_error')==0,'Demo configuration/playlist regression'
        if g.value('display_allocation_test'):
            assert g.value('g_slicks_display_allocation_checks')==10,'Incomplete display failure coverage'
        report('ALLOCATION_CLEANUP_OK')
        # Resume into DOS so the CLI can unload this executable and run it
        # again. FS-UAE's executable-load trigger is one-shot after detach.
        g.cmd('delete breakpoints')
        g.cmd('detach')
    finally:g.close()
