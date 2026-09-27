import importlib.util,tempfile,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('audit_production',Path(__file__).with_name('audit-production.py'))
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)

def fixture():
 lines=[]
 for start in range(0,a.FRONT,a.CHUNK):
  first=start==0
  stats=f'nodes=0 dropI={a.ODD if first else 0} dropP=0 leafn=0 skipn=0 traced=0 stopP=0 fails=0 maxsteps={3 if first else 0} at n={a.LO+3 if first else 0} maxdropdepth=0 time=0.1s rate=0.0 n/s'
  lines.append(f'DONE {start} {min(start+a.CHUNK,a.FRONT)} {a.TAG}| '+stats)
 pre='nodes=0 dropI=0 dropP=0 leafn=0 skipn=0 traced=0 stopP=0 fails=0 maxsteps=0 at n=0 maxdropdepth=0 time=0.000s rate=-nan n/s'
 count=len(lines);lines.append(f'ALLDONE {a.TAG}| chunks={count}/{count} odd={a.ODD} accounted={a.ODD} OK maxsteps=3 at n={a.LO+3} | pre: '+pre)
 return lines

class AuditTests(unittest.TestCase):
 def check(self,lines,bad=False):
  with tempfile.TemporaryDirectory() as directory:
   p=Path(directory)/'fixture.log';p.write_text('\n'.join(lines)+'\n')
   if bad:
    with self.assertRaises(ValueError):a.audit(p)
   else:
    result=a.audit(p);self.assertTrue(result['complete']);self.assertTrue(result['argmax_certificate']['certificate_I'])
 def test_valid_synthetic(self):self.check(fixture())
 def test_missing(self):self.check(fixture()[1:],True)
 def test_duplicate(self):
  lines=fixture();lines.insert(1,lines[0]);self.check(lines,True)
 def test_failure(self):
  lines=fixture();lines[0]=lines[0].replace('fails=0','fails=1');self.check(lines,True)
 def test_wrong_odd(self):
  lines=fixture();lines[-1]=lines[-1].replace(f'odd={a.ODD}',f'odd={a.ODD-1}');self.check(lines,True)
 def test_wrong_argmax(self):
  lines=fixture();lines[-1]=lines[-1].replace(f'at n={a.LO+3}',f'at n={a.LO+5}');self.check(lines,True)
 def test_overflow(self):
  lines=fixture();lines[0]=lines[0].replace('maxsteps=3',f'maxsteps={1<<63}');self.check(lines,True)
 def test_no_completion(self):self.check(fixture()[:-1],True)

if __name__=='__main__':unittest.main(verbosity=2)
