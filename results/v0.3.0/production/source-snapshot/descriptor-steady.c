// 3x-1 写像の計算検証（高速版）
// 主張: LO <= n < HI の全ての正整数は、3 つの巡回 {1,2}, {5,14,7,20,10}, {17,...} のいずれかに入る
//       （1 <= n < LO が検証済みであることを前提にする。LO = 1 なら前提なし）。
//
// 帰納法: 各 n（巡回の最小元 1, 5, 17 を除く）について、軌道上のある x について
//   (I) x < n、または
//   (P) x の逆向き経路で作る祖先 m = (2^k x + c)/3^a が m < n
// を示す。(P) は E(y) = 2y（常に C(2y) = y）と O(y) = (2y+1)/3（y ≡ 1 mod 3 のとき奇数の整数で C(O(y)) = y）を
// k 回（うち O が a 回）かけたもので、m の軌道は k 歩で x を通る。経路の可否は x mod 3^L だけで決まる（a <= L）。
// 最小の反例 n0 を考えると、(I) でも (P) でも n0 より小さい反例（x または m）が出るので矛盾する。x = n（0 歩目）で (P) なら n を追わずに済む。
//
// 木: 奇数 n を n mod 2^j のクラスに分け、クラスの全員に共通な最初の j 歩を 1 回だけ計算する。
//   クラス（代表 B < 2^j、深さ j）の員は n(t) = B + 2^j t、x_j(t) = e + 3^a t（t は [LO, HI) に入る範囲）。
//   (I)(P) の不等式は t について 1 次なので、範囲の両端で成り立てばクラス全体で成り立つ → クラスごと落とす。
//   (P) は a >= L のときだけ使う（そのとき x_j(t) ≡ e mod 3^L がクラスで一定）。
//   クラスの員数が LEAFN 以下になったら、1 つずつ追う（葉）。
//
// モード: 0 = 本番、1 = 厳密（(P) を使わず、葉では n から素朴に数えた停止時間を記録。旧 verify の maxsteps と比べる用）、
//         2 = 自己検査（本番の判定を、落とした数・止めた数の全てについて素朴な前向き計算で照合する。小さい範囲用）
// 使い方: verify2 <LO> <HI> <mode> [F L LEAFN [chunk logfile]]（chunk logfile はチェックポイント付き。mode 2・BENCH_STRIDE とは併用不可）
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <omp.h>
#include "descriptor-api.h"

typedef unsigned __int128 u128;
#define STEP_CAP 100000
// 確保の失敗は黙って続けない（NULL 参照は未定義動作。検証ツールは止まる方が安全）
static void *xmalloc(size_t n) { void *p = malloc(n); if (!p) { fprintf(stderr, "out of memory (%zu bytes)\n", n); exit(3); } return p; }
static void *xrealloc(void *p, size_t n) { void *q = realloc(p, n); if (!q) { free(p); fprintf(stderr, "out of memory (%zu bytes)\n", n); exit(3); } return q; }
static void *xcalloc(size_t n) { void *p = calloc(n ? n : 1, 1); if (!p) { fprintf(stderr, "out of memory (%zu bytes)\n", n); exit(3); } return p; }

static uint64_t LO, HI;
// 3 進の法はコンパイル時の定数にする（% M3 を掛け算とシフトにするため。実行時の変数だと除算命令になり遅い）
#ifndef LC
#define LC 10
#endif
#if LC == 6
#define M3 729ULL
#elif LC == 8
#define M3 6561ULL
#elif LC == 10
#define M3 59049ULL
#elif LC == 12
#define M3 531441ULL
#else
#error LC must be 6, 8, 10 or 12
#endif
static int MODE, F = 22, L = LC, LEAFN = 2;
static u128 P3[81];
static uint64_t P3M[81], INV2;                                     // 3^a mod 3^L、2 の逆元 mod 3^L
typedef struct { uint64_t c; uint8_t a, k; } path_t;
static path_t *ptab;
static uint16_t *pt16; static uint64_t *pc;                          // 詰めた表: a | k << 5（0 なら経路なし）と c。判定の頻度が高いので小さくする
static uint64_t P3S[21];
static uint64_t skipbits[(M3 + 63) / 64], SKIP_T;                     // 0 歩目の (P) を剰余だけで決める表と、それが正しくなる n の下限

