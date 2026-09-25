#!/bin/bash
# 査読依頼 (b) の全体試験（各ケースを照合し、ずれたら exit 1）。Docker 内で実行（bash bench_b.sh）。先に ./t2.sh（verify2 のビルド）を通しておく。
# verify2_nocm = class_drops の巡回最小元の除外ループを外した変異。verify2.c から -DNO_CM_EXCL で作る（凍結コピー verify2_nocm.c はもう使わない）。
# 照合するもの: 両方とも exit 0（mode 2 なら selfcheck_bad=0・数え上げ OK・fails=0 を含む）、かつ標準出力（time・rate と argmax の at n= を除く）が一致。
# 自己検査モード（mode 2）で、落としたクラスの全員・止めた数・飛ばした数を素朴な前向き計算で照合する。
# 1, 5, 17 が内部に来る深さは m=5 で j=1,2、m=17 で j=1..4 なので、F=1..4 の浅い分割も含める。
# 引数でバイナリを渡すと、verify2_nocm の代わりにそれを原本と照合する（変異試験 mut.sh 用。ビルドはしない）。
set -u
M=${1:-}
if [ -z "$M" ]; then gcc -O3 -march=native -fopenmp -Wall -DNO_CM_EXCL -o verify2_nocm verify2.c -lm || exit 1; M=./verify2_nocm; fi
md5sum verify2 "$M" verify2.c
NG=0
cmp_run() {   # 変異と原本を同じ引数で実行して照合する
  # maxsteps が同点のとき argmax（at n=）はスレッドの順で変わるため比べない
  "$M" "$@" 2>/dev/null | sed 's/ time=.*//; s/ at n=[0-9]*//' > bb_mut.out; em=${PIPESTATUS[0]}
  "./verify2" "$@" 2>/dev/null | sed 's/ time=.*//; s/ at n=[0-9]*//' > bb_orig.out; eo=${PIPESTATUS[0]}
  if [ $em -eq 0 ] && [ $eo -eq 0 ] && cmp -s bb_mut.out bb_orig.out; then r=ok; else r=NG; NG=$((NG + 1)); fi
  echo "  $r  mode$3 $*: exit mutant=$em orig=$eo, stdout $(cmp -s bb_mut.out bb_orig.out && echo same || echo DIFFERENT)"
  sed 's/^/        /' bb_orig.out
  [ $r = NG ] && diff bb_mut.out bb_orig.out | sed 's/^/        diff: /'
}
echo "== mode 2 (selfcheck): mutant vs orig"
for args in "1 4194304 2 10 10 2" "1 4194304 2 4 10 1" "1 1048576 2 2 10 16" "1 20000000 2 16 10 16" "3 1000000 2 6 10 2" "4 1000000 2 6 10 2" "1 64 2 1 10 1" "2 64 2 1 10 1" "1 40 2 2 10 2" "17 100000 2 4 10 2" "5 100000 2 3 10 2" "1 100000 2 1 10 1"; do
  cmp_run $args
done
# 本番モードの統計の一致（落とし方が同じなら nodes/dropI/dropP/leafn/skipn/traced/stopP/maxsteps が一致する）
echo "== mode 0: mutant vs orig"
for args in "1 4194304 0 10 10 2" "1 20000000 0 16 10 16" "1 1048576 0 2 10 16" "1 1099511627776 0 22 10 16"; do
  cmp_run $args
done
rm -f bb_mut.out bb_orig.out
date -u
if [ $NG -eq 0 ]; then echo "bench_b: all ok"; else echo "bench_b: $NG NG"; exit 1; fi
