// 突き合わせ用の総当たり: 1 <= n < N の全ての n が巡回 {1}, {5,...}, {17,...} のどれかに入るかを直接確かめる
// （篩も帰納法も使わない独立な実装）。巡回の要素集合に到達したら OK。
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static int incyc(uint64_t x) {
    static const uint64_t c[] = {1,2, 5,14,7,20,10, 17,50,25,74,37,110,55,164,82,41,122,61,182,91,272,136,68,34};
    for (unsigned i = 0; i < sizeof c / sizeof c[0]; i++) if (x == c[i]) return 1;
    return 0;
}
int main(int argc, char **argv) {
    uint64_t N = strtoull(argv[1], 0, 10), bad = 0, cnt[3] = {0};
    for (uint64_t n = 1; n < N; n++) {
        uint64_t x = n; long s = 0;
        while (!incyc(x)) { x = (x & 1) ? 3 * x - 1 : x / 2; if (++s > 100000 || x > (1ULL << 62)) { bad++; printf("BAD %llu\n", (unsigned long long)n); break; } }
        if (incyc(x)) { if (x <= 2) cnt[0]++; else if (x <= 20) cnt[1]++; else cnt[2]++; }
    }
    printf("N=%llu bad=%llu reach{1}=%llu reach{5}=%llu reach{17}=%llu\n", (unsigned long long)N,
           (unsigned long long)bad, (unsigned long long)cnt[0], (unsigned long long)cnt[1], (unsigned long long)cnt[2]);
    return bad ? 2 : 0;
}
