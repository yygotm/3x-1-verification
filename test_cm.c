// test_cm.c — 巡回の最小元 1, 5, 17 を含むクラスを class_drops が落とさないことの狙った試験（査読依頼 (b)・レビュー #13）
// verify2.c を取り込む。-DNO_CM_EXCL で class_drops の巡回最小元の除外ループを外した変異になる（同じソースから作る。結果は同じになるはず）。
// ビルド: gcc -O2 -fopenmp -Wall [-DLC=6|8|10|12] [-DNO_CM_EXCL] -o test_cm test_cm.c -lm
// 内容（どれかが期待と違えば exit 2）:
//   1. 経路の表の a, k, c の最大値（class_drops の桁あふれの注記 k < LC·log2 3 の確認）
//   2. m ∈ {1, 5, 17}、深さ j = 1..44 について、m を含むクラス B = m mod 2^j の (a, e, r) を dfs と同じ手順で求め、
//      m が範囲に入る [LO, HI) を多数作って class_drops を直接呼ぶ。期待: drop は 0 件。加えて
//      (i)  m が内部（両端でない）に来るのは 2^j <= m のときだけで、そのとき a <= j < L なので (P) の枝（a >= L）に入らない → 内部で a >= L が 0 件
//      (ii) 全ての j で ptest(C^j(m), m, C^j(m) mod 3^L) が偽（a に関係なく。LC ごとに表が違うので各 LC のビルドで）
//   3. 対照: 巡回最小元を含まないクラスは (I)・(P) で落ちる（試験が空でないことの確認）。期待: B=3 j=2 で (I)、j=20 の全クラスで (I)>0 かつ (P)>0
//   4. (iii) 巡回の各元 x から逆向きの経路（E(y) = 2y、y ≡ 1 mod 3 なら O(y) = (2y+1)/3）を k <= 40 まで全て列挙し（表の最良経路に限らない）、
//      祖先 m' < m（m はその巡回の最小元）が無いことを確かめる。(P) の祖先はこの列挙に含まれる（表の経路は k <= 19）。
#include <stdio.h>
#define main verify2_main
#ifndef SRC
#define SRC "verify2.c"                             // 変異試験（mut.sh）では -DSRC='"mut_X.c"' で差し替える
#endif
#include SRC
#undef main

// dfs / split と同じ手順で、深さ j のクラス B（奇数、B < 2^j）の a, e, r を求める（根は B=1, j=1, a=1, e=1, r=1）
static void class_params(uint64_t B, int j, int *pa, u128 *pe, uint64_t *pr) {
    int a = 1; u128 e = 1; uint64_t r = 1 % M3;
    for (int i = 1; i < j; i++) {
        int b = (B >> i) & 1;
        uint64_t rc = b ? (r + P3M[a]) % M3 : r;
        u128 ec = b ? e + P3[a] : e;
        if (ec & 1) { e = (3 * ec - 1) >> 1; a++; r = (3 * rc + M3 - 1) % M3 * INV2 % M3; } else { e = ec >> 1; r = rc * INV2 % M3; }
    }
    *pa = a; *pe = e; *pr = r;
}
static const char *s128(u128 v) {                  // u128 を 10 進で（表示用）
    static char buf[4][48]; static int w; char *p = buf[w = (w + 1) & 3] + 47; *p = 0;
    do { *--p = '0' + (int)(v % 10); v /= 10; } while (v);
    return p;
}

// 4. の逆向き列挙: y から k 歩さかのぼった祖先を全て調べる。m' < m なら数える。m' から前向きに k 歩で x に着くことも照合する
static long bnodes, bbelow, bfwdbad;
static void back(u128 y, int k, u128 x, uint64_t m) {
    bnodes++;
    if (y < m) { bbelow++; if (bbelow <= 5) printf("   ANCESTOR BELOW MIN m=%llu: m'=%s reaches x=%s in %d steps\n", (unsigned long long)m, s128(y), s128(x), k); }
    if (fwd(y, k) != x) bfwdbad++;
    if (k == 40) return;
    back(2 * y, k + 1, x, m);
    if (y % 3 == 1) back((2 * y + 1) / 3, k + 1, x, m);
}