// ---- 3 進の逆向き経路の表 ----
static double bestR; static path_t bp;
static void pdfs(uint64_t y, int a, int k, uint64_t c) {        // y は mod 3^(L-a) で持つ
    double R = pow(3, a) / pow(2, k);
    if (R > bestR) { bestR = R; bp.a = a; bp.k = k; bp.c = c; }
    if (a == L || R * pow(1.5, L - a) <= bestR || k > 40) return;
    uint64_t mod = (uint64_t)P3[L - a];
    if (y % 3 == 1) pdfs(((2 * y + 1) / 3) % (uint64_t)P3[L - a - 1], a + 1, k + 1, 2 * c + (uint64_t)P3[a]);
    pdfs((2 * y) % mod, a, k + 1, 2 * c);
}
static void build_paths(void) {
    for (int i = 0; i <= 80; i++) P3M[i] = (uint64_t)(P3[i] % M3);
    INV2 = (M3 + 1) / 2;
    ptab = xmalloc(sizeof(path_t) * M3);
    for (uint64_t r = 0; r < M3; r++) { bestR = 1; memset(&bp, 0, sizeof bp); pdfs(r, 0, 0, 0); ptab[r] = bp; }
    pt16 = xmalloc(sizeof(uint16_t) * M3); pc = xmalloc(sizeof(uint64_t) * M3);
    for (uint64_t r = 0; r < M3; r++) { pt16[r] = ptab[r].a ? (uint16_t)(ptab[r].a | ptab[r].k << 5) : 0; pc[r] = ptab[r].c; }
    for (int i = 0; i <= 20; i++) P3S[i] = (uint64_t)P3[i];
    // 2^k n + c < 3^a n  <=>  n (3^a - 2^k) > c。3^a > 2^k の経路なら n > c / (3^a - 2^k) で必ず成り立つ。
    // SKIP_T = max_r floor(c / (3^a - 2^k)) + 1 とすると、n >= SKIP_T の n では「経路があり 3^a > 2^k」だけで判定できる。
    SKIP_T = 0;
    for (uint64_t r = 0; r < M3; r++) {
        path_t p = ptab[r];
        if (!p.a || P3[p.a] <= ((u128)1 << p.k)) continue;
        uint64_t d = (uint64_t)(P3[p.a] - ((u128)1 << p.k)), t = p.c / d + 1;
        if (t > SKIP_T) SKIP_T = t;
        skipbits[r >> 6] |= (uint64_t)1 << (r & 63);
    }
    fprintf(stderr, "SKIP_T=%llu\n", (unsigned long long)SKIP_T);
}
// (P) の判定: 2^k x + c < 3^a n（x mod 3^L の最良経路。a = 0 は恒等で使わない）
static inline int ptest(u128 x, u128 n, uint64_t xr) {
    uint16_t v = pt16[xr];
    if (!v) return 0;
    u128 lhs = x << (v >> 5), rhs = (u128)P3S[v & 31] * n;          // c >= 0 なので lhs >= rhs なら不成立（c を読まずに済む）
    return lhs < rhs && lhs + pc[xr] < rhs;
}

// ---- 自己検査用の素朴な計算 ----
static u128 fwd(u128 x, int steps) { while (steps--) x = (x & 1) ? (3 * x - 1) >> 1 : x >> 1; return x; }
static long selfcheck_bad = 0;
static inline void bad(void) {
    #pragma omp atomic
    selfcheck_bad++;
}
// (P) の証明書を照合: m = (2^k x + c)/3^a が整数、m < n、m から k 歩で x
static void check_P(u128 x, u128 n) {
    path_t p = ptab[(uint64_t)(x % M3)];
    u128 num = (x << p.k) + p.c;
    if (!p.a || num % P3[p.a] != 0) { bad(); return; }
    u128 m = num / P3[p.a];
    if (!(m < n) || fwd(m, p.k) != x) bad();
}

typedef struct {
    uint64_t nodes, dropI, dropP, dropn, leafn, skipn, traced, fails, stopP;
    long maxsteps; u128 argmax; int maxdropdepth;
} stats_t;

// 葉の 1 つの n を追う。x は j 歩目の値。戻り値: 停止時の歩数（負なら失敗）
static long trace(u128 n, u128 x, long s, stats_t *st) {
    const u128 lim = (u128)1 << 125;
    u128 nq = n * 32;                    // (P) を試すのは x < 32n のときだけ（それ以上は使える経路がほぼ無い。試さないのは安全側）
    for (;;) {
        if (x < n) return s;
        if (MODE != 1 && x < nq && ptest(x, n, (uint64_t)(x % M3))) { st->stopP++; if (MODE == 2) check_P(x, n); return s; }
        if (x & 1) { if (x > lim) return -2; x = (3 * x - 1) >> 1; } else x >>= 1;
        if (++s > STEP_CAP) return -1;
    }
}

