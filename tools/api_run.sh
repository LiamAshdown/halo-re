#!/bin/sh
# usage: tools/api_run.sh <module>   (resets the working tree except tools/, then converts the module from scratch)
mod=$1
git reset -q
git checkout -- . ':!tools'
rm -f src/$mod/${mod}_api.cpp include/halo/$mod/api.hpp
python tools/api_convert.py $mod | cut -c1-200
git mv src/$mod/${mod}_c_api.cpp src/$mod/${mod}_api.cpp
python tools/api_post.py $mod | tail -1
sed -i 's|target_include_directories(halo_data PRIVATE "${CMAKE_SOURCE_DIR}/types"|target_include_directories(halo_data PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/types"|' CMakeLists.txt
