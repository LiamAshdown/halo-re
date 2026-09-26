#!/bin/sh
# first-boot loop: relink the standalone exe, boot it for up to 60 s without the trap box, print the symbolized log
cd /c/Users/Liam-/halo-re || exit 1
[ -f build/standalone/override/shaders/fx.bin ] || { mkdir -p build/standalone/override/shaders && python tools/convert_fx.py "/c/Program Files (x86)/Microsoft Games/Halo/shaders/fx.bin" build/standalone/override/shaders/fx.bin; }
python tools/gen_standalone.py > /dev/null && python tools/gen_standalone_link.py 2>&1 | tail -1
cd build/standalone && rm -f halo_standalone.log
HALO_STANDALONE_NOBOX=1 timeout ${1:-60} ./halo_rebuilt.exe -window -novideo $HALO_ARGS; echo "exit $?"
taskkill //F //IM halo_rebuilt.exe >/dev/null 2>&1
cd ../.. && python tools/standalone_symbolize.py | tail -n +6 | head -60