// 64 bit の高速経路（本番用）。奇数のあとの半減は ctz でまとめる（途中の値で判定しないのは安全側）。
// 3x-1 が 64 bit を超えそうなら u128 版に渡す。
// k 歩の一括表: x = 2^JK q + u（u = x mod 2^JK）なら C^JK(x) = 3^a(u) q + C^JK(u)（最初の JK 歩の偶奇は u で決まる）。
#ifndef JK
#define JK 10
#endif
#ifndef TRACE_P
#define TRACE_P 0
#endif
#ifndef LEAF_SKIP
#define LEAF_SKIP 1
#endif
#ifndef TREE_P
#define TREE_P 1
#endif
#if LC != 10 || JK != 10
#error GPU descriptor backend requires LC=10 and JK=10
#endif
static uint32_t jt3[1 << JK], jtd[1 << JK];
static void build_jump(void) {
    for (uint64_t u = 0; u < (1u << JK); u++) {
        uint64_t x = u, p = 1;
        for (int i = 0; i < JK; i++) { if (x & 1) { x = (3 * x - 1) >> 1; p *= 3; } else x >>= 1; }
        jt3[u] = (uint32_t)p; jtd[u] = (uint32_t)x;
    }
}
// 本番の葉: JK 歩ずつ進め、区切りごとに (I)(P) を判定する（区切りの間で判定しないのは安全側）。
// 3^JK < 2^16 なので x < 2^47 なら 3^a q + d < 2^64。それを超えたら 1 歩ずつの版に渡す。
static long trace64j(uint64_t n, uint64_t x, long s, stats_t *st);
static long trace64(uint64_t n, uint64_t x, long s, stats_t *st) {
    const uint64_t lim = (UINT64_MAX - 1) / 3;
    uint64_t nq = TRACE_P ? (n < ((uint64_t)1 << 58) ? n * TRACE_P : UINT64_MAX) : 0;
    for (;;) {
        if (x < n) return s;
        if (x < nq && ptest(x, n, x % M3)) { st->stopP++; if (MODE == 2) check_P(x, n); return s; }
        if (x & 1) { if (x > lim) return trace(n, x, s, st); x = 3 * x - 1; }
        int z = __builtin_ctzll(x); x >>= z; s += z;
        if (s > STEP_CAP) return -1;
        if (x < ((uint64_t)1 << 47)) return trace64j(n, x, s, st);
    }
}
static long trace64j(uint64_t n, uint64_t x, long s, stats_t *st) {
    uint64_t nq = TRACE_P ? (n < ((uint64_t)1 << 58) ? n * TRACE_P : UINT64_MAX) : 0;   // 追う途中の (P)。実測では無し（0）が最速
    for (;;) {
        if (x < n) return s;
        if (x < nq && ptest(x, n, x % M3)) { st->stopP++; if (MODE == 2) check_P(x, n); return s; }
        if (x >= ((uint64_t)1 << 47)) return trace64(n, x, s, st);
        uint32_t u = x & ((1u << JK) - 1);
        x = (uint64_t)jt3[u] * (x >> JK) + jtd[u]; s += JK;
        if (s > STEP_CAP) return -1;
    }
}

// ---- 葉の数をまとめて追う（本番と自己検査）: 員を行列に貯め、W 本の軌道を交互に JK 歩ずつ進める ----
// 止まる判定（x < n）と入れ替えは条件付き移動で書き、分岐の予測外れと依存の待ちを減らす。
// x が 2^57 以上になる・歩数が上限に達するのは稀なので、その軌道だけ 1 本ずつの版（trace）に回す。
#ifndef W
#define W 8
#endif
#define QCAP 1024
#define XLIM ((uint64_t)1 << (63 - (JK * 585 + 999) / 1000))     // x < XLIM なら 3^JK (x >> JK) + d < 2^64（log2 3 - 1 < 0.585。JK=10 で 2^57）
typedef struct { uint64_t n, x; uint32_t j; } item_t;
static __thread item_t Q[QCAP + 1];
static __thread size_t qn;
static void slow_one(uint64_t n, u128 x, long s, stats_t *st) {       // 稀な場合: 1 本ずつの版で最後まで追う
    long r = trace(n, x, s, st);
    if (r < 0) {
        st->fails++;
        #pragma omp critical
        fprintf(stderr, "FAIL n=%llu code=%ld\n", (unsigned long long)n, r);
    } else if (r > st->maxsteps) { st->maxsteps = r; st->argmax = n; }
}
static void kernel(stats_t *st) {
    size_t m = qn, next = 0, left = m;
    if (!m) return;
    Q[m] = (item_t){UINT64_MAX, 0, 0};                                  // 番兵: 0 < n で即座に終わる
    uint64_t n[W], x[W]; uint32_t sj[W], jj[W];
    for (int w = 0; w < W; w++) { n[w] = UINT64_MAX; x[w] = 0; sj[w] = 0; jj[w] = 0; }
    long ms = st->maxsteps; u128 am = st->argmax;
    while (left) {
        #pragma GCC unroll 16
        for (int w = 0; w < W; w++) {
            uint64_t xv = x[w], nv = n[w];
            int done = xv < nv, real = nv != UINT64_MAX, fin = done & real;
            left -= fin;
            long steps = (long)jj[w] + (long)JK * sj[w];
            if (fin && steps > ms) { ms = steps; am = nv; }
            if (MODE == 2 && fin && fwd(nv, (int)steps) != xv) bad();   // 止まった値を素朴な前向き計算で照合
            size_t i = next < m ? next : m;
            item_t it = Q[i];
            next += done & (next < m);
            uint32_t u = xv & ((1u << JK) - 1);
            uint64_t xj = (uint64_t)jt3[u] * (xv >> JK) + jtd[u];
            uint32_t s2 = sj[w] + 1;
            x[w] = done ? it.x : xj; n[w] = done ? it.n : nv; sj[w] = done ? 0 : s2; jj[w] = done ? it.j : jj[w];
            if (__builtin_expect(!done && (xj >= XLIM || s2 > STEP_CAP / JK), 0)) {
                slow_one(nv, xj, (long)jj[w] + (long)JK * s2, st);       // この軌道は 1 本ずつの版で終える
                left--;
                n[w] = UINT64_MAX; x[w] = 0;                              // 番兵に戻す（次の周で補充）
            }
        }
    }
    if (ms > st->maxsteps) { st->maxsteps = ms; st->argmax = am; }
    qn = 0;
}

