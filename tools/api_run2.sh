#!/bin/sh
# usage: tools/api_run2.sh <module>   (like api_run.sh, for modules whose extern "C" definitions live in their own files)
mod=$1
git reset -q
git checkout -- . ':!tools'
rm -f include/halo/$mod/api.hpp
python tools/api_convert2.py $mod | cut -c1-300
python tools/api_post.py $mod | tail -1
sed -i 's|target_include_directories(halo_data PRIVATE "${CMAKE_SOURCE_DIR}/types"|target_include_directories(halo_data PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/types"|' CMakeLists.txt
