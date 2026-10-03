# halo_rebuilt.exe: the rewritten game (src/) with the standalone loader, from committed sources only.
#
#   cmake -S . -B build/cmake -G "Visual Studio 17 2022" -A Win32
#   cmake --build build/cmake --config Release --parallel
# or from an "x86 Native Tools Command Prompt":
#   cmake -S . -B build/cmake-ninja -G Ninja && cmake --build build/cmake-ninja
#
# Needs Visual Studio 2022 (C and MASM, x86) and the DirectX SDK (June 2010) for d3dx9. The exe runs against an
# installed Halo PC (maps, binkw32.dll, vorbisfile.dll): HALO_FOLDER. Graphics need the converted shaders\fx.bin
# (python tools/convert_fx.py) in override\shaders\ next to the exe: HALO_FX_OVERRIDE, copied after the build.
#
# A new engine global is bound through halo::<module>::vars() (standalone/data/link/<module>_vars.hpp names the link
# symbol, standalone/data/*.cpp defines it).
cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)   # CMAKE_MSVC_RUNTIME_LIBRARY
project(halo C CXX)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "halo_rebuilt is 32-bit MSVC only: use -A Win32 (Visual Studio) or an x86 tools prompt (Ninja)")
endif()

if(DEFINED ENV{DXSDK_DIR})
    file(TO_CMAKE_PATH "$ENV{DXSDK_DIR}" _dxsdk_default)
