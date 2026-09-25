#!/bin/bash
# verify2 の回帰試験（自己検査・厳密モードの旧版との一致）とベンチマーク（chunk 60 = [15360*2^28, 15616*2^28)）
set -eo pipefail
gcc -O3 -march=native -fopenmp -Wall -o verify2 verify2.c -lm
echo "== selfcheck [1,2^22)";               ./verify2 1 4194304 2 10 10 2 | tail -2
echo "== selfcheck [2^40, 2^40+2^22)";      ./verify2 1099511627776 1099515822080 2 16 10 2 | tail -2
echo "== selfcheck [2^44, 2^44+2^20)"; ./verify2 17592186044416 17592187092992 2 12 10 2 | tail -2
echo "== selfcheck [2^45, 2^45+2^24) LEAFN=16"; ./verify2 35184372088832 35184388866048 2 12 10 16 | tail -2
echo "== exact [1, 20*2^20)  (old: 273 at 5960769)"; ./verify2 1 20971520 1 16 10 2 | head -1 | grep -o 'fails=.*maxdropdepth=[0-9]*'
if [ "$1" = bench ]; then
  lo=$((15360*268435456)); hi=$((15616*268435456))
  echo "== bench chunk 60 fast ${@:2}"; ./verify2 $lo $hi 0 ${2:-22} ${3:-10} ${4:-2} | head -1
fi
