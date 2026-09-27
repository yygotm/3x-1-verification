"""Validate the complete checkpoint log and independently certify its argmax."""
from pathlib import Path
import json,re,sys
out=Path(__file__).resolve().parent
LO=1<<48;HI=1<<50;FRONT=656074;CHUNK=4096;ODD=(HI-LO)//2
TAG=f'LO={LO} HI={HI} mode=0 F=26 L=10 LEAFN=16 nfront={FRONT}'
FIELDS=['nodes','dropI','dropP','leafn','skipn','traced','stopP','fails','maxsteps','maxdropdepth']

def fields(text):
 result={}
 for field in FIELDS:
  m=re.search(r'\b'+field+r'=(\d+)(?: |$)',text)
  if not m:raise ValueError('missing '+field)
  result[field]=int(m.group(1))
  if result[field]>((1<<63)-1 if field=='maxsteps' else (1<<64)-1):raise ValueError('counter overflow')
 result['argmax']=int(re.search(r'at n=(\d+)',text).group(1))
 return result

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
def audit(path):
 seen={};completion=[];seconds=0
 for line in path.read_text().splitlines():
  if line.startswith('DONE '):
   match=re.match(r'DONE (\d+) (\d+) '+re.escape(TAG)+r'\| (.*)',line)
   if not match:raise ValueError('invalid DONE header')
   start,end=map(int,match.group(1,2))
   if start%CHUNK or start>=FRONT or end!=min(start+CHUNK,FRONT) or start in seen:raise ValueError('invalid/duplicate DONE interval')
   row=fields(match.group(3));seen[start]=row
   if row['fails'] or row['traced']+row['skipn']!=row['leafn']:raise ValueError('invalid DONE counters')
   seconds+=float(re.search(r'time=([\d.]+)s',line).group(1))
  elif line.startswith('ALLDONE '):completion.append(line)
  elif line.startswith('FAIL ') or line.startswith('ALLFAIL '):raise ValueError('failure record')
  elif line.strip():raise ValueError('unexpected log record')
 expected=set(range(0,FRONT,CHUNK))
 if set(seen)!=expected or len(completion)!=1:raise ValueError('incomplete log')
 final=completion[0]
 match=re.match(r'ALLDONE '+re.escape(TAG)+r'\| chunks=(\d+)/(\d+) odd=(\d+) accounted=(\d+) OK maxsteps=(\d+) at n=(\d+) \| pre: (.*)',final)
 if not match:raise ValueError('invalid completion record')
 count,total,odd,accounted,maxsteps,argmax=map(int,match.group(1,2,3,4,5,6))
 if count!=len(expected) or total!=count or odd!=ODD or accounted!=ODD:raise ValueError('completion range/count mismatch')
 pre=fields(match.group(7));sums={key:pre[key]+sum(row[key] for row in seen.values()) for key in FIELDS if key not in ['maxsteps','maxdropdepth']}
 if sums['fails'] or sums['dropI']+sums['dropP']+sums['leafn']!=ODD:raise ValueError('odd aggregation mismatch')
 if maxsteps!=max([pre['maxsteps']]+[row['maxsteps'] for row in seen.values()]):raise ValueError('maxsteps mismatch')
 if not LO<=argmax<HI:raise ValueError('argmax outside requested range')
 if not any(row['maxsteps']==maxsteps and row['argmax']==argmax for row in seen.values()):raise ValueError('argmax absent from maximum chunk records')
 x=argmax
 for _ in range(maxsteps):x=(3*x-1)//2 if x&1 else x//2
 certificate=dict(n=argmax,steps=maxsteps,value_after_steps=str(x),certificate_I=x<argmax)
 if x>=argmax:
  ancestor,path=ancestor_below(x,argmax)
  if ancestor is None:raise ValueError('missing independent argmax certificate')
  y=ancestor
  for _ in path:y=(3*y-1)//2 if y&1 else y//2
  if not ancestor<argmax or y!=x:raise ValueError('invalid independent argmax certificate')
  certificate.update(certificate_P=True,ancestor=str(ancestor),inverse_path=''.join(path))
 return dict(complete=True,lo=LO,hi=HI,nfront=FRONT,chunks=count,odd=ODD,statistics=sums,maxsteps=maxsteps,argmax=argmax,compute_seconds=seconds,argmax_certificate=certificate)

if __name__=='__main__':
 result=audit(Path(sys.argv[1]) if len(sys.argv)>1 else out/'production.log')
 print(json.dumps(result,indent=2),flush=True)
