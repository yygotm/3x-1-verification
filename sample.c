// 独立な抜き取り検査: [lo, hi) から乱数で選んだ n を、篩も帰納法も使わずに巡回の要素に入るまで直接追う。
// verify.c とは別の判定（「n 未満に落ちる」ではなく「既知の巡回に入る」）で、同じ主張を抜き取りで確かめる。
// 使い方: sample <log2_lo> <log2_hi> <count> <seed>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
typedef unsigned __int128 u128;
static const uint64_t cyc[] = {1,2, 5,14,7,20,10, 17,50,25,74,37,110,55,164,82,41,122,61,182,91,272,136,68,34};
static int incyc(u128 x) { if (x > 272) return 0; for (unsigned i = 0; i < sizeof cyc / sizeof cyc[0]; i++) if (x == cyc[i]) return 1; return 0; }
static uint64_t s[2];
static uint64_t rnd(void) { uint64_t a = s[0], b = s[1]; s[0] = b; a ^= a << 23; s[1] = a ^ b ^ (a >> 17) ^ (b >> 26); return s[1] + b; }
int main(int argc, char **argv) {
    int lo = atoi(argv[1]), hi = atoi(argv[2]); long cnt = atol(argv[3]); s[0] = strtoull(argv[4], 0, 10) | 1; s[1] = 0x9E3779B97F4A7C15ULL;
    long bad = 0, reach[3] = {0}; long maxs = 0; uint64_t arg = 0;
    uint64_t L = 1ULL << lo, span = (1ULL << hi) - L;
    for (long t = 0; t < cnt; t++) {
        uint64_t n = L + rnd() % span; u128 x = n; long st = 0;
        while (!incyc(x)) { x = (x & 1) ? 3 * x - 1 : x >> 1; if (++st > 1000000 || x > ((u128)1 << 120)) { bad++; printf("BAD %llu\n", (unsigned long long)n); break; } }
        if (incyc(x)) { if (x <= 2) reach[0]++; else if (x <= 20) reach[1]++; else reach[2]++; if (st > maxs) { maxs = st; arg = n; } }
    }
    printf("sample [2^%d,2^%d) count=%ld bad=%ld reach{1}=%ld reach{5}=%ld reach{17}=%ld max_total_steps=%ld at n=%llu\n",
           lo, hi, cnt, bad, reach[0], reach[1], reach[2], maxs, (unsigned long long)arg);
    return bad ? 2 : 0;
}
