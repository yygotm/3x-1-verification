#include "descriptor-api.h"
#include <hip/hip_runtime.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <vector>
#include <string>
namespace {
void check(hipError_t e,const char *s){if(e!=hipSuccess){fprintf(stderr,"HIP %s: %s\n",s,hipGetErrorString(e));exit(4);}}
#define CK(x) check((x),#x)
__constant__ uint32_t jump3[1024],jumpd[1024];
__constant__ uint64_t skipbits[923];
struct Sum {uint64_t maxsteps,argmax,leafn,skipn,completed;};
__device__ Sum merge(Sum a,Sum b){
 if(b.maxsteps>a.maxsteps||(b.maxsteps==a.maxsteps&&b.argmax<a.argmax)){a.maxsteps=b.maxsteps;a.argmax=b.argmax;}
 a.leafn+=b.leafn;a.skipn+=b.skipn;a.completed+=b.completed;return a;
}
__global__ void expand_trace(const desc_t *ds,size_t count,Sum *sums,summary_t *header,result_t *fb,uint32_t cap,result_t *audit){
 unsigned lane=threadIdx.x&15;size_t di=(size_t(blockIdx.x)*256+threadIdx.x)/16;
 Sum s{};s.argmax=UINT64_MAX;
 if(di<count){
  desc_t d=ds[di];s.completed=lane==0;
  if(d.cnt==0||d.cnt>16||d.r>=59049||d.dr>=59049||d.j>62){if(lane==0)atomicOr(&header->error,1u);}
  else if(lane<d.cnt){
   uint64_t n=d.n+d.dn*lane,x=d.x+d.dx*lane,r=(d.r+d.dr*lane)%59049;
   unsigned steps=d.j,jumps=0,flag=0;s.leafn=1;
   if((skipbits[r>>6]>>(r&63))&1){s.skipn=1;flag=2;}
   else {
    if(x>=(uint64_t(1)<<57))flag=1;
    else while(x>=n){unsigned u=x&1023;x=uint64_t(jump3[u])*(x>>10)+jumpd[u];steps+=10;
     if(x>=(uint64_t(1)<<57)||++jumps>10000){flag=1;break;}
    }
    if(flag==1){unsigned pos=atomicAdd(&header->fallback,1u);if(pos<cap)fb[pos]={n,x,steps,1};else atomicOr(&header->error,2u);}
    else {s.maxsteps=steps;s.argmax=n;}
   }
   if(audit)audit[di*16+lane]={n,x,steps,flag};
  }
 }
 __shared__ Sum tmp[256];tmp[threadIdx.x]=s;__syncthreads();
 for(unsigned stride=128;stride;stride>>=1){if(threadIdx.x<stride)tmp[threadIdx.x]=merge(tmp[threadIdx.x],tmp[threadIdx.x+stride]);__syncthreads();}
 if(threadIdx.x==0)sums[blockIdx.x]=tmp[0];
}
__global__ void reduce_results(const Sum *blocks,size_t n,summary_t *header){
 Sum s{};s.argmax=UINT64_MAX;for(size_t i=threadIdx.x;i<n;i+=256)s=merge(s,blocks[i]);
 __shared__ Sum tmp[256];tmp[threadIdx.x]=s;__syncthreads();
 for(unsigned stride=128;stride;stride>>=1){if(threadIdx.x<stride)tmp[threadIdx.x]=merge(tmp[threadIdx.x],tmp[threadIdx.x+stride]);__syncthreads();}
 if(threadIdx.x==0){s=tmp[0];header->maxsteps=s.maxsteps;header->argmax=s.argmax;header->leafn=s.leafn;header->skipn=s.skipn;header->traced=s.leafn-s.skipn;header->completed=s.completed;}
}
struct Slot {desc_t *in=nullptr;result_t *fb=nullptr,*audit=nullptr;summary_t *header=nullptr;size_t n=0;bool busy=false,ready=false;};
struct Channel {desc_t *in;result_t *fb,*audit;summary_t *header;Sum *sums;hipStream_t stream;hipEvent_t done,begin,input_done,trace_done,reduce_done;Slot *pending=nullptr;};
std::vector<Slot> slots;Channel channels[2];std::mutex mutex;std::condition_variable cv;std::deque<Slot*> queue;std::thread dispatcher;
bool stopping=false;int producers=0,device=-1,mode=0;size_t capacity;uint64_t calls=0,descriptors=0,leaves=0,traced=0,skipped=0,fallbacks=0,h2d=0,d2h=0;
double init_seconds=0,wait_seconds=0,input_ms=0,trace_ms=0,reduce_ms=0,header_ms=0;bool initialized=false;
Slot &slot(int t,int k){if(t<0||t>=producers||k<0||k>1){fprintf(stderr,"DESC invalid slot\n");exit(4);}return slots[size_t(t)*2+k];}
void complete(Channel &c){
 if(!c.pending)return;Slot *s=c.pending;auto t=std::chrono::steady_clock::now();CK(hipEventSynchronize(c.done));
 if(s->header->error||s->header->completed!=s->n||s->header->fallback>capacity*16){fprintf(stderr,"DESC invalid GPU completion\n");exit(4);}
 float ms;CK(hipEventElapsedTime(&ms,c.begin,c.input_done));input_ms+=ms;CK(hipEventElapsedTime(&ms,c.input_done,c.trace_done));trace_ms+=ms;CK(hipEventElapsedTime(&ms,c.trace_done,c.reduce_done));reduce_ms+=ms;CK(hipEventElapsedTime(&ms,c.reduce_done,c.done));header_ms+=ms;
 size_t bytes=size_t(s->header->fallback)*sizeof(result_t);
 if(bytes){CK(hipMemcpyAsync(s->fb,c.fb,bytes,hipMemcpyDeviceToHost,c.stream));CK(hipStreamSynchronize(c.stream));}
 d2h+=sizeof(summary_t)+bytes+(mode==2?s->n*16*sizeof(result_t):0);
 leaves+=s->header->leafn;traced+=s->header->traced;skipped+=s->header->skipn;fallbacks+=s->header->fallback;
 wait_seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
 {std::lock_guard<std::mutex> lock(mutex);s->ready=true;}c.pending=nullptr;cv.notify_all();
}
void dispatch(){
 CK(hipSetDevice(device));size_t turn=0;
 for(;;){Slot *s;
  {std::unique_lock<std::mutex> lock(mutex);if(queue.empty()){
    lock.unlock();for(auto &c:channels)complete(c);lock.lock();cv.wait(lock,[]{return stopping||!queue.empty();});}
   if(queue.empty()&&stopping)break;s=queue.front();queue.pop_front();}
  Channel &c=channels[turn++%2];complete(c);CK(hipEventRecord(c.begin,c.stream));CK(hipMemcpyAsync(c.in,s->in,s->n*sizeof(desc_t),hipMemcpyHostToDevice,c.stream));
  CK(hipMemsetAsync(c.header,0,sizeof(summary_t),c.stream));CK(hipEventRecord(c.input_done,c.stream));size_t blocks=(s->n+15)/16;
  hipLaunchKernelGGL(expand_trace,dim3(blocks),dim3(256),0,c.stream,c.in,s->n,c.sums,c.header,c.fb,uint32_t(capacity*16),c.audit);CK(hipGetLastError());
  CK(hipEventRecord(c.trace_done,c.stream));hipLaunchKernelGGL(reduce_results,dim3(1),dim3(256),0,c.stream,c.sums,blocks,c.header);CK(hipGetLastError());
  CK(hipEventRecord(c.reduce_done,c.stream));CK(hipMemcpyAsync(s->header,c.header,sizeof(summary_t),hipMemcpyDeviceToHost,c.stream));
  if(mode==2)CK(hipMemcpyAsync(s->audit,c.audit,s->n*16*sizeof(result_t),hipMemcpyDeviceToHost,c.stream));
  CK(hipEventRecord(c.done,c.stream));c.pending=s;calls++;descriptors+=s->n;h2d+=s->n*sizeof(desc_t);
 }
 for(auto &c:channels)complete(c);
}
}
extern "C" void desc_init(const uint32_t *a,const uint32_t *b,const uint64_t *bits,size_t cap,int threads,int m){
 if(initialized||!cap||cap>1048576||threads<1||threads>256||m==1){fprintf(stderr,"DESC invalid initialization\n");exit(4);}
 auto t=std::chrono::steady_clock::now();capacity=cap;producers=threads;mode=m;int count;CK(hipGetDeviceCount(&count));
 for(int i=0;i<count;i++){hipDeviceProp_t prop{};CK(hipGetDeviceProperties(&prop,i));if(std::string(prop.gcnArchName).find("gfx1201")==0){device=i;break;}}
 if(device<0){fprintf(stderr,"DESC gfx1201 not found\n");exit(4);}CK(hipSetDevice(device));
 CK(hipMemcpyToSymbol(HIP_SYMBOL(jump3),a,4096));CK(hipMemcpyToSymbol(HIP_SYMBOL(jumpd),b,4096));CK(hipMemcpyToSymbol(HIP_SYMBOL(skipbits),bits,923*8));
 slots.resize(size_t(threads)*2);
 for(auto &s:slots){CK(hipHostMalloc(&s.in,cap*sizeof(desc_t)));CK(hipHostMalloc(&s.header,sizeof(summary_t)));CK(hipHostMalloc(&s.fb,cap*16*sizeof(result_t)));if(mode==2)CK(hipHostMalloc(&s.audit,cap*16*sizeof(result_t)));}
 for(auto &c:channels){CK(hipMalloc(&c.in,cap*sizeof(desc_t)));CK(hipMalloc(&c.header,sizeof(summary_t)));CK(hipMalloc(&c.fb,cap*16*sizeof(result_t)));CK(hipMalloc(&c.sums,((cap+15)/16)*sizeof(Sum)));c.audit=nullptr;if(mode==2)CK(hipMalloc(&c.audit,cap*16*sizeof(result_t)));CK(hipStreamCreateWithFlags(&c.stream,hipStreamNonBlocking));CK(hipEventCreateWithFlags(&c.done,hipEventDefault));CK(hipEventCreate(&c.begin));CK(hipEventCreate(&c.input_done));CK(hipEventCreate(&c.trace_done));CK(hipEventCreate(&c.reduce_done));}
 initialized=true;init_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();dispatcher=std::thread(dispatch);
 fprintf(stderr,"DESC_INIT capacity=%zu producers=%d streams=2 mode=%d seconds=%.9f\n",capacity,producers,mode,init_seconds);
}
extern "C" desc_t *desc_input(int t,int k){return slot(t,k).in;}
extern "C" void desc_submit(int t,int k,size_t n){Slot &s=slot(t,k);std::unique_lock<std::mutex> lock(mutex);
 if(!n||n>capacity||s.busy){lock.unlock();fprintf(stderr,"DESC invalid submission\n");exit(4);}s.busy=true;s.ready=false;s.n=n;queue.push_back(&s);cv.notify_all();}