// 本番用の葉（64 bit の足し算だけ）。条件: 員が巡回の最小元を含まない（n0 > 17）、飛ばす判定が剰余だけで決まる（n0 >= SKIP_T）、
// 員の n と x が 2^63 未満（n は HI < 2^62 で常に、x は呼び出し側で x1 を確認）。判定の中身は leaf() の一般経路と同じ。
static void leaf_fast(uint64_t n, uint64_t dn, uint64_t x, uint64_t dx, uint64_t r, uint64_t dr, uint64_t cnt, int j, stats_t *st) {
    uint64_t skipped = 0, q = qn;
    for (uint64_t t = 0; t < cnt; t++) {
        int skip = LEAF_SKIP & (int)((skipbits[r >> 6] >> (r & 63)) & 1);
        skipped += skip;
        if (__builtin_expect(!skip && x >= XLIM, 0)) slow_one(n, x, j, st);   // 稀: u128 の 1 歩版へ
        else {
            Q[q] = (item_t){n, x, (uint32_t)j};                            // 分岐なしで追加（飛ばすなら q を進めない）
            q += !skip;
            if (q == QCAP) { qn = q; kernel(st); q = 0; }
        }
        n += dn; x += dx; r += dr; r -= r >= M3 ? M3 : 0;
    }
    qn = q;
    st->leafn += cnt; st->skipn += skipped; st->traced += cnt - skipped;
}

static void cpu_leaf(uint64_t B, int j, int a, u128 e, uint64_t tlo, uint64_t thi, stats_t *st) {
    // 員を t の順に足し算で進める: n += 2^j、x += 3^a、n mod 3^L += 2^j mod 3^L
    u128 n = (u128)B + ((u128)tlo << j), x = e + P3[a] * tlo, dx = P3[a];
    uint64_t rn = (uint64_t)(n % M3), dr = (uint64_t)((((u128)1) << j) % M3), dn = (uint64_t)1 << j;
    if (MODE == 0 && n > 17 && n >= SKIP_T && !((x + dx * (thi - tlo - 1)) >> 63)) {
        leaf_fast((uint64_t)n, dn, (uint64_t)x, (uint64_t)dx, rn, dr, thi - tlo, j, st);
        return;
    }
    for (uint64_t t = tlo; t < thi; t++, n += dn, x += dx, rn = rn + dr >= M3 ? rn + dr - M3 : rn + dr) {
        st->leafn++;
        if (n == 1 || n == 5 || n == 17) continue;
        long s;
        if (MODE == 1) {
            s = trace(n, n, 0, st);                                   // 厳密: n から素朴に
        } else {
            if (MODE == 2 && fwd(n, j) != x) bad();                  // x_j の公式を照合
            uint64_t r = rn;                                         // 0 歩目で (P): n を飛ばす
            if (MODE == 2 && r != (uint64_t)(n % M3)) bad();
            int skip;
            if (n >= SKIP_T) skip = (skipbits[r >> 6] >> (r & 63)) & 1;   // n が大きければ剰余だけで決まる（ビット表、L1 に収まる）
            else { uint16_t v = pt16[r]; u128 lhs = n << (v >> 5), rhs = (u128)P3S[v & 31] * n; skip = lhs + pc[r] < rhs; }
            skip &= LEAF_SKIP;
            st->skipn += skip;
            if (MODE == 2 && skip) check_P(n, n);
            if (__builtin_expect(!skip && x >= XLIM, 0)) { st->traced++; slow_one((uint64_t)n, x, j, st); continue; }
            Q[qn] = (item_t){(uint64_t)n, (uint64_t)x, (uint32_t)j};  // 分岐なしで追加（飛ばすなら qn を進めない）
            qn += !skip; st->traced += !skip;
            if (qn == QCAP) kernel(st);
            continue;
        }
        st->traced++;
        if (s < 0) {
            st->fails++;
            #pragma omp critical
            fprintf(stderr, "FAIL n=%llu code=%ld\n", (unsigned long long)n, s);
        } else if (s > st->maxsteps) { st->maxsteps = s; st->argmax = n; }
    }
}

static inline void trange(uint64_t B, int j, uint64_t *tlo, uint64_t *thi) {
    uint64_t w = (uint64_t)1 << j;
    *tlo = LO > B ? (LO - B + w - 1) >> j : 0;
    *thi = HI > B ? (HI - B + w - 1) >> j : 0;
}

// クラスの判定: 1 = (I) で落ちる、2 = (P) で落ちる、0 = 落ちない
// 両端で十分な根拠: 員は n(t) = B + 2^j t、x(t) = e + 3^a t（t = tlo..t1 の整数）で、どちらも t の 1 次式。
//   (I) d(t) = x(t) − n(t) = (e − B) + (3^a − 2^j) t も 1 次式。1 次式の値は区間の両端の値の凸結合
//       d((1−λ) tlo + λ t1) = (1−λ) d(tlo) + λ d(t1)（0 <= λ <= 1）なので、両端で負なら間の全ての t で負。
//   (P) 3^a' n(t) − (2^k x(t) + c) も t の 1 次式（a', k, c は経路の定数）なので同じ。経路は x mod 3^L で決まり、
//       a >= L なら x(t) ≡ e (mod 3^L) がクラスで一定なので全ての t で同じ経路が使え、祖先 (2^k x(t) + c)/3^a' は整数。

