import copy, importlib.util, json, math, os, subprocess, sys, tempfile, unittest
from pathlib import Path
from validate_runs import validate
import run

class AuditTests(unittest.TestCase):
 def setUp(self):
  self.data={'complete':True,'runs':[dict(power=36,variant=v,rep=r,warmup=r==0,wall_seconds=1.0,program_seconds=1.0,exit_code=0,stats={'fails':0,'leafn':10}) for v in ['baseline','descriptor'] for r in range(4)],'launches':[dict(power=36,variant=v,exit_code=0) for v in ['baseline','descriptor']]}
 def check_bad(self,mutation,cold=True):
  data=copy.deepcopy(self.data);mutation(data)
  with self.assertRaises(ValueError):validate(data,[36],['baseline','descriptor'],cold=cold)
 def test_valid(self):
  for cold in [True,False]:self.assertEqual(len(validate(self.data,[36],['baseline','descriptor'],cold=cold)),8)
 def test_missing(self):self.check_bad(lambda d:d['runs'].pop())
 def test_failed(self):self.check_bad(lambda d:d['runs'][0].update(exit_code=4))
 def test_duplicate(self):self.check_bad(lambda d:d['runs'].append(d['runs'][0]))
 def test_incomplete(self):self.check_bad(lambda d:d.update(complete=False))
 def test_old_list_without_completion(self):
  with self.assertRaises(ValueError):validate(self.data['runs'],[36],['baseline','descriptor'],cold=True)
 def test_stats(self):self.check_bad(lambda d:d['runs'][0].update(stats={'fails':0,'leafn':9}))
 def test_warmup(self):self.check_bad(lambda d:d['runs'][1].update(warmup=True))
 def test_nan(self):self.check_bad(lambda d:d['runs'][1].update(wall_seconds=math.nan))
 def test_failed_launch(self):self.check_bad(lambda d:d['launches'][0].update(exit_code=4),False)
 def test_missing_launch(self):self.check_bad(lambda d:d['launches'].pop(),False)

FAKE=r'''#!PYTHON
import sys,time
name=__import__('pathlib').Path(sys.argv[0]).name
if name.startswith('bad-ready'):time.sleep(30)
print('READY rep=0',flush=True)
for rep in range(4):
 if not sys.stdin.readline():break
 if name.startswith('bad-run'):time.sleep(30)
 if name.startswith('bad-eof'):sys.exit(4)
 print('nodes=1 dropI=1 dropP=1 leafn=1 skipn=1 traced=0 stopP=0 fails=0 maxsteps=1 maxdropdepth=1 at n=1 time=0.01s',flush=True)
 print('RUN_DONE rep='+str(rep),flush=True)
 if rep<3:print('READY rep='+str(rep+1),flush=True)
'''

class HarnessTests(unittest.TestCase):
 def exercise(self,variant,success=False):
  original=run.out
  try:
   with tempfile.TemporaryDirectory() as directory:
    run.out=Path(directory)
    for name in ['baseline',variant]:
     p=run.out/(name+'-steady');p.write_text(FAKE.replace('PYTHON',sys.executable));p.chmod(0o755)
    if success:run.steady([10],variant,response_timeout=2)
    else:
     with self.assertRaises((TimeoutError,RuntimeError)):run.steady([10],variant,response_timeout=0.3)
    data=json.loads((run.out/'steady-runs.json').read_text())
    self.assertEqual(data['complete'],success)
    if not success:self.assertTrue(data['failure'])
    for launch in data['launches']:
     self.assertIsNotNone(launch['exit_code'])
     with self.assertRaises(ProcessLookupError):os.kill(launch['pid'],0)
    if success:self.assertEqual(len(data['runs']),8)
    elif variant=='bad-run':self.assertEqual(len(data['runs']),1)
  finally:run.out=original
 def test_success(self):self.exercise('descriptor',True)
 def test_ready_timeout_cleanup(self):self.exercise('bad-ready')
 def test_run_timeout_cleanup(self):self.exercise('bad-run')
 def test_eof_cleanup(self):self.exercise('bad-eof')

class ManifestTests(unittest.TestCase):
 def test_rerun_and_tamper(self):
  path=Path(__file__).with_name('final-check.py');spec=importlib.util.spec_from_file_location('final_check',path)
  module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
  with tempfile.TemporaryDirectory() as d:
   directory=Path(d);binary=directory/'backend.o';binary.write_bytes(b'fixture')
   source=Path(__file__).with_name('descriptor-backend.cpp');before=source.read_bytes()
   manifest={'sources':{source.name:module.sha(source)},'artifacts':{'backend.o':module.sha(binary)}}
   (directory/'build-manifest.json').write_text(json.dumps(manifest))
   module.verify_manifest(directory);module.verify_manifest(directory)
   self.assertEqual(source.read_bytes(),before)
   binary.write_bytes(b'changed')
   with self.assertRaises(ValueError):module.verify_manifest(directory)

if __name__=='__main__':unittest.main(verbosity=2)
