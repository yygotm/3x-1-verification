#!/bin/bash
# チェックポイント経路と引数検査の試験（各ケースを期待値と照合し、1 つでもずれたら exit 1）。
# 照合するもの: 終了コード、log の DONE/FAIL 行の区画の並び、最後の ALLDONE/ALLFAIL 行の行頭と照合結果、ALLDONE 行の数。
# Docker 内で実行（bash t3.sh [バイナリ]）。先に ./t2.sh（ビルドと回帰試験）を通しておく。
set -u
B=${1:-./verify2}
NG=0
ck() { if [ "$2" = "$3" ]; then echo "  ok  $1: $3"; else echo "  NG  $1: expected '$2' got '$3'"; NG=$((NG + 1)); fi; }
run() { "$B" "$@" >t3.out 2>t3.err; echo $?; }
ids() { grep -E '^(DONE|FAIL) ' t3.log | cut -d' ' -f1-3 | tr '\n' ';'; }                  # 区画の並び
lastall() { grep -E '^ALL(DONE|FAIL) ' t3.log | tail -1 | sed -E 's/^(ALL[A-Z]+) .*\| chunks=/\1 chunks=/; s/ \| pre:.*//'; }
nall() { grep -c '^ALLDONE ' t3.log; }
R="1 4194304 0 10 10 2"                                                                   # nfront=76
OKLINE="ALLDONE chunks=4/4 odd=2097152 accounted=2097152 OK maxsteps=241 at n=1556709"
rm -f t3.log t3x.log; rm -rf t3dir

echo "== single run (mode 0): stats match expected baseline"
ck exit 0 "$(run $R)"
ck stats "nodes=70557 dropI=1959628 dropP=80540 leafn=56984" "$(head -1 t3.out | grep -o 'nodes=[0-9]* dropI=[0-9]* dropP=[0-9]* leafn=[0-9]*')"
ck accounted OK "$(grep -o 'OK$\|MISMATCH$' t3.out)"
echo "== single run with BENCH_STRIDE=2 (still allowed without chunk)"
ck exit 0 "$(BENCH_STRIDE=2 run $R)"

echo "== checkpoint chunk=20 (4 chunks): fresh"
ck exit 0 "$(run $R 20 t3.log)"
ck chunks "DONE 0 20;DONE 20 40;DONE 40 60;DONE 60 76;" "$(ids)"
ck alldone "$OKLINE" "$(lastall)"
echo "== resume: remove 'DONE 20 40' and ALLDONE, re-run (only chunk 20..40 runs)"
grep -v '^DONE 20 40 \|^ALLDONE' t3.log > t3.tmp; mv t3.tmp t3.log
ck exit 0 "$(run $R 20 t3.log)"
ck chunks "DONE 0 20;DONE 40 60;DONE 60 76;DONE 20 40;" "$(ids)"
ck alldone "$OKLINE" "$(lastall)"
echo "== all done already: nothing runs, tally from log"
ck exit 0 "$(run $R 20 t3.log)"
ck chunks "DONE 0 20;DONE 40 60;DONE 60 76;DONE 20 40;" "$(ids)"
ck n_alldone 2 "$(nall)"
echo "== tamper: leafn of one DONE line changed -> ALLFAIL ... MISMATCH, exit 2"
grep -v '^ALL' t3.log | sed '0,/leafn=/s/leafn=/leafn=9/' > t3.tmp; mv t3.tmp t3.log
ck exit 2 "$(run $R 20 t3.log)"
ck allfail "ALLFAIL chunks=4/4 odd=2097152 accounted=2997152 MISMATCH maxsteps=241 at n=1556709" "$(lastall)"
ck n_alldone 0 "$(nall)"
echo "== chunk size changed (25): old lines rejected, all chunks re-run, tally uses only matching lines"
grep -v '^ALL' t3.log > t3.tmp; mv t3.tmp t3.log
ck exit 0 "$(run $R 25 t3.log)"
ck chunks "DONE 0 20;DONE 40 60;DONE 60 76;DONE 20 40;DONE 0 25;DONE 25 50;DONE 50 75;DONE 75 76;" "$(ids)"
ck alldone "$OKLINE" "$(lastall)"