#define DESC_CAP 32768
static __thread int desc_thread,desc_active,desc_busy[2];
static __thread size_t desc_count,desc_counts[2];
static __thread uint64_t desc_leaves,desc_expected[2];
static __thread desc_t *desc_queue;
static void desc_consume(int k,stats_t *st){
 if(!desc_busy[k])return;
 const summary_t *h=desc_wait(desc_thread,k);
 if(h->leafn!=desc_expected[k]||h->skipn+h->traced!=h->leafn){fprintf(stderr,"descriptor count mismatch\n");exit(4);}
 st->leafn+=h->leafn;st->skipn+=h->skipn;st->traced+=h->traced;
 if((long)h->maxsteps>st->maxsteps){st->maxsteps=h->maxsteps;st->argmax=h->argmax;}
 if(MODE==2){
  const result_t *a=desc_audit(desc_thread,k);const desc_t *ds=desc_input(desc_thread,k);
  for(size_t i=0;i<desc_counts[k];i++)for(uint64_t t=0;t<ds[i].cnt;t++){
   uint64_t n=ds[i].n+t*ds[i].dn,r=(ds[i].r+t*ds[i].dr)%M3;const result_t *v=&a[i*16+t];
   int skip=(skipbits[r>>6]>>(r&63))&1;
   if(v->n!=n||(v->flag==2)!=skip||v->flag>2)bad();
   if(skip)check_P(n,n);
   else {if(fwd(n,v->steps)!=v->x)bad();if(v->flag==0&&v->x>=n)bad();}
  }
 }
 const result_t *fb=desc_fallback(desc_thread,k);
 for(uint32_t i=0;i<h->fallback;i++)slow_one(fb[i].n,fb[i].x,fb[i].steps,st);
 desc_release(desc_thread,k);desc_busy[k]=0;
}
static void desc_flush(stats_t *st){
 if(!desc_count)return;int k=desc_active;desc_counts[k]=desc_count;desc_expected[k]=desc_leaves;desc_busy[k]=1;
 desc_submit(desc_thread,k,desc_count);desc_count=0;desc_leaves=0;desc_active=1-k;
 desc_consume(desc_active,st);desc_queue=desc_input(desc_thread,desc_active);
}
static void leaf(uint64_t B,int j,int a,u128 e,uint64_t tlo,uint64_t thi,stats_t *st){
 u128 n=(u128)B+((u128)tlo<<j),x=e+P3[a]*tlo;uint64_t cnt=thi-tlo;
 if(MODE!=1&&LEAF_SKIP&&cnt&&cnt<=16&&n>17&&n>=SKIP_T&&!((x+P3[a]*(cnt-1))>>63)){
  uint64_t dn=(uint64_t)1<<j,r=(uint64_t)(n%M3),dr=dn%M3;
  if(MODE==2)for(uint64_t t=0;t<cnt;t++)if(fwd(n+dn*t,j)!=x+P3[a]*t)bad();
  desc_queue[desc_count++]=(desc_t){(uint64_t)n,dn,(uint64_t)x,(uint64_t)P3[a],r,dr,cnt,(uint64_t)j};
  desc_leaves+=cnt;if(desc_count==DESC_CAP)desc_flush(st);return;
 }
 cpu_leaf(B,j,a,e,tlo,thi,st);
}

static int class_drops(uint64_t B, int j, int a, u128 e, uint64_t r, uint64_t tlo, uint64_t thi) {
    uint64_t t1 = thi - 1;
    u128 n0 = (u128)B + ((u128)tlo << j), n1 = (u128)B + ((u128)t1 << j);
    u128 x0 = e + P3[a] * tlo, x1 = e + P3[a] * t1;
    // 桁あふれを避ける（落とさないのは安全側）。表に記録される経路（a', k, c）は R = 3^a'/2^k > bestR >= 1 を満たすので
    // k < a'·log2 3 <= LC·1.585: LC=10 で k <= 15、LC=12 で k <= 19（pdfs の k > 40 の打ち切りではなく、この不等式が効く）。
    // x1 < 2^90 なら x << k < 2^109、3^a' n < 3^12 · 2^62 < 2^82、c <= 3^a' (2^k − 1) < 2^39 で、どれも u128 に収まる。
    // LC を上げるときはここを見直す。
    if (x1 >> 90) return 0;
    // 巡回の最小元 1, 5, 17 を含むクラスは落とさず葉まで下ろす（葉では追わずに飛ばす）。
    // この除外が無くても落ちないことは証明できる（冗長な安全策。狙った試験は test_cm.c）:
    //   (I) 落ちるなら全ての t で x(t) < n(t) だが、n(t*) = m が巡回の最小元なら x(t*) = C^j(m) は m の巡回の元で m 以上。
    //   (P) 落ちるなら m' = (2^k x(t*) + c)/3^a' が正整数で m' < m、かつ m' から k 歩で C^j(m) に着くので m' は m の巡回に入る。
    //       m = 1 なら m' < 1 の正整数は無い。m = 5, 17 なら 1..16 の行き先は巡回 {1} か {5}（列挙で確認）で、
    //       5 の巡回に入る m' < 5 も、17 の巡回に入る m' < 17 も無い。
    // -DNO_CM_EXCL はこの除外を外した変異（試験用: test_cm.c と bench_b.sh が同じソースから作る。本番では定義しない）。
#ifndef NO_CM_EXCL
    for (int i = 0; i < 3; i++) {
        static const uint64_t cm[3] = {1, 5, 17};
        if (cm[i] >= n0 && cm[i] <= n1 && ((cm[i] - B) & (((uint64_t)1 << j) - 1)) == 0) return 0;
    }
#endif
    if (x0 < n0 && x1 < n1) return 1;
    if (TREE_P && MODE != 1 && a >= L) {
        if (ptest(x0, n0, r) && ptest(x1, n1, r)) return 2;
    }
    return 0;
}