int main(void) {
    MODE = 0; L = LC; F = 22; LEAFN = 16;
    P3[0] = 1; for (int i = 1; i <= 80; i++) P3[i] = 3 * P3[i - 1];
    build_paths(); build_jump();
    long fails = 0;
#ifdef NO_CM_EXCL
    printf("target: %s -DNO_CM_EXCL (mutant: cycle-minimum exclusion loop removed from class_drops), LC=%d\n", SRC, LC);
#else
    printf("target: %s (with the exclusion loop), LC=%d\n", SRC, LC);
#endif
    // 1. 表の最大値
    int kmax = 0, amax = 0; uint64_t cmax = 0; long npath = 0;
    for (uint64_t r = 0; r < M3; r++) if (ptab[r].a) { npath++; if (ptab[r].k > kmax) kmax = ptab[r].k; if (ptab[r].a > amax) amax = ptab[r].a; if (ptab[r].c > cmax) cmax = ptab[r].c; }
    printf("1. path table LC=%d: residues with a path=%ld/%llu  max a=%d  max k=%d  max c=%llu   (bound: k < LC*log2(3)=%.3f, c <= 3^a(2^k-1))\n",
           LC, npath, (unsigned long long)M3, amax, kmax, (unsigned long long)cmax, LC * log2(3.0));
    if (!(kmax < LC * log2(3.0)) || kmax > 19) { printf("   NG: k bound\n"); fails++; }

    // 2. 巡回最小元を含むクラス
    static const uint64_t cm[3] = {1, 5, 17};
    long tot = 0, tot_int = 0, drops = 0, drops_int = 0, int_aL = 0, ptrue = 0;
    printf("2. classes containing a cycle minimum m (B = m mod 2^j, t* = m >> j; interior means tlo < t* < t1)\n");
    for (int ci = 0; ci < 3; ci++) {
        uint64_t m = cm[ci];
        for (int j = 1; j <= 44; j++) {
            uint64_t B = m & (((uint64_t)1 << j) - 1), ts = m >> j;
            int a; u128 e; uint64_t r; class_params(B, j, &a, &e, &r);
            { u128 x = B; int oa = 0;                                // 検算: e = C^j(B)、a = 奇数の歩数、r = e mod 3^L
              for (int i = 0; i < j; i++) { if (x & 1) { x = (3 * x - 1) >> 1; oa++; } else x >>= 1; }
              if (x != e || oa != a || r != (uint64_t)(e % M3)) { printf("class_params mismatch m=%llu j=%d\n", (unsigned long long)m, j); return 2; } }
            u128 xs = e + P3[a] * ts;                                 // x(t*) = C^j(m)
            if (fwd(m, j) != xs) { printf("x(t*) mismatch m=%llu j=%d\n", (unsigned long long)m, j); return 2; }
            int pt = ptest(xs, m, (uint64_t)(xs % M3));               // (ii) 巡回最小元そのものに (P) が成り立つか（a に関係なく。成り立たないはず）
            if (pt) { printf("   NG: PTEST TRUE at cycle minimum m=%llu j=%d\n", (unsigned long long)m, j); ptrue++; }
            long cnt = 0, cint = 0, dr = 0, drint = 0, cintaL = 0;
            // 範囲の候補: LO = 1..m の全て、HI = m+1 .. m+2^(j+1)+2 の全て（上限 4098）と、幅の大きいもの（HI <= 2^62−1 に抑える）
            static const uint64_t wide[] = {1000, 100000, 10000000, 1000000000, (uint64_t)1 << 40, (uint64_t)1 << 50, ((uint64_t)1 << 62) - 1 - 17};
            uint64_t nk = ((uint64_t)1 << (j + 1)) + 2; if (nk > 4098) nk = 4098;
            for (uint64_t lo = 1; lo <= m; lo++) {
                for (uint64_t q = 0; q < nk + 7; q++) {
                    uint64_t hi = q < nk ? m + 1 + q : wide[q - nk] + m;
                    if (hi <= m) continue;
                    LO = lo; HI = hi;
                    uint64_t tlo, thi; trange(B, j, &tlo, &thi);
                    if (tlo >= thi || ts < tlo || ts >= thi) { printf("m not in range?? m=%llu j=%d LO=%llu HI=%llu\n", (unsigned long long)m, j, (unsigned long long)lo, (unsigned long long)hi); return 2; }
                    int interior = tlo < ts && ts < thi - 1;
                    int d = class_drops(B, j, a, e, r, tlo, thi);
                    cnt++; cint += interior; cintaL += interior && a >= L;
                    if (d) { dr++; drint += interior; if (dr <= 5) printf("   DROP m=%llu j=%d LO=%llu HI=%llu tlo=%llu t1=%llu d=%d\n", (unsigned long long)m, j, (unsigned long long)lo, (unsigned long long)hi, (unsigned long long)tlo, (unsigned long long)(thi - 1), d); }
                }
            }
            if (cintaL) printf("   NG: interior with a >= L at m=%llu j=%d (%ld ranges)\n", (unsigned long long)m, j, cintaL);
            printf("   m=%2llu j=%2d B=%-8llu a=%2d e=%-24s t*=%-6llu x(t*)-m=%-24s a>=L=%s ptest(x(t*),m)=%d ranges=%6ld interior=%6ld drops=%ld drops_interior=%ld\n",
                   (unsigned long long)m, j, (unsigned long long)B, a, s128(e), (unsigned long long)ts, s128(xs - m), a >= L ? "yes" : "no ", pt, cnt, cint, dr, drint);
            tot += cnt; tot_int += cint; drops += dr; drops_int += drint; int_aL += cintaL;
        }
    }
    printf("   TOTAL ranges=%ld interior=%ld drops=%ld drops_interior=%ld interior_with_a>=L=%ld ptest_true_at_minimum=%ld (of %d)\n",
           tot, tot_int, drops, drops_int, int_aL, ptrue, 3 * 44);
    fails += drops + int_aL + ptrue;
    if (tot_int == 0) { printf("   NG: no interior case was generated\n"); fails++; }

    // 3. 対照: 巡回最小元を含まないクラスは落ちる
    printf("3. controls (classes without a cycle minimum)\n");
    { int a; u128 e; uint64_t r, tlo, thi; class_params(3, 2, &a, &e, &r); LO = 1; HI = 1000; trange(3, 2, &tlo, &thi);
      int d = class_drops(3, 2, a, e, r, tlo, thi);
      printf("   B=3 j=2 (n=3+4t, x=%s+%llu t) on [1,1000): class_drops=%d (expect 1: (I))\n", s128(e), (unsigned long long)P3[a], d);
      if (d != 1) { printf("   NG: control B=3\n"); fails++; } }
    { long c1 = 0, c2 = 0, c0 = 0; uint64_t firstP = 0;
      LO = (uint64_t)1 << 40; HI = ((uint64_t)1 << 40) + ((uint64_t)1 << 30);
      for (uint64_t B = 1; B < ((uint64_t)1 << 20); B += 2) {
          int a; u128 e; uint64_t r, tlo, thi; class_params(B, 20, &a, &e, &r); trange(B, 20, &tlo, &thi);
          if (tlo >= thi) continue;
          int d = class_drops(B, 20, a, e, r, tlo, thi);
          if (d == 1) c1++; else if (d == 2) { c2++; if (!firstP) firstP = B; } else c0++;
      }
      printf("   all odd B at j=20 on [2^40, 2^40+2^30): (I)=%ld (P)=%ld none=%ld  (first (P) class B=%llu)\n", c1, c2, c0, (unsigned long long)firstP);
      if (!(c1 > 0 && c2 > 0)) { printf("   NG: control j=20\n"); fails++; } }

    // 4. (iii) 巡回の各元からの逆向き経路の全列挙（k <= 40）
    printf("4. all backward paths (k <= 40) from every cycle element: ancestors below the cycle minimum\n");
    for (int ci = 0; ci < 3; ci++) {
        uint64_t m = cm[ci]; u128 x = m; int len = 0;
        long n0 = bnodes, b0 = bbelow, f0 = bfwdbad;
        do { back(x, 0, x, m); len++; x = (x & 1) ? (3 * x - 1) >> 1 : x >> 1; if (x < m) { printf("   NG: %llu is not a cycle minimum\n", (unsigned long long)m); fails++; break; } } while (x != m && len < 1000);
        printf("   m=%2llu cycle length=%d nodes=%ld below_m=%ld forward_mismatch=%ld\n", (unsigned long long)m, len, bnodes - n0, bbelow - b0, bfwdbad - f0);
    }
    fails += bbelow + bfwdbad;

    printf("RESULT: %s (fails=%ld)\n", fails ? "NG" : "ok: no class containing 1, 5 or 17 is dropped; all checks passed", fails);
    return fails ? 2 : 0;
}
