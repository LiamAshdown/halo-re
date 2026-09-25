#!/bin/sh
# usage: tools/callsite.sh <caller> <callee>  -- the caller's declaration + C calls of callee, and the binary call sites
cf=$(ls src/*/$1.c); ef=$(ls src/*/$2.c)
ea=$(grep -o "address 0x[0-9a-f]*" $ef | head -1 | cut -d' ' -f2)
awk '/#if 0/{exit}1' $cf | grep -n "\b$2\b" | grep -v "^[0-9]*:\s*//"
a=$(grep -o "address 0x[0-9a-f]*, size [0-9]*" $cf | head -1); ad=$(echo $a|cut -d' ' -f2|tr -d ,); sz=$(echo $a|cut -d' ' -f4)
objdump -d -M intel --no-show-raw-insn --start-address=$ad --stop-address=$((ad+sz+64)) bin/halo.exe | grep -B${3:-8} -A1 "call   $ea" | cut -f1,2 | sed 's/^ *//'
