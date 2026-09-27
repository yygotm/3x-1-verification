"""One warmup + three interleaved full-workload measurements of repaired code."""
from pathlib import Path
import json,tempfile,hashlib,statistics,sys
import run
from validate_runs import validate
source=Path(__file__).resolve().parent
binary_dir=Path(sys.argv[1]).resolve()
spec=__import__('importlib.util',fromlist=['util']).spec_from_file_location('final_check',source/'final-check.py')
module=__import__('importlib.util',fromlist=['util']).module_from_spec(spec);spec.loader.exec_module(module)
module.verify_manifest(binary_dir)
directory=Path(tempfile.mkdtemp(prefix='performance-',dir=source))
run.out=directory;run.records=[]
for name in ['baseline-steady','descriptor-large-steady']:
 run.run('build-'+name,['gcc','-O3','-fopenmp','-Wall','-c',str(source/(name+'.c')),'-o',str(directory/(name+'.o'))])
 command=['hipcc' if name.startswith('descriptor') else 'gcc',str(directory/(name+'.o'))]
 if name.startswith('descriptor'):command.append(str(binary_dir/'backend.o'))
 run.run('link-'+name,command+['-fopenmp','-pthread','-lm','-o',str(directory/name)])
with (directory/'build-manifest.json').open('x') as f:
 json.dump(dict(sources={p.name:module.sha(p) for p in source.iterdir() if p.suffix in ['.c','.cpp','.h','.py']},backend_object=dict(path=str(binary_dir/'backend.o'),sha256=module.sha(binary_dir/'backend.o')),artifacts={p.name:module.sha(p) for p in directory.iterdir() if p.suffix=='.o' or p.name in ['baseline-steady','descriptor-large-steady']}),f,indent=2)
run.steady([40],'descriptor-large',response_timeout=300)
data=json.loads((directory/'steady-runs.json').read_text());validate(data,[40],['baseline','descriptor-large'])
report={}
for variant in ['baseline','descriptor-large']:
 vals=[r['program_seconds'] for r in data['runs'] if r['variant']==variant and not r['warmup']]
 report[variant]=dict(median=statistics.median(vals),range=[min(vals),max(vals)])
report['speedup']=report['baseline']['median']/report['descriptor-large']['median']
report['paired_speedups']=[next(r['program_seconds'] for r in data['runs'] if r['rep']==rep and r['variant']=='baseline')/next(r['program_seconds'] for r in data['runs'] if r['rep']==rep and r['variant']=='descriptor-large') for rep in [1,2,3]]
(directory/'summary.json').write_text(json.dumps(report,indent=2));module.verify_manifest(binary_dir)
print(json.dumps(report),flush=True);print('PERFORMANCE_COMPLETE '+str(directory),flush=True)
