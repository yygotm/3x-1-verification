import subprocess,os,json,time,re,hashlib,statistics,sys
from pathlib import Path
from harness import Responses, stop
from validate_runs import validate
out=Path(__file__).resolve().parent
env=os.environ.copy();env['PATH']='/opt/rocm/core-10.0/bin:'+env['PATH'];env['LD_LIBRARY_PATH']='/opt/rocm/core-10.0/lib:/opt/rocm/lib:/usr/lib/wsl/lib'
env.update(OMP_NUM_THREADS='16',OMP_DYNAMIC='FALSE',OMP_PROC_BIND='FALSE')
for k in ['BENCH_STRIDE','GPU_AUDIT','GPU_OMP_THREADS']:env.pop(k,None)
records=json.loads((out/'commands.json').read_text()) if (out/'commands.json').exists() else []
def run(label,cmd,timeout=300):
 t=time.perf_counter()
 try:p=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=timeout)
 except subprocess.TimeoutExpired as exc:
  for stream,value in [('stdout',exc.stdout),('stderr',exc.stderr)]:
   if isinstance(value,bytes):value=value.decode(errors='replace')
   (out/(label+'.'+stream+'.txt')).write_text(value or '')
  records.append(dict(label=label,command=cmd,exit_code=None,timed_out=True,wall_seconds=time.perf_counter()-t))
  (out/'commands.json').write_text(json.dumps(records,indent=2));raise
 (out/(label+'.stdout.txt')).write_text(p.stdout);(out/(label+'.stderr.txt')).write_text(p.stderr)
 rec=dict(label=label,command=cmd,exit_code=p.returncode,wall_seconds=time.perf_counter()-t);records.append(rec)
 (out/'commands.json').write_text(json.dumps(records,indent=2));print(json.dumps(rec),flush=True)
 if p.returncode:raise RuntimeError(p.stderr)
 return p.stdout,p.stderr,rec
fields=['nodes','dropI','dropP','leafn','skipn','traced','stopP','fails','maxsteps','maxdropdepth']
def stats(s):return {k:int(re.search(r'\b'+k+r'=(\d+)',s).group(1)) for k in fields}
def steady(powers,gpu_variant='descriptor',filename='steady-runs.json',height=48,response_timeout=300):
 if response_timeout <= 0:raise ValueError('response_timeout must be positive')
 env['OMP_NUM_THREADS']='16';rows=[];launches=[]
 failure=None
 def data(complete=False):return dict(complete=complete,failure=failure,runs=rows,launches=launches,environment={k:env[k] for k in ['OMP_NUM_THREADS','OMP_DYNAMIC','OMP_PROC_BIND','LD_LIBRARY_PATH']})
 def save(complete=False):(out/filename).write_text(json.dumps(data(complete),indent=2))
 save()
 for power in powers:
  ps={};outs={};errs={};readers={};reference=None;cleanup_errors=[]
  try:
   for variant in ['baseline',gpu_variant]:
    args=list(map(str,[(1<<height)-(1<<power),1<<height,0,26,10,16]));cmd=[str(out/(variant+'-steady')),*args]
    prefix='' if filename=='steady-runs.json' else ('large-' if filename=='large-steady-runs.json' else filename.removesuffix('-runs.json')+'-')
    outs[variant]=(out/f'{prefix}steady-w{power}-{variant}.stdout.txt').open('w');errs[variant]=(out/f'{prefix}steady-w{power}-{variant}.stderr.txt').open('w')
    t=time.perf_counter();p=subprocess.Popen(cmd,env=env,text=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errs[variant],bufsize=1);ps[variant]=p
    launches.append(dict(height=height,power=power,variant=variant,command=cmd,pid=p.pid));save()
    readers[variant]=Responses(p,outs[variant]);readers[variant].read('READY rep=0',response_timeout)
    launches[-1]['launch_to_ready_seconds']=time.perf_counter()-t;save()
   for rep in range(4):
    for variant in (['baseline',gpu_variant] if rep%2==0 else [gpu_variant,'baseline']):
     p=ps[variant];t=time.perf_counter();p.stdin.write('go\n');p.stdin.flush();s=readers[variant].read(f'RUN_DONE rep={rep}',response_timeout)
     rec=dict(height=height,power=power,variant=variant,rep=rep,warmup=rep==0,program_seconds=float(re.search(r'time=([0-9.]+)s',s).group(1)),roundtrip_seconds=time.perf_counter()-t,stats=stats(s),argmax=int(re.search(r'at n=(\d+)',s).group(1)))
     if reference is None:reference=rec['stats']
     if reference!=rec['stats'] or 'MISMATCH' in s or rec['stats']['fails']:raise RuntimeError('steady mismatch')
     rows.append(rec);save();print(json.dumps(rec),flush=True)
     if rep<3:readers[variant].read(f'READY rep={rep+1}',response_timeout)
   for variant,p in ps.items():
    p.stdin.close();rc=p.wait(timeout=60)
    next(x for x in launches if x['power']==power and x['variant']==variant)['exit_code']=rc
    if rc:raise RuntimeError('steady process exit '+str(rc))
  except BaseException as exc:
   failure=type(exc).__name__+': '+str(exc);raise
  finally:
   for variant,p in ps.items():
    try:stop(p)
    except Exception as exc:cleanup_errors.append(str(exc))
    next(x for x in launches if x['power']==power and x['variant']==variant)['exit_code']=p.poll()
   for reader in readers.values():
    try:reader.close()
    except Exception as exc:cleanup_errors.append(str(exc))
   for f in [*outs.values(),*errs.values()]:f.close()
   if cleanup_errors:failure=(failure or '')+' cleanup: '+'; '.join(cleanup_errors)
   save()
  if cleanup_errors:raise RuntimeError(failure)
 validate(data(True),powers,['baseline',gpu_variant])
 save(True)
 print('STEADY_COMPLETE',flush=True)