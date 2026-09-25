// 3x-1 写像の計算検証（試作）
// 主張: 1 <= n < LIMIT の全ての正整数は、3 つの巡回 {1,2}, {5,14,7,20,10}, {17,...} のいずれかに入る。
// 方法: 奇数 n を加速写像 C(x) = x/2 (偶) / (3x-1)/2 (奇) で追い、n 未満に落ちたら OK（強い帰納法）。
//       巡回の最小元 1, 5, 17 は既知の巡回として別扱い。
//       2^K を法とした剰余篩: j <= K ステップ以内に 3^a < 2^j となる類は、その類の全ての n で
//       C^j(n) = (3^a n - c)/2^j <= 3^a n / 2^j < n（c >= 0。3x-1 では定数項が引き算）なので丸ごとスキップ。
// 使い方: verify <K> <start_block> <end_block>   （ブロック b は n in [b*2^K, (b+1)*2^K)）
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef unsigned __int128 u128;
#define STEP_CAP 100000

static int K;
static uint32_t *surv;      // 生き残った奇数剰余
static size_t nsurv;

static void build_sieve(void) {
    size_t mod = (size_t)1 << K;
    surv = malloc(sizeof(uint32_t) * (mod / 2));
    nsurv = 0;
    for (uint64_t r = 1; r < mod; r += 2) {
        // r の上位ビットは未知なので、x を「係数 3^a / 2^j と剰余」で追う: x_j = (3^a n - c)/2^j
        // パリティは n ≡ r (mod 2^K) で決まる範囲（j < K）で r の軌道と一致する。
        uint64_t x = r;  // r 自身の軌道でパリティを取る（j < K の範囲では n と一致）
        int a = 0, dropped = 0;
        double lg3 = 1.584962500721156;
        for (int j = 1; j <= K; j++) {
            if (x & 1) { x = (3 * x - 1) / 2; a++; } else { x = x / 2; }
            if (a * lg3 < j - 1e-9) { dropped = 1; break; }   // 3^a < 2^j
        }
        if (!dropped) surv[nsurv++] = (uint32_t)r;
    }
}

// n から n 未満に落ちるまで追う。戻り値: ステップ数（失敗時は -1: 上限到達、-2: オーバーフロー）
static int descend(u128 n) {
    u128 x = n; int steps = 0;
    const u128 lim = ((u128)1 << 126);
    while (x >= n) {
        if (x & 1) { if (x > lim) return -2; x = (3 * x - 1) >> 1; }
        else x >>= 1;
        if (++steps > STEP_CAP) return -1;
    }
    return steps;
}

static int run_range(uint64_t b0, uint64_t b1, FILE *out);

// チェックポイント付きの実行: 区画 [c, c+chunk) ごとに run_range し、終わった区画を log に 1 行ずつ追記する。
// 再起動時は log の "DONE <c0> <c1> K=<K>" 行を読み、c0・c1・K がこの実行の区画と一致するものだけを済みとして飛ばす
// （区画幅や K を変えて同じ log で再開しても、範囲の違う行を済みと誤認しない。ブロックの単位は 2^K なので K も照合する）。
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: verify K start_block end_block [chunk_blocks logfile]\n"); return 1; }
    K = atoi(argv[1]);
    uint64_t b0 = strtoull(argv[2], 0, 10), b1 = strtoull(argv[3], 0, 10);
    build_sieve();
    size_t mod = (size_t)1 << K;
    fprintf(stderr, "K=%d survivors=%zu / %zu odd residues (%.4f%%)\n", K, nsurv, mod / 2, 100.0 * nsurv / (mod / 2));
    if (argc < 6) return run_range(b0, b1, stdout);
    uint64_t chunk = strtoull(argv[4], 0, 10);
    const char *logf = argv[5];
    size_t nch = (b1 - b0 + chunk - 1) / chunk;
    char *done = calloc(nch, 1);
    FILE *lf = fopen(logf, "r");
    if (lf) {
        char line[512]; unsigned long long x, y; int k;
        while (fgets(line, sizeof line, lf))
            if (sscanf(line, "DONE %llu %llu K=%d ", &x, &y, &k) == 3 && k == K && x >= b0 && x < b1 && (x - b0) % chunk == 0
                && y == (x + chunk < b1 ? x + chunk : b1)) done[(x - b0) / chunk] = 1;
        fclose(lf);
    }
    for (size_t c = 0; c < nch; c++) {
        if (done[c]) continue;
        uint64_t c0 = b0 + c * chunk, c1 = c0 + chunk < b1 ? c0 + chunk : b1;
        char buf[512];
        FILE *mem = fmemopen(buf, sizeof buf, "w");
        int rc = run_range(c0, c1, mem);
        fclose(mem);
        lf = fopen(logf, "a");
        fprintf(lf, "%s %llu %llu K=%d %s", rc == 0 ? "DONE" : "FAIL", (unsigned long long)c0, (unsigned long long)c1, K, buf);
        fclose(lf);
        if (rc != 0) return rc;
    }
    lf = fopen(logf, "a"); fprintf(lf, "ALLDONE %llu %llu K=%d\n", (unsigned long long)b0, (unsigned long long)b1, K); fclose(lf);
    return 0;
}

static int run_range(uint64_t b0, uint64_t b1, FILE *out) {
    size_t mod = (size_t)1 << K;
    uint64_t checked = 0, fails = 0; int maxsteps = 0; u128 argmax = 0;
    struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
    #pragma omp parallel for schedule(dynamic, 1) reduction(+:checked, fails)
    for (uint64_t b = b0; b < b1; b++) {
        int lmax = 0; u128 larg = 0;
        for (size_t i = 0; i < nsurv; i++) {
            u128 n = (u128)b * mod + surv[i];
            if (n == 1 || n == 5 || n == 17) continue;          // 既知の巡回の最小元
            int s = descend(n);
            checked++;
            if (s < 0) {
                fails++;
                #pragma omp critical
                fprintf(stderr, "FAIL n=%llu code=%d\n", (unsigned long long)n, s);
            } else if (s > lmax) { lmax = s; larg = n; }
        }
        #pragma omp critical
        { if (lmax > maxsteps) { maxsteps = lmax; argmax = larg; } }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double dt = (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec);
    double span = (double)(b1 - b0) * mod;
    fprintf(out, "range=[%llu*2^%d, %llu*2^%d) checked(odd survivors)=%llu fails=%llu maxsteps=%d at n=%llu time=%.3fs rate=%.3e n/s\n",
           (unsigned long long)b0, K, (unsigned long long)b1, K, (unsigned long long)checked,
           (unsigned long long)fails, maxsteps, (unsigned long long)argmax, dt, span / dt);
    return fails ? 2 : 0;
}
