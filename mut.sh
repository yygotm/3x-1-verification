#!/bin/bash
# 変異試験: test_cm と bench_b.sh が「落としてはいけないクラスを落とす」などの誤りを検出できることの確認。
# verify2.c から sed で 1 箇所だけ変えた変異を作り（変わった行数を確かめる）、-DNO_CM_EXCL で
#   - verify2 のバイナリ mut_X（bench_b.sh に原本との照合をさせる）
#   - test_cm（LC=10、-DSRC で変異を取り込む）
# を作って実行し、終了コードを期待表と照合する。1 つでもずれたら exit 1。Docker 内で実行（bash mut.sh）。
# 期待の "any" は事前に期待を決めていない欄（結果は記録する）。
set -u
CF="-O3 -march=native -fopenmp -Wall"
gcc $CF -o verify2 verify2.c -lm || exit 1                                    # bench_b.sh の原本
md5sum verify2.c test_cm.c bench_b.sh mut.sh
NG=0
# 名前 @ sed 式 @ 変わる行数（diff の < と > の数） @ test_cm の期待終了コード @ bench_b の期待終了コード @ 内容（区切りは @。sed 式に | を含むため）
MUTS=(
  "none@@0@0@0@変異なし（試験側の変更の回帰確認。原本との照合）"
  "I_LE@s/x0 < n0 && x1 < n1/x0 <= n0 \&\& x1 <= n1/@2@2@1@(I) を <= に（x = n の員を落とす: 巡回最小元のクラス）"
  "P_NOGUARD@s/if (TREE_P && MODE != 1 && a >= L) {/if (TREE_P \&\& MODE != 1) {/@2@any@1@(P) の a >= L の条件を外す（経路がクラスで一定でないのに落とす）"
  "PTEST_LE@s/return lhs < rhs && lhs + pc\[xr\] < rhs;/return lhs <= rhs \&\& lhs + pc[xr] <= rhs;/@2@2@1@ptest を <= に（祖先 m' = n を認める）"
  "LEAF_CM@/if (n == 1 || n == 5 || n == 17) continue;/d@1@0@1@葉で巡回最小元を飛ばさない（test_cm は class_drops だけを見るので対象外）"
  "NO_TREE_P@s/^#define TREE_P 1\$/#define TREE_P 0/@2@2@1@木の (P) を使わない（落とし不足。対照と統計の照合で検出）"
)
printf "%-10s %-12s %-12s %s\n" mutant test_cm bench_b result > mut_summary.txt
for row in "${MUTS[@]}"; do
  IFS='@' read -r name expr nchg etc ebb desc <<< "$row"
  echo "=================== $name: $desc"
  sed "$expr" verify2.c > mut_$name.c
  n=$(diff verify2.c mut_$name.c | grep -c '^[<>]')
  diff verify2.c mut_$name.c
  if [ "$n" != "$nchg" ]; then echo "NG: sed changed $n lines (expected $nchg)"; NG=$((NG + 1)); continue; fi
  gcc $CF -DNO_CM_EXCL -o mut_$name mut_$name.c -lm || { echo "NG: build"; NG=$((NG + 1)); continue; }
  gcc -O2 -fopenmp -Wall -DLC=10 -DNO_CM_EXCL -DSRC="\"mut_$name.c\"" -o test_cm_mut_$name test_cm.c -lm || { echo "NG: build test_cm"; NG=$((NG + 1)); continue; }
  ./test_cm_mut_$name > test_cm_mut_$name.out 2>&1; tc=$?
  grep -E 'NG|TOTAL|RESULT|DROP|j=20 on' test_cm_mut_$name.out | head -12
  bash bench_b.sh ./mut_$name > bench_b_mut_$name.out 2>&1; bb=$?
  grep -E '^  (ok|NG)|^bench_b' bench_b_mut_$name.out
  r=ok
  [ "$etc" = any ] || [ "$etc" = "$tc" ] || r=NG
  [ "$ebb" = "$bb" ] || r=NG
  [ $r = NG ] && NG=$((NG + 1))
  echo "-> $name: test_cm exit=$tc (expect $etc), bench_b exit=$bb (expect $ebb): $r"
  printf "%-10s %-12s %-12s %s\n" $name "$tc(exp $etc)" "$bb(exp $ebb)" $r >> mut_summary.txt
done
cat mut_summary.txt
if [ $NG -eq 0 ]; then echo "mut: all as expected"; else echo "mut: $NG NG"; exit 1; fi
