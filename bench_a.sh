#!/bin/bash
# 査読依頼 (a)(a') の測定。Docker 内で実行（bash bench_a.sh）。
# verify2 は既定マクロ（-D なし）で事前にビルドする: gcc -O3 -march=native -fopenmp -Wall -o verify2 verify2.c -lm
# stderr は落とさない（SKIP_T=28 の行と LO=... の行をそのまま残す）。
set -x
date -u
md5sum verify2 verify2.c verify
nproc
# (a) [2^40, 2^42) 全数（BENCH_STRIDE 無し）、F=22 L=10、LEAFN=16（本番向け）と LEAFN=2（実行時の既定値）
./verify2 1099511627776 4398046511104 0 22 10 16
./verify2 1099511627776 4398046511104 0 22 10 2
# 同じバイナリ・同じ時間帯で chunk 60（2^36 幅、2p44.log の区画）: 既定値 LEAFN=2 と LEAFN=16、各 2 回
lo=$((15360*268435456)); hi=$((15616*268435456))
./verify2 $lo $hi 0 22 10 2
./verify2 $lo $hi 0 22 10 2
./verify2 $lo $hi 0 22 10 16
./verify2 $lo $hi 0 22 10 16
# 抜き取り（従来の F の条件）: [2^44, 2^46) F=26 L=10 LEAFN=16 BENCH_STRIDE=512、2 回
BENCH_STRIDE=512 ./verify2 17592186044416 70368744177664 0 26 10 16
BENCH_STRIDE=512 ./verify2 17592186044416 70368744177664 0 26 10 16
# (a') 本番設定 F=26 で、間引きなし全数と抜き取りの突き合わせ: [2^44-2^43, 2^44)（2^43 幅）
lo=$(( (1<<44) - (1<<43) )); hi=$((1<<44))
./verify2 $lo $hi 0 26 10 16
BENCH_STRIDE=8 ./verify2 $lo $hi 0 26 10 16
BENCH_STRIDE=512 ./verify2 $lo $hi 0 26 10 16
BENCH_STRIDE=512 ./verify2 $lo $hi 0 26 10 16
date -u