echo "== chunk = nfront (76) and chunk = 2^64-1: one chunk, no overflow (#2)"
for c in 76 18446744073709551615; do
  rm -f t3.log
  ck "exit c=$c" 0 "$(run $R $c t3.log)"
  ck "chunks c=$c" "DONE 0 76;" "$(ids)"
  ck "alldone c=$c" "ALLDONE chunks=1/1 odd=2097152 accounted=2097152 OK maxsteps=241 at n=1556709" "$(lastall)"
done
echo "== nfront=0 ([2,3) has no odd n): log is created, ALLDONE 0/0 (#4)"
rm -f t3.log
ck exit 0 "$(run 2 3 0 22 10 2 1 t3.log)"
ck chunks "" "$(ids)"
ck alldone "ALLDONE chunks=0/0 odd=0 accounted=0 OK maxsteps=0 at n=0" "$(lastall)"

echo "== log cannot be opened: exit 3 before computing (#4)"
mkdir t3dir
ck "exit dir" 3 "$(run $R 20 t3dir)"
ck "exit nodir" 3 "$(run $R 20 t3nodir/x.log)"
ck "setup not run" 0 "$(grep -c '^LO=' t3.err)"
echo "== log write fails (file size limit, EFBIG): exit 3, the chunk is not recorded (#6)"
rm -f t3.log; run $R 20 t3.log >/dev/null
{ for i in $(seq 40); do echo "# padding (not a DONE line) to exceed the size limit"; done; grep -v '^ALL' t3.log; } > t3.full   # 4 DONE 行は 890 バイトで上限に届かないので詰め物を足す
ck "precondition size>1024" 1 "$([ "$(stat -c %s t3.full)" -gt 1024 ] && echo 1 || echo 0)"
lim() { ( trap '' XFSZ; ulimit -f 1; "$B" "$@" >/dev/null 2>&1; echo $? ); }            # 1024 バイトを超える書き込みは EFBIG
# (a) DONE 行が書けない: 区画幅が違うので全区画を実行する → 最初の DONE で失敗
cp t3.full t3.log; before=$(md5sum < t3.log)
ck "exit DONE-write" 3 "$(lim $R 25 t3.log)"
ck "log unchanged" "$before" "$(md5sum < t3.log)"
# (b) ALLDONE 行が書けない: 全区画が済んでいて、最後の集計行だけ書く
cp t3.full t3.log; before=$(md5sum < t3.log)
ck "exit ALLDONE-write" 3 "$(lim $R 20 t3.log)"
ck "log unchanged" "$before" "$(md5sum < t3.log)"

echo "== bad args: all must be rejected with exit 1, and no log is created"
bad() { ck "$*" 1 "$(run "$@")"; }
bad 1 100 3
bad 1 100 0 22 10 2 0 t3x.log
bad 100 100 0
bad 1 100 0 22 12 2
bad 1 100 0 22 10 0
bad 1x 100 0
bad +1 100 0
bad " 1" 100 0
bad 1 100000 0 10 10 2 -1 t3x.log                                                        # #2: strtoull が 2^64-1 にする
bad 1 100000 0 10 10 2 18446744073709551616 t3x.log                                      # 2^64（ERANGE）
bad 1 100000 0 10 10 2 0                                                                 # #3: logfile の書き忘れ
bad 1 100000 0 10 10 2 20                                                                # #3: 同上
bad 1 100000 0 10 10 2 20 t3x.log extra                                                  # 引数が多すぎる
bad 1 100000 2 10 10 2 20 t3x.log                                                        # mode 2 とチェックポイントの併用
ck "BENCH_STRIDE=0" 1 "$(BENCH_STRIDE=0 run 1 100 0)"
ck "BENCH_STRIDE=-1" 1 "$(BENCH_STRIDE=-1 run 1 100 0)"                                  # #2
ck "BENCH_STRIDE=2^30+1" 1 "$(BENCH_STRIDE=1073741825 run 1 100 0)"
ck "BENCH_STRIDE=2 + chunk" 1 "$(BENCH_STRIDE=2 run 1 100000 0 10 10 2 20 t3x.log)"     # #3
ck "no t3x.log" 0 "$([ -e t3x.log ] && echo 1 || echo 0)"

rm -rf t3.log t3x.log t3.tmp t3.full t3.out t3.err t3dir
if [ $NG -eq 0 ]; then echo "t3: all ok"; else echo "t3: $NG NG"; exit 1; fi
