"""Check repaired sources without transforming or overwriting preserved versions."""
import argparse, hashlib, json, tempfile
from pathlib import Path
import run
source=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def verify_manifest(directory):
 manifest=json.loads((directory/'build-manifest.json').read_text())
 for name,digest in manifest['sources'].items():
  if sha(source/name)!=digest:raise ValueError('source changed since build: '+name)
 for name,digest in manifest['artifacts'].items():
  if sha(directory/name)!=digest:raise ValueError('artifact changed since build: '+name)

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--checks-only',type=Path)
 args=parser.parse_args();directory=Path(tempfile.mkdtemp(prefix='checks-',dir=source))
 run.out=directory;run.records=[]
 if args.checks_only:
  binary_dir=args.checks_only.resolve();verify_manifest(binary_dir)
 else:
  binary_dir=directory
  run.run('build-backend',['hipcc','-O3','-std=c++17','--offload-arch=gfx1201','-Wall','-c',str(source/'descriptor-backend.cpp'),'-o',str(directory/'backend.o')])
  for name in ['baseline','descriptor','descriptor-large','test-backend-final']:
   run.run('build-'+name,['gcc','-O3','-fopenmp','-Wall','-c',str(source/(name+'.c')),'-o',str(directory/(name+'.o'))])
   command=['gcc' if name=='baseline' else 'hipcc',str(directory/(name+'.o'))]
   if name!='baseline':command.append(str(directory/'backend.o'))
   run.run('link-'+name,command+['-fopenmp','-pthread','-lm','-o',str(directory/name)])
  manifest=dict(sources={p.name:sha(p) for p in source.iterdir() if p.suffix in ['.h','.c','.cpp']},artifacts={p.name:sha(p) for p in directory.iterdir() if p.suffix=='.o' or p.name in ['baseline','descriptor','descriptor-large','test-backend-final']})
  with (directory/'build-manifest.json').open('x') as f:json.dump(manifest,f,indent=2)
  verify_manifest(directory)
 checks=[]
 try:
  s,_,r=run.run('independent-backend',[str(binary_dir/'test-backend-final')])
  if 'mismatches=0' not in s:raise ValueError('backend arithmetic mismatch')
  checks.append(r)
  for case in ['zero','oversize','free-wait','bad-slot','free-release','reinit','bad-descriptor','before-init','after-input','after-submit','after-wait','after-fallback','after-audit','after-release','free-fallback','free-audit','finish-busy','invalid-mode','null-table','pinned-budget']:
   import subprocess,time
   cmd=[str(binary_dir/'test-backend-final'),case];started=time.perf_counter()
   p=subprocess.run(cmd,env=run.env,capture_output=True,text=True,timeout=30)
   (directory/(case+'.stdout.txt')).write_text(p.stdout);(directory/(case+'.stderr.txt')).write_text(p.stderr)
   r=dict(case=case,command=cmd,exit_code=p.returncode,wall_seconds=time.perf_counter()-started);checks.append(r)
   if p.returncode!=4:raise ValueError('rejection failed: '+case)
  run.env['OMP_NUM_THREADS']='1'
  for name in ['descriptor','descriptor-large']:
   s,e,r=run.run('mode2-'+name,[str(binary_dir/name),str((1<<44)-(1<<26)),str(1<<44),'2','26','10','16'])
   if 'selfcheck_bad=0' not in s or 'traced=452954 ' not in e:raise ValueError('GPU mode2 verification failed')
   checks.append(r)
  run.env['OMP_NUM_THREADS']='16'
  for label,lo,hi,mode in [('small',1,4097,0),('odd',17,8194,0),('upper',(1<<62)-1025,(1<<62)-1,0),('strict',1,65537,1),('whole36',(1<<48)-(1<<36),1<<48,0)]:
   values=[]
   for name in ['baseline','descriptor-large']:
    s,e,r=run.run(label+'-'+name,[str(binary_dir/name),str(lo),str(hi),str(mode),'26','10','16'])
    values.append(run.stats(s));checks.append(r)
   if values[0]!=values[1] or values[0]['fails']!=0:raise ValueError('CPU/GPU statistics mismatch: '+label)
  verify_manifest(binary_dir)
  (directory/'checks.json').write_text(json.dumps(dict(complete=True,binary_directory=str(binary_dir),checks=checks),indent=2))
 except BaseException as exc:
  (directory/'checks.json').write_text(json.dumps(dict(complete=False,failure=str(exc),checks=checks),indent=2));raise
 print('FINAL_CHECK_COMPLETE '+str(directory),flush=True)

if __name__=='__main__':main()
