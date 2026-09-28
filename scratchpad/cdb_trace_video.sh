#!/bin/sh
# usage: cdb_trace.sh "sym1 sym2 ..." [seconds] [extra halo args] [max output lines]
# Sets a logging breakpoint on each C symbol (first stack args dumped) in the child process the loader relaunches,
# and runs the standalone exe under cdb. Keystone.dll's own thread AV (normally swallowed) ends that thread.
cd /c/Users/Liam-/halo-re/build/standalone || exit 1
child=/c/Users/Liam-/halo-re/scratchpad/cdb_child.txt
first=/c/Users/Liam-/halo-re/scratchpad/cdb_first.txt
: > $child
for s in $1; do
  a=$(grep -m1 " _$s " halo_rebuilt.map | awk '{print $3}')
  [ -z "$a" ] && a=$(grep -m1 " _$s@" halo_rebuilt.map | awk '{print $3}')
  [ -z "$a" ] && { echo "no symbol $s"; continue; }
  echo "bp $a \".echo HIT_$s; dd esp+4 L4; kb 3; g\"" >> $child
done
[ -n "$CDB_EXTRA" ] && printf '%s
' "$CDB_EXTRA" >> $child
ks='.if (@eip >= keystone & @eip < keystone+0x300000) { .echo KEYSTONE_AV_thread_ended; r eip = ntdll!RtlExitUserThread; gh } .else { r; kb 12; .kill; qd }'
echo "sxe -c \"$ks\" -c2 \"$ks\" av" >> $child
echo 'g' >> $child
cat > $first <<EOF
sxe -c "g" ibp
sxe -c "\$\$< C:\\\\Users\\\\Liam-\\\\halo-re\\\\scratchpad\\\\cdb_child.txt" cpr
g
EOF
HALO_STANDALONE_NOBOX=1 HALO_STANDALONE_KEYS="$CDB_KEYS" timeout ${2:-60} "/c/Program Files (x86)/Windows Kits/10/Debuggers/x86/cdb.exe" -G -o \
  -cf "C:\\Users\\Liam-\\halo-re\\scratchpad\\cdb_first.txt" ./halo_rebuilt.exe -window $3 \
  > /c/Users/Liam-/halo-re/scratchpad/cdb_run.txt 2>&1
taskkill //F //IM halo_rebuilt.exe > /dev/null 2>&1
grep -v "ModLoad\|natvis\|^\*\*\*\|Symbol search\|^$\|Repository\|>>>>\|Nuget\|Extension\|UseExperimental\|EnableRedirect" \
  /c/Users/Liam-/halo-re/scratchpad/cdb_run.txt | sed -n '/HIT_\|KEYSTONE\|Access violation\|^eax\|^eip\|ChildEBP\|^[0-9a-f]\{8\} [0-9a-f]\{8\} /p' | head -${4:-80}
