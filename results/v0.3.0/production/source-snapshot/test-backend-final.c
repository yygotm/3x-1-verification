#define main baseline_main
#include "baseline.c"
#undef main
#include "descriptor-api.h"
static int cmp_result(const void *aa,const void *bb){
 const result_t *a=aa,*b=bb;
 if(a->n!=b->n)return a->n<b->n?-1:1;
 if(a->x!=b->x)return a->x<b->x?-1:1;
 return a->steps<b->steps?-1:a->steps>b->steps;
}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"before-init"))desc_input(0,0);
 P3[0]=1;for(int i=1;i<=80;i++)P3[i]=3*P3[i-1];build_paths();build_jump();
 if(argc>1&&!strcmp(argv[1],"invalid-mode"))desc_init(jt3,jtd,skipbits,33,1,-1);
 if(argc>1&&!strcmp(argv[1],"null-table"))desc_init(NULL,jtd,skipbits,33,1,2);
 if(argc>1&&!strcmp(argv[1],"pinned-budget"))desc_init(jt3,jtd,skipbits,1048576,256,0);
 desc_init(jt3,jtd,skipbits,33,1,2);atexit(desc_finish);
 if(argc>1){
  if(!strncmp(argv[1],"after-",6)){
   desc_finish();desc_finish();
   if(!strcmp(argv[1],"after-input"))desc_input(0,0);
   else if(!strcmp(argv[1],"after-submit"))desc_submit(0,0,1);
   else if(!strcmp(argv[1],"after-wait"))desc_wait(0,0);
   else if(!strcmp(argv[1],"after-fallback"))desc_fallback(0,0);
   else if(!strcmp(argv[1],"after-audit"))desc_audit(0,0);
   else if(!strcmp(argv[1],"after-release"))desc_release(0,0);
   return 9;
  }
  if(!strcmp(argv[1],"free-fallback"))desc_fallback(0,0);
  if(!strcmp(argv[1],"free-audit"))desc_audit(0,0);
  if(!strcmp(argv[1],"finish-busy")){
   desc_input(0,0)[0]=(desc_t){100,2,50,0,100,2,1,1};desc_submit(0,0,1);desc_finish();
  }
  if(!strcmp(argv[1],"reinit")){desc_finish();desc_init(jt3,jtd,skipbits,33,1,2);}
  else if(!strcmp(argv[1],"bad-descriptor")){desc_input(0,0)[0]=(desc_t){100,2,200,3,100,2,17,1};desc_submit(0,0,1);desc_wait(0,0);}
  else if(!strcmp(argv[1],"zero"))desc_submit(0,0,0);
  else if(!strcmp(argv[1],"oversize"))desc_submit(0,0,34);
  else if(!strcmp(argv[1],"free-wait"))desc_wait(0,0);
  else if(!strcmp(argv[1],"bad-slot"))desc_input(1,0);
  else if(!strcmp(argv[1],"free-release"))desc_release(0,0);
  return 9;
 }
 unsigned bads=0;uint64_t checked=0,fbchecked=0;
 for(unsigned rep=0;rep<6;rep++){
  int slot=rep%2;unsigned count=rep%3==0?1:rep%3==1?19:33;desc_t *ds=desc_input(0,slot);
  for(unsigned i=0;i<count;i++){
   uint64_t n=((uint64_t)1<<48)+rep*10001+i*521+1;
   uint64_t x=i%4==0?n/2:i%4==1?n*4:i%4==2?XLIM-1:XLIM+123;
   ds[i]=(desc_t){n,2,x,0,n%M3,2,1+i%16,1+i%61};
  }
  desc_submit(0,slot,count);const summary_t *h=desc_wait(0,slot);const result_t *audit=desc_audit(0,slot);
  summary_t expected={0};result_t want[33*16],got[33*16];unsigned wn=0;
  for(unsigned i=0;i<count;i++)for(uint64_t t=0;t<ds[i].cnt;t++){
   uint64_t n=ds[i].n+t*2;u128 x=ds[i].x;unsigned steps=ds[i].j,flag=0,jumps=0;uint64_t r=n%M3;
   expected.leafn++;
   if((skipbits[r>>6]>>(r&63))&1){expected.skipn++;flag=2;}
   else {
    expected.traced++;
    if(x>=XLIM)flag=1;
    else while(x>=n){x=fwd(x,10);steps+=10;if(x>=XLIM||++jumps>10000){flag=1;break;}}
    if(flag==1)want[wn++]=(result_t){n,(uint64_t)x,steps,1};
    else if(steps>expected.maxsteps){expected.maxsteps=steps;expected.argmax=n;}
   }
   result_t v=audit[i*16+t];if(v.n!=n||v.x!=(uint64_t)x||v.steps!=steps||v.flag!=flag)bads++;
   checked++;
  }
  if(h->leafn!=expected.leafn||h->skipn!=expected.skipn||h->traced!=expected.traced||h->maxsteps!=expected.maxsteps||h->fallback!=wn||h->completed!=count)bads++;
  memcpy(got,desc_fallback(0,slot),wn*sizeof(result_t));qsort(got,wn,sizeof(result_t),cmp_result);qsort(want,wn,sizeof(result_t),cmp_result);
  for(unsigned q=0;q<wn;q++)if(got[q].n!=want[q].n||got[q].x!=want[q].x||got[q].steps!=want[q].steps||got[q].flag!=1)bads++;
  if(h->maxsteps){int found=0;for(unsigned i=0;i<count;i++)for(uint64_t t=0;t<ds[i].cnt;t++)if(audit[i*16+t].flag==0&&audit[i*16+t].n==h->argmax&&audit[i*16+t].steps==h->maxsteps)found=1;if(!found)bads++;}
  fbchecked+=wn;desc_release(0,slot);
 }
 printf("BACKEND_TEST checked=%llu fallback_checked=%llu mismatches=%u\n",(unsigned long long)checked,(unsigned long long)fbchecked,bads);
 return bads?2:0;
}
