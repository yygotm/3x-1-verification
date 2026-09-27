import json
from pathlib import Path
out=Path(__file__).resolve().parent
data={'complete': True, 'failure': None, 'runs': [{'height': 48, 'power': 40, 'variant': 'baseline', 'rep': 0, 'warmup': True, 'program_seconds': 19.55651004, 'roundtrip_seconds': 19.561810375000277, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'descriptor-large', 'rep': 0, 'warmup': True, 'program_seconds': 11.868305004, 'roundtrip_seconds': 11.872659685001054, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'descriptor-large', 'rep': 1, 'warmup': False, 'program_seconds': 12.4383569, 'roundtrip_seconds': 12.448611005000203, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'baseline', 'rep': 1, 'warmup': False, 'program_seconds': 19.85647852, 'roundtrip_seconds': 19.8652177820004, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'baseline', 'rep': 2, 'warmup': False, 'program_seconds': 21.354889195, 'roundtrip_seconds': 21.35995732400079, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'descriptor-large', 'rep': 2, 'warmup': False, 'program_seconds': 11.75456907, 'roundtrip_seconds': 11.760322702999474, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'descriptor-large', 'rep': 3, 'warmup': False, 'program_seconds': 12.956733295, 'roundtrip_seconds': 13.0236814710006, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'height': 48, 'power': 40, 'variant': 'baseline', 'rep': 3, 'warmup': False, 'program_seconds': 18.903013223, 'roundtrip_seconds': 18.906638451000617, 'stats': {'nodes': 688019545, 'dropI': 519384860704, 'dropP': 25629838208, 'leafn': 4741114976, 'skipn': 2202776936, 'traced': 2538338040, 'stopP': 49186902, 'fails': 0, 'maxsteps': 561, 'maxdropdepth': 36}, 'argmax': 280462342610241}, {'argmax': 1689, 'stats': {'maxsteps': 88}}, {'argmax': 1689, 'stats': {'maxsteps': 88}}, {'argmax': 5505, 'stats': {'maxsteps': 99}}, {'argmax': 5505, 'stats': {'maxsteps': 99}}, {'argmax': 4611686018427387201, 'stats': {'maxsteps': 91}}, {'argmax': 4611686018427387201, 'stats': {'maxsteps': 91}}, {'argmax': 54081, 'stats': {'maxsteps': 130}}, {'argmax': 54081, 'stats': {'maxsteps': 130}}, {'argmax': 281442358627089, 'stats': {'maxsteps': 467}}, {'argmax': 281442358627089, 'stats': {'maxsteps': 467}}], 'launches': [{'height': 48, 'power': 40, 'variant': 'baseline', 'command': ['/mnt/c/Users/81909/Projects/3xm1-release/results/gpu-descriptor-fixed-20260927-1312/performance-5tfmw4he/baseline-steady', '280375465082880', '281474976710656', '0', '26', '10', '16'], 'pid': 468, 'launch_to_ready_seconds': 0.13578004600094573, 'exit_code': 0}, {'height': 48, 'power': 40, 'variant': 'descriptor-large', 'command': ['/mnt/c/Users/81909/Projects/3xm1-release/results/gpu-descriptor-fixed-20260927-1312/performance-5tfmw4he/descriptor-large-steady', '280375465082880', '281474976710656', '0', '26', '10', '16'], 'pid': 470, 'launch_to_ready_seconds': 2.480712250999204, 'exit_code': 0}], 'environment': {'OMP_NUM_THREADS': '16', 'OMP_DYNAMIC': 'FALSE', 'OMP_PROC_BIND': 'FALSE', 'LD_LIBRARY_PATH': '/opt/rocm/core-10.0/lib:/opt/rocm/lib:/usr/lib/wsl/lib'}};cert=[]

def ancestor_below(x,n):
 # Independent integer inverse steps. E(y)=2y, O(y)=(2y+1)/3
 # when y=1 mod3. At most 10 odd and 40 total inverse steps.
 stack=[(x,0,0,[])];visited=set()
 while stack:
  y,a,k,path=stack.pop()
  if y<n:return y,path
  if k>=40 or a>=10:continue
  # Even the remaining all-odd path cannot shrink below n.
  left=10-a
  if y*(1<<left)>=n*3**left:continue
  key=(y,a,k)
  if key in visited:continue
  visited.add(key)
  stack.append((2*y,a,k+1,path+['E']))
  if y%3==1:stack.append(((2*y+1)//3,a+1,k+1,path+['O']))
 return None,None
for n,steps in sorted(set((r['argmax'],r['stats']['maxsteps']) for r in data['runs'])):
 x=n;first=None
 for s in range(1,steps+1):
  x=(3*x-1)//2 if x%2 else x//2
  if x<n and first is None:first=s
 rec=dict(n=n,reported_steps=steps,value_after_reported_steps=str(x),first_drop_step=first,certificate_I=x<n,method='Python arbitrary precision forward steps and independent integer inverse certificate; no C table or jump table')
 if x>=n:
  m,path=ancestor_below(x,n)
  if m is None:raise SystemExit('NO_INDEPENDENT_CERTIFICATE')
  z=m
  for _ in path:z=(3*z-1)//2 if z%2 else z//2
  if not(m<n and z==x):raise SystemExit('INVALID_P_CERTIFICATE')
  rec.update(certificate_P=True,ancestor=str(m),inverse_path=''.join(path),forward_steps=len(path),ancestor_reaches_final=z==x)
 cert.append(rec)
(out/'argmax-independent.json').write_text(json.dumps(cert,indent=2));print(json.dumps(cert),flush=True)