extern "C" const summary_t *desc_wait(int t,int k){Slot &s=slot(t,k);std::unique_lock<std::mutex> lock(mutex);if(!s.busy){lock.unlock();fprintf(stderr,"DESC wait free slot\n");exit(4);}cv.wait(lock,[&]{return s.ready;});return s.header;}
extern "C" const result_t *desc_fallback(int t,int k){return slot(t,k).fb;}
extern "C" const result_t *desc_audit(int t,int k){return slot(t,k).audit;}
extern "C" void desc_release(int t,int k){Slot &s=slot(t,k);std::unique_lock<std::mutex> lock(mutex);if(!s.busy||!s.ready){lock.unlock();fprintf(stderr,"DESC invalid release\n");exit(4);}s.busy=false;s.ready=false;}
extern "C" void desc_finish(void){
 if(!initialized)return;{std::lock_guard<std::mutex> lock(mutex);stopping=true;}cv.notify_all();dispatcher.join();
 fprintf(stderr,"DESC_SUMMARY calls=%llu descriptors=%llu leaves=%llu traced=%llu skipped=%llu fallback=%llu h2d_bytes=%llu d2h_bytes=%llu wait_seconds=%.9f init_seconds=%.9f\n",(unsigned long long)calls,(unsigned long long)descriptors,(unsigned long long)leaves,(unsigned long long)traced,(unsigned long long)skipped,(unsigned long long)fallbacks,(unsigned long long)h2d,(unsigned long long)d2h,wait_seconds,init_seconds);
 fprintf(stderr,"DESC_PROFILE input_event_ms=%.6f trace_event_ms=%.6f reduce_event_ms=%.6f header_event_ms=%.6f\n",input_ms,trace_ms,reduce_ms,header_ms);
 for(auto &c:channels){CK(hipEventDestroy(c.begin));CK(hipEventDestroy(c.input_done));CK(hipEventDestroy(c.trace_done));CK(hipEventDestroy(c.reduce_done));CK(hipEventDestroy(c.done));CK(hipStreamDestroy(c.stream));CK(hipFree(c.in));CK(hipFree(c.header));CK(hipFree(c.fb));CK(hipFree(c.sums));if(c.audit)CK(hipFree(c.audit));}
 for(auto &s:slots){CK(hipHostFree(s.in));CK(hipHostFree(s.header));CK(hipHostFree(s.fb));if(s.audit)CK(hipHostFree(s.audit));}initialized=false;
}