static void dfs(uint64_t B, int j, int a, u128 e, uint64_t r, stats_t *st) {
    uint64_t tlo, thi;
    trange(B, j, &tlo, &thi);
    if (tlo >= thi) return;
    st->nodes++;
    if (MODE == 2 && e % M3 != r) bad();                               // 持ち回りの剰余を照合
    int d = class_drops(B, j, a, e, r, tlo, thi);
    if (d) {
        uint64_t cnt = thi - tlo;
        if (d == 1) st->dropI += cnt; else st->dropP += cnt;
        if (j > st->maxdropdepth) st->maxdropdepth = j;
        if (MODE == 2)                                               // 落としたクラスの全員を素朴に照合
            for (uint64_t t = tlo; t < thi; t++) {
                u128 n = (u128)B + ((u128)t << j), x = fwd(n, j);
                if (x != e + P3[a] * t) { bad(); continue; }
                if (d == 1) { if (!(x < n)) bad(); } else check_P(x, n);
            }
        return;
    }
    if (thi - tlo <= (uint64_t)LEAFN || j >= 62 || (e >> 90)) { leaf(B, j, a, e, tlo, thi, st); return; }
    for (int b = 0; b < 2; b++) {
        uint64_t B2 = B + ((uint64_t)b << j), rc = b ? (r + P3M[a]) % M3 : r, r2;
        u128 ec = b ? e + P3[a] : e, e2; int a2 = a;
        if (ec & 1) { e2 = (3 * ec - 1) >> 1; a2++; r2 = (3 * rc + M3 - 1) % M3 * INV2 % M3; } else { e2 = ec >> 1; r2 = rc * INV2 % M3; }
        dfs(B2, j + 1, a2, e2, r2, st);
    }
}

// ---- 分割（並列化とチェックポイントの単位）: 深さ F まで下ろしたクラスの列 ----
typedef struct { uint64_t B; int j, a; u128 e; uint64_t r; } node_t;
static node_t *front; static size_t nfront, capfront;
static stats_t pre;                                                  // 分割の途中で落ちた・葉になったもの
static void push_front(node_t nd) {                                  // realloc はここだけ（失敗は xrealloc が止める）
    if (nfront == capfront) { capfront = capfront ? 2 * capfront : 1024; front = xrealloc(front, sizeof(node_t) * capfront); }
    front[nfront++] = nd;
}
static void split(uint64_t B, int j, int a, u128 e, uint64_t r) {
    uint64_t tlo, thi;
    trange(B, j, &tlo, &thi);
    if (tlo >= thi) return;
    if (j == F || thi - tlo <= (uint64_t)LEAFN) { push_front((node_t){B, j, a, e, r}); return; }
    pre.nodes++;
    int d = class_drops(B, j, a, e, r, tlo, thi);
    if (d) {                                                         // 浅い段で落ちるクラス（自己検査は dfs 側と同じ手順を踏むため、ここでは front に回す）
        if (MODE == 2) { push_front((node_t){B, j, a, e, r}); pre.nodes--; return; }
        if (d == 1) pre.dropI += thi - tlo; else pre.dropP += thi - tlo;
        return;
    }
    for (int b = 0; b < 2; b++) {
        uint64_t B2 = B + ((uint64_t)b << j), rc = b ? (r + P3M[a]) % M3 : r, r2;
        u128 ec = b ? e + P3[a] : e, e2; int a2 = a;
        if (ec & 1) { e2 = (3 * ec - 1) >> 1; a2++; r2 = (3 * rc + M3 - 1) % M3 * INV2 % M3; } else { e2 = ec >> 1; r2 = rc * INV2 % M3; }
        split(B2, j + 1, a2, e2, r2);
    }
}

static void add(stats_t *s, const stats_t *t) {
    s->nodes += t->nodes; s->dropI += t->dropI; s->dropP += t->dropP; s->leafn += t->leafn; s->skipn += t->skipn;
    s->traced += t->traced; s->fails += t->fails; s->stopP += t->stopP;
    if (t->maxsteps > s->maxsteps) { s->maxsteps = t->maxsteps; s->argmax = t->argmax; }
    if (t->maxdropdepth > s->maxdropdepth) s->maxdropdepth = t->maxdropdepth;
}

