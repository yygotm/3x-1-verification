#pragma once
#include <stdint.h>
#include <stddef.h>
/* This experimental backend supports exactly these table configurations. */
#if defined(LC) && LC != 10
#error GPU descriptor backend requires LC=10
#endif
#if defined(JK) && JK != 10
#error GPU descriptor backend requires JK=10
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint64_t n,dn,x,dx,r,dr,cnt,j; } desc_t;
typedef struct { uint64_t n,x; uint32_t steps,flag; } result_t;
typedef struct { uint64_t maxsteps,argmax,leafn,skipn,traced,completed; uint32_t fallback,error; } summary_t;
void desc_init(const uint32_t*,const uint32_t*,const uint64_t*,size_t,int,int);
/* One producer owns each slot. input is writable only while free and readable
 * after wait. Summary/fallback/audit remain valid until release. No pointer
 * remains valid after finish. finish must run after all producers join and all
 * slots are released. init/finish are serialized by the caller. Invalid calls
 * terminate with exit code 4; finish before init or after finish is a no-op. */
desc_t *desc_input(int,int);
void desc_submit(int,int,size_t);
const summary_t *desc_wait(int,int);
const result_t *desc_fallback(int,int);
const result_t *desc_audit(int,int);
void desc_release(int,int);
void desc_finish(void);
#ifdef __cplusplus
}
#endif