else()
    set(_dxsdk_default "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
endif()
set(DXSDK_DIR "${_dxsdk_default}" CACHE PATH "DirectX SDK (June 2010) folder")
set(HALO_FOLDER "C:/Program Files (x86)/Microsoft Games/Halo" CACHE PATH "Installed Halo PC folder the exe runs against")
set(HALO_FX_OVERRIDE "${CMAKE_SOURCE_DIR}/build/standalone/override/shaders/fx.bin" CACHE FILEPATH
    "shaders\\fx.bin converted by tools/convert_fx.py, copied to override/shaders/ next to the exe")
if(NOT EXISTS "${DXSDK_DIR}/Include/d3dx9.h")
    message(FATAL_ERROR "DirectX SDK not found at DXSDK_DIR=${DXSDK_DIR}")
endif()

# One set of flags for every configuration (/Od keeps the generated code simple; no /RTC)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_C_FLAGS_${config} "")
    set(CMAKE_CXX_FLAGS_${config} "")
    set(CMAKE_EXE_LINKER_FLAGS_${config} "/INCREMENTAL:NO")
endforeach()
# Debug adds symbols only (a PDB for breakpoints; /Zi does not change the generated code)
set(CMAKE_C_FLAGS_DEBUG "/Zi /FS")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/INCREMENTAL:NO /DEBUG")

# ---- the rewritten game functions, one file each
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*/*.c" "${CMAKE_SOURCE_DIR}/src/*/*.cpp")
add_library(halo_game OBJECT ${GAME_SOURCES})
# the game code is compiled as C++20. No exceptions, no RTTI.
set(CXX_GAME_SOURCES ${GAME_SOURCES})
list(FILTER CXX_GAME_SOURCES EXCLUDE REGEX "/src/gamespy/")   # GameSpy stays vendored C
set_source_files_properties(${CXX_GAME_SOURCES} PROPERTIES LANGUAGE CXX)
target_include_directories(halo_game PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/standalone/data" "${CMAKE_SOURCE_DIR}/types" "${DXSDK_DIR}/Include")
target_compile_options(halo_game PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/std:c++20> $<$<COMPILE_LANGUAGE:CXX>:/permissive-> $<$<COMPILE_LANGUAGE:CXX>:/GR-> /W3 /Od /GS- /Oy- /Gy /wd4996
    "/FI${CMAKE_SOURCE_DIR}/include/halo/msvc_compat.h")

# ---- import libraries for the third-party DLLs in the Halo folder (no .lib ships with them), delay-loaded
set(THIRD_PARTY_DLLS binkw32 vorbisfile)
set(THIRD_PARTY_LIBS)
foreach(dll ${THIRD_PARTY_DLLS})
    set(lib "${CMAKE_CURRENT_BINARY_DIR}/${dll}.lib")
    add_custom_command(OUTPUT "${lib}"
        COMMAND "${CMAKE_AR}" /nologo /machine:x86 "/def:${CMAKE_SOURCE_DIR}/standalone/libs/${dll}.def" "/out:${lib}"
        DEPENDS "${CMAKE_SOURCE_DIR}/standalone/libs/${dll}.def" VERBATIM)
    list(APPEND THIRD_PARTY_LIBS "${lib}")
endforeach()
add_custom_target(third_party_import_libs DEPENDS ${THIRD_PARTY_LIBS})

# ---- the standalone exe: loader, data image, link tables
set(SA "${CMAKE_SOURCE_DIR}/standalone")
# the engine globals as extern "C" definitions (standalone/data/*.cpp), compiled with the game code's flags and headers
file(GLOB DATA_SOURCES CONFIGURE_DEPENDS "${SA}/data/*.cpp")
add_library(halo_data OBJECT ${DATA_SOURCES})
target_include_directories(halo_data PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/types" "${DXSDK_DIR}/Include")
target_compile_options(halo_data PRIVATE /std:c++20 /permissive- /GR- /W3 /Od /GS- /Oy- /Gy /wd4996
    "/FI${CMAKE_SOURCE_DIR}/include/halo/msvc_compat.h")
set(SUPPORT_SOURCES
    "${SA}/loader.cpp"
    "${SA}/d3dx_compat.cpp"
    "${SA}/bridges.cpp")
add_executable(halo_rebuilt WIN32 ${SUPPORT_SOURCES}
    $<TARGET_OBJECTS:halo_game> $<TARGET_OBJECTS:halo_data>)
add_dependencies(halo_rebuilt third_party_import_libs)
target_compile_options(halo_rebuilt PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/std:c++20> $<$<COMPILE_LANGUAGE:CXX>:/permissive-> $<$<COMPILE_LANGUAGE:CXX>:/GR->)

set_source_files_properties(${SUPPORT_SOURCES} PROPERTIES COMPILE_OPTIONS "/O2;/GS-")
set_source_files_properties("${SA}/d3dx_compat.cpp" PROPERTIES INCLUDE_DIRECTORIES "${DXSDK_DIR}/Include")
target_include_directories(halo_rebuilt PRIVATE "${SA}" "${CMAKE_SOURCE_DIR}/include")

file(TO_NATIVE_PATH "${HALO_FOLDER}" _halo_folder)
string(REPLACE "\\" "\\\\" _halo_folder "${_halo_folder}")
set_source_files_properties("${SA}/loader.cpp" PROPERTIES COMPILE_DEFINITIONS "HALO_FOLDER=\"${_halo_folder}\"")

target_link_directories(halo_rebuilt PRIVATE "${DXSDK_DIR}/Lib/x86")
target_link_libraries(halo_rebuilt PRIVATE
    kernel32 user32 gdi32 advapi32 ole32 oleaut32 winmm ws2_32 shell32 version shlwapi comdlg32
    dsound dinput8 dxguid d3d9 d3dx9 legacy_stdio_definitions wininet
    ${THIRD_PARTY_LIBS} delayimp)
# the data image sits at the original addresses (0x400000..0x891000), so the exe must load elsewhere, unrelocated
target_link_options(halo_rebuilt PRIVATE /FIXED /DYNAMICBASE:NO /BASE:0x10000000 /SAFESEH:NO /OPT:NOREF /OPT:NOICF
    /LARGEADDRESSAWARE:NO /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcpmt.lib /DELAYLOAD:binkw32.dll /DELAYLOAD:vorbisfile.dll /MAP)

if(EXISTS "${HALO_FX_OVERRIDE}")
    add_custom_command(TARGET halo_rebuilt POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${HALO_FX_OVERRIDE}"
                "$<TARGET_FILE_DIR:halo_rebuilt>/override/shaders/fx.bin" VERBATIM)
else()
    message(WARNING "HALO_FX_OVERRIDE (${HALO_FX_OVERRIDE}) not found: convert shaders\\fx.bin with "
                    "tools/convert_fx.py and put it in override/shaders/ next to halo_rebuilt.exe")
endif()

# ---- offline map reader / validator (tools/map_inspect): header and tag index of the retail .map files, no engine code
add_executable(map_inspect tools/map_inspect/main.cpp tools/map_inspect/map_reader.cpp)
target_include_directories(map_inspect PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_options(map_inspect PRIVATE /std:c++20 /EHsc /W3)
target_compile_definitions(map_inspect PRIVATE _CRT_SECURE_NO_WARNINGS)