static size_t STRIDE = 1;                                            // ベンチマーク用の抜き取り（環境変数 BENCH_STRIDE）。本番は 1
static stats_t run_front(size_t i0, size_t i1) {
    stats_t tot = {0};
    #pragma omp parallel
    {
        stats_t st = {0};
        if(MODE!=1){desc_thread=omp_get_thread_num();desc_active=0;desc_count=0;desc_leaves=0;desc_busy[0]=desc_busy[1]=0;desc_queue=desc_input(desc_thread,0);}
        #pragma omp for schedule(dynamic, 1) nowait
        for (size_t i = i0; i < i1; i += STRIDE) dfs(front[i].B, front[i].j, front[i].a, front[i].e, front[i].r, &st);
        kernel(&st); if(MODE!=1){desc_flush(&st);desc_consume(0,&st);desc_consume(1,&st);} // CPU rare leaves and descriptor tails
        #pragma omp critical
        add(&tot, &st);
    }
    return tot;
}

static void print_stats(FILE *o, const stats_t *s, double dt, double span) {
    fprintf(o, "nodes=%llu dropI=%llu dropP=%llu leafn=%llu skipn=%llu traced=%llu stopP=%llu fails=%llu maxsteps=%ld at n=%llu maxdropdepth=%d time=%.9fs rate=%.3e n/s\n",
            (unsigned long long)s->nodes, (unsigned long long)s->dropI, (unsigned long long)s->dropP, (unsigned long long)s->leafn,
            (unsigned long long)s->skipn, (unsigned long long)s->traced, (unsigned long long)s->stopP, (unsigned long long)s->fails,
            s->maxsteps, (unsigned long long)s->argmax, s->maxdropdepth, dt, span / dt);
}

static uint64_t parse_u64(const char *s, uint64_t lo, uint64_t hi, const char *name) {   // 範囲付きの厳密な整数解析（失敗は終了）
    char *end; errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    // strtoull は先頭の空白・'+'・'-' を受け付け、"-1" を 2^64 − 1 にする。先頭が数字でなければ拒否する
    if (*s < '0' || *s > '9' || errno || end == s || *end || v < lo || v > hi) {
        fprintf(stderr, "bad %s: '%s' (allowed %llu..%llu)\n", name, s, (unsigned long long)lo, (unsigned long long)hi); exit(1);
    }
    return (uint64_t)v;
}
// DONE 行の解析: この実行の tag と区画割りに合う行だけ受け付け、区画番号と統計を返す（再開の判定と最終集計で共用）
typedef struct { uint64_t dropI, dropP, leafn, fails; long maxsteps; unsigned long long argmax; } donestat_t;
#include "checkpoint-parser.h"

