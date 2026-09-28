"""Independent ZIP-library comparison plus rejection/preservation tests."""
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'tmp/Slix151-release.zip'
def expected_files():
    with zipfile.ZipFile(SOURCE) as z:
        return {n:z.read(n) for n in z.namelist() if n in ('SLICKS.000','SLICKS.DAT') or (n.startswith('TRACKS/') and n.endswith('.SS'))}
def main():
    exe=Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='slicks-install-',dir=ROOT/'tmp') as t:
        t=Path(t);dest=t/'new'
        subprocess.run([exe,SOURCE,dest],check=True)
        expected=expected_files()
        assert len(expected)==197
        assert {p.relative_to(dest).as_posix() for p in dest.rglob('*') if p.is_file()}==set(expected)
        for name,data in expected.items(): assert (dest/name).read_bytes()==data,name
        (dest/'SLICKS.REK').write_bytes(b'private-placeholder-not-a-key')
        (dest/'SLICKS.CFG').write_bytes(b'user-settings')
        before={p.relative_to(dest):p.read_bytes() for p in dest.rglob('*') if p.is_file()}
        assert subprocess.run([exe,SOURCE,dest],capture_output=True).returncode==20
        assert before=={p.relative_to(dest):p.read_bytes() for p in dest.rglob('*') if p.is_file()}
        original=SOURCE.read_bytes()
        for i,data in enumerate((b'',original[:100],original[:-1],original+b'x',b'x'+original[1:])):
            bad=t/f'bad{i}.zip';bad.write_bytes(data);out=t/f'out{i}'
            assert subprocess.run([exe,bad,out],capture_output=True).returncode==20
            assert not out.exists()
        assert subprocess.run([exe,t/'missing.zip',t/'missing-out'],capture_output=True).returncode==20
    print('PASS: 197 exact originals, key/settings preservation, corrupt/truncated/wrong ZIP rejection')
if __name__=='__main__': main()
