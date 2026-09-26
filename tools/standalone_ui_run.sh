#!/bin/sh
# Boot the standalone exe with the loader's in-process key driver (HALO_STANDALONE_KEYS, see standalone/loader.c)
# and screenshot the window at the given times (seconds after launch) to build/standalone/ui_<t>.png.
# usage: standalone_ui_run.sh "30000:DOWN,1500:ENTER" [total seconds] "35 40 45"
cd /c/Users/Liam-/halo-re || exit 1
python tools/gen_standalone.py > /dev/null && python tools/gen_standalone_link.py 2>&1 | tail -1
cd build/standalone && rm -f halo_standalone.log ui_*.png
(HALO_STANDALONE_NOBOX=1 HALO_STANDALONE_KEYS="$1" timeout ${2:-90} ./halo_rebuilt.exe -window -novideo; echo "exit $?" > exit.txt) &
last=0
for t in ${3:-40}; do
  sleep $((t - last)); last=$t
  powershell -ExecutionPolicy Bypass -File "C:\\Users\\Liam-\\halo-re\\scratchpad\\shot.ps1" -out "C:\\Users\\Liam-\\halo-re\\build\\standalone\\ui_$t.png" 2>/dev/null | head -1
done
wait
cat exit.txt
cd ../.. && python tools/standalone_symbolize.py | tail -n +6 | grep -v "^override" | head -40
