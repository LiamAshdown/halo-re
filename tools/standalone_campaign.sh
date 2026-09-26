#!/bin/sh
# boot straight into a campaign level (default a10) through -exec
m=${2:-a10}
printf "map_name %s
" "$m" > /c/Users/Liam-/halo-re/build/standalone/campaign.txt
HALO_ARGS="-exec C:\Users\Liam-\halo-re\build\standalone\campaign.txt" sh /c/Users/Liam-/halo-re/tools/standalone_boot.sh ${1:-60}