int main(int argc, char **argv) {
    // 引数は 3〜6 個（単発）か 8 個（チェックポイント付き）。7 個（logfile の書き忘れ）や 9 個以上は黙って単発にせず拒否する
    if (argc < 4 || argc == 8 || argc > 9) { fprintf(stderr, "usage: verify2 LO HI mode [F L LEAFN [chunk logfile]]\n"); return 1; }
    const uint64_t HMAX = ((uint64_t)1 << 62) - 1;                    // HI < 2^62: n < 2^62 で 32n・3^12 n・(3/2)^62 n が u128 に収まる前提
    LO = parse_u64(argv[1], 1, HMAX - 1, "LO"); HI = parse_u64(argv[2], LO + 1, HMAX, "HI");
    MODE = (int)parse_u64(argv[3], 0, 2, "mode");
    if (argc > 4) F = (int)parse_u64(argv[4], 1, 40, "F");
    if (argc > 5) L = (int)parse_u64(argv[5], LC, LC, "L");            // L はコンパイル時定数 LC と一致しなければならない
    if (argc > 6) LEAFN = (int)parse_u64(argv[6], 1, 1 << 30, "LEAFN");
    if (getenv("BENCH_STRIDE")) STRIDE = (size_t)parse_u64(getenv("BENCH_STRIDE"), 1, (uint64_t)1 << 30, "BENCH_STRIDE");
    size_t chunk = 0; const char *logf = NULL;
    if (argc == 9) {
        chunk = (size_t)parse_u64(argv[7], 1, SIZE_MAX, "chunk"); logf = argv[8];
        // 自己検査の不一致（selfcheck_bad）はチェックポイントの log に残らない。抜き取りは区画の統計を歪める。どちらも単発でだけ使う
        if (MODE == 2) { fprintf(stderr, "mode 2 (selfcheck) cannot be used with chunk/logfile\n"); return 1; }
        if (STRIDE > 1) { fprintf(stderr, "BENCH_STRIDE cannot be used with chunk/logfile\n"); return 1; }
        FILE *lf = fopen(logf, "a");                                 // 先に log を作る（区画が 0 個でも最後の読み込みで落ちない。パスの誤りも準備の前に分かる）
        if (!lf) { perror(logf); return 3; }
        fclose(lf);
    }
    P3[0] = 1; for (int i = 1; i <= 80; i++) P3[i] = 3 * P3[i - 1];
    struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
    build_paths();
    build_jump();
    if(MODE!=1){desc_init(jt3,jtd,skipbits,DESC_CAP,omp_get_max_threads(),MODE);atexit(desc_finish);}
    // 根: 奇数 n = 1 + 2t、x_1 = 1 + 3t（深さ 1、a = 1、代表 B = 1）。偶数 n は 1 歩で n/2 < n。
    split(1, 1, 1, 1, 1 % M3);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    fprintf(stderr, "LO=%llu HI=%llu mode=%d F=%d L=%d LEAFN=%d front=%zu setup=%.2fs\n", (unsigned long long)LO, (unsigned long long)HI,
            MODE, F, L, LEAFN, nfront, (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec));
    double span = (double)(HI - LO);
    if (getenv("BENCH_STRIDE")) { span /= STRIDE; fprintf(stderr, "BENCH: every %zu-th of the split only, rate scaled\n", STRIDE); }
    if(argc<9){for(int rep=0;rep<4;rep++){printf("READY rep=%d\n",rep);fflush(stdout);char gate[32];if(!fgets(gate,sizeof gate,stdin))return 3;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        stats_t s = run_front(0, nfront); add(&s, &pre);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double dt = (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec);
        uint64_t odd = HI / 2 - LO / 2;                                  // [LO, HI) の奇数の個数
        print_stats(stdout, &s, dt, span);
        if (STRIDE == 1) printf("odd in range=%llu accounted(dropI+dropP+leafn)=%llu %s\n", (unsigned long long)odd,
               (unsigned long long)(s.dropI + s.dropP + s.leafn), odd == s.dropI + s.dropP + s.leafn ? "OK" : "MISMATCH");
        if (MODE == 2) printf("selfcheck_bad=%ld\n", selfcheck_bad);
        if(s.fails||selfcheck_bad||(STRIDE==1&&odd!=s.dropI+s.dropP+s.leafn))return 2;
        printf("RUN_DONE rep=%d\n",rep);fflush(stdout);
    }return 0;}
    // チェックポイント付き: 分割の列を chunk 個ずつ実行し、済んだものを log に追記する
    // 区画数は桁あふれしない形で（(nfront + chunk − 1) / chunk は chunk が大きいと 0 になる）。chunk > nfront なら 1 区画で i0 = 0 なので、
    // 下の i0 + chunk と parse_done の x + chunk も溢れない（i0 > 0 なら chunk < nfront で i0 + chunk < 2 nfront）
    size_t nch = nfront / chunk + (nfront % chunk != 0);
    char *done = xcalloc(nch);
    char tag[256];
    snprintf(tag, sizeof tag, "LO=%llu HI=%llu mode=%d F=%d L=%d LEAFN=%d nfront=%zu", (unsigned long long)LO, (unsigned long long)HI, MODE, F, L, LEAFN, nfront);
    FILE *lf = fopen(logf, "r");
    if (lf) {
        char line[1024]; size_t ci; donestat_t d;
        while (fgets(line, sizeof line, lf)) if (parse_done(line, tag, chunk, nfront, &ci, &d)) done[ci] = 1;
        fclose(lf);
    }
    for (size_t c = 0; c < nch; c++) {
        if (done[c]) continue;
        size_t i0 = c * chunk, i1 = i0 + chunk < nfront ? i0 + chunk : nfront;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        stats_t s = run_front(i0, i1);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double dt = (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec);
        lf = fopen(logf, "a");
        if (!lf) { perror(logf); return 3; }
        fprintf(lf, "%s %zu %zu %s| ", s.fails ? "FAIL" : "DONE", i0, i1, tag);
        print_stats(lf, &s, dt, span * (i1 - i0) / nfront);
        int werr = ferror(lf); werr |= fclose(lf) != 0;               // 書けなかった区画を済んだことにしない（書き込みは fclose で初めて失敗することがある）
        if (werr) { perror(logf); return 3; }
        if (s.fails) return 2;
    }
    // 全区画が済んだ: log の DONE 行（再開前の分も含む）から合計を作り、奇数の個数と照合する（単発実行と同じ検査を本番経路にも）
    stats_t all = pre; memset(done, 0, nch);
    lf = fopen(logf, "r"); if (!lf) { perror(logf); return 3; }
    { char line[1024]; size_t ci; donestat_t d;
      while (fgets(line, sizeof line, lf))
          if (parse_done(line, tag, chunk, nfront, &ci, &d) && !done[ci]) {
              done[ci] = 1; all.dropI += d.dropI; all.dropP += d.dropP; all.leafn += d.leafn; all.fails += d.fails;
              if (d.maxsteps > all.maxsteps) { all.maxsteps = d.maxsteps; all.argmax = d.argmax; }
          } }
    fclose(lf);
    size_t nseen = 0; for (size_t c = 0; c < nch; c++) nseen += done[c];
    uint64_t odd = HI / 2 - LO / 2, acc = all.dropI + all.dropP + all.leafn;
    int ok = nseen == nch && acc == odd && all.fails == 0;
    lf = fopen(logf, "a"); if (!lf) { perror(logf); return 3; }
    // 照合に落ちたら行頭を ALLDONE にしない（grep ^ALLDONE で完了と読まれないように）
    fprintf(lf, "%s %s| chunks=%zu/%zu odd=%llu accounted=%llu %s maxsteps=%ld at n=%llu | pre: ", ok ? "ALLDONE" : "ALLFAIL", tag, nseen, nch,
            (unsigned long long)odd, (unsigned long long)acc, ok ? "OK" : "MISMATCH", all.maxsteps, (unsigned long long)all.argmax);
    print_stats(lf, &pre, 0, 0);
    { int werr = ferror(lf); werr |= fclose(lf) != 0; if (werr) { perror(logf); return 3; } }
    free(done);
    return ok ? 0 : 2;
}
