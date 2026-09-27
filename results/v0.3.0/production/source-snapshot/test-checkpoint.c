#define main baseline_main
#include "baseline.c"
#undef main
#include <assert.h>
int main(void){
 const char *valid="DONE 0 2 tag| nodes=1 dropI=1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123 maxdropdepth=2";
 size_t ci=99;donestat_t d;
 assert(parse_done(valid,"tag",2,4,&ci,&d)&&ci==0&&d.leafn==3&&d.maxsteps==10&&d.argmax==123);
 const char *bad[]={
  "DONE 999999999999999999999999999999999999999 2 tag| nodes=1 dropI=1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123",
  "DONE 0 2 tag| nodes=1 dropI=18446744073709551616 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123",
  "DONE 0 2 tag| nodes=1 dropI=1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=9223372036854775808 at n=123",
  "DONE 0 2 tag| nodes=1 dropI=-1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123",
  "DONE 0 2 tag| nodes=1 dropI=1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123x",
  "DONE 0 3 tag| nodes=1 dropI=1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123",
  "DONE 0 2 wrong| nodes=1 dropI=1 dropP=2 leafn=3 skipn=1 traced=2 stopP=0 fails=0 maxsteps=10 at n=123",
  "DONE 0 2 t", "DONE ", "", "DONE 0 2 tag| nodes=1"
 };
 for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);i++)assert(!parse_done(bad[i],"tag",2,4,&ci,&d));
 assert(!parse_done(valid,"tag",0,4,&ci,&d));
 puts("CHECKPOINT_TEST valid=1 rejected=12");return 0;
}
