#!/bin/sh
# Boot the standalone exe, wait for the main menu, then drive the UI with keystrokes (scratchpad/keys.ps1 steps)
# and screenshot after each step. usage: standalone_ui_run.sh "ENTER,WAIT:2000,DOWN" [total seconds] [menu wait seconds]
cd /c/Users/Liam-/halo-re || exit 1
python tools/gen_standalone.py > /dev/null && python tools/gen_standalone_link.py 2>&1 | tail -1
cd build/standalone && rm -f halo_standalone.log ui_*.png
(HALO_STANDALONE_NOBOX=1 timeout ${2:-120} ./halo_rebuilt.exe -window -novideo; echo "exit $?" > exit.txt) &
sleep ${3:-30}
powershell -ExecutionPolicy Bypass -File "C:\\Users\\Liam-\\halo-re\\scratchpad\\keys.ps1" -steps "$1"
wait
cat exit.txt
cd ../.. && python tools/standalone_symbolize.py | tail -n +6 | grep -v "^override" | head -40
