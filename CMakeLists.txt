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
# Same build as tools/msvc_build.py + tools/gen_standalone_link.py. The generated sources it uses are committed:
# after adding or renaming functions run tools/gen_link_sources.py, after adding globals tools/update_globals.py.
cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)   # CMAKE_MSVC_RUNTIME_LIBRARY
project(halo C ASM_MASM)

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

# One set of flags for every configuration: the game code is compiled like tools/msvc_build.py (/Od keeps a 1:1
# shape with the original functions; no /RTC, whose checks the original code was never written for)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_C_FLAGS_${config} "")
    set(CMAKE_EXE_LINKER_FLAGS_${config} "/INCREMENTAL:NO")
endforeach()
# Debug adds symbols only (a PDB for breakpoints; /Zi does not change the generated code)
set(CMAKE_C_FLAGS_DEBUG "/Zi /FS")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/INCREMENTAL:NO /DEBUG")

# ---- the rewritten game functions, one file each
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*/*.c")
add_library(halo_game OBJECT ${GAME_SOURCES})
target_include_directories(halo_game PRIVATE "${CMAKE_SOURCE_DIR}/types" "${DXSDK_DIR}/Include")
target_compile_options(halo_game PRIVATE /TC /W3 /Od /GS- /Oy- /Gy /wd4996
    "/FI${CMAKE_SOURCE_DIR}/harness/msvc_compat.h")

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
file(GLOB IMAGE_ASM CONFIGURE_DEPENDS "${SA}/image/*.asm")
# engine globals as C definitions (one file per slice, see standalone/globals.asm for the ones still at fixed addresses)
file(GLOB DATA_SOURCES CONFIGURE_DEPENDS "${SA}/data/*.c")
set_source_files_properties(${DATA_SOURCES} PROPERTIES COMPILE_OPTIONS "/TC;/W3;/Od;/GS-;/wd4996;/FI${CMAKE_SOURCE_DIR}/harness/msvc_compat.h"
    INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/types;${DXSDK_DIR}/Include")
set(SUPPORT_SOURCES
    "${SA}/loader.c"
    "${SA}/d3dx_compat.c"
    "${CMAKE_SOURCE_DIR}/harness/x87_shims.c"
    "${SA}/image/pieces.c"
    "${SA}/generated/image_bindings.c")
add_executable(halo_rebuilt WIN32 ${SUPPORT_SOURCES} ${DATA_SOURCES} ${IMAGE_ASM}
    "${SA}/globals.asm" "${SA}/generated/code_entries.asm" "${SA}/bridges.asm"
    $<TARGET_OBJECTS:halo_game>)
add_dependencies(halo_rebuilt third_party_import_libs)

# ---- keep the committed link sources in step with src/ (tools/cmake_regen.py): after the game objects compile and
# before the link, regenerate code_entries.asm / image_bindings.c / pieces.c (new or renamed functions) and add new
# engine globals to globals.asm. Off, or without Python, the committed files are used as they are.
find_package(Python3 COMPONENTS Interpreter)
option(HALO_REGENERATE "Regenerate the link sources and globals.asm from src/ during the build" ${Python3_FOUND})
if(HALO_REGENERATE)
    if(CMAKE_CONFIGURATION_TYPES)   # multi-config (Visual Studio): one list per configuration
        set(_objects_list "${CMAKE_CURRENT_BINARY_DIR}/halo_game_objects_$<CONFIG>.txt")
    else()                          # Ninja / CLion
        set(_objects_list "${CMAKE_CURRENT_BINARY_DIR}/halo_game_objects.txt")
    endif()
    set(_regen_stamp "${CMAKE_CURRENT_BINARY_DIR}/regen.stamp")
    file(GENERATE OUTPUT "${_objects_list}" CONTENT "$<JOIN:$<TARGET_OBJECTS:halo_game>,\n>\n")
    file(GLOB _regen_tools "${CMAKE_SOURCE_DIR}/tools/cmake_regen.py" "${CMAKE_SOURCE_DIR}/tools/gen_link_sources.py"
        "${CMAKE_SOURCE_DIR}/tools/gen_standalone.py" "${CMAKE_SOURCE_DIR}/tools/update_globals.py")
    add_custom_command(OUTPUT "${_regen_stamp}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/cmake_regen.py" "${_objects_list}" "${_regen_stamp}"
        DEPENDS halo_game $<TARGET_OBJECTS:halo_game> "${_objects_list}" ${_regen_tools}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Regenerating link sources and globals from src/" VERBATIM)
    add_custom_target(halo_regenerate DEPENDS "${_regen_stamp}")
    add_dependencies(halo_rebuilt halo_regenerate)
    set_source_files_properties("${SA}/image/pieces.c" "${SA}/generated/image_bindings.c" "${SA}/globals.asm"
        "${SA}/generated/code_entries.asm" PROPERTIES OBJECT_DEPENDS "${_regen_stamp}")
endif()
set_source_files_properties(${SUPPORT_SOURCES} PROPERTIES COMPILE_OPTIONS "/O2;/GS-")
set_source_files_properties("${SA}/d3dx_compat.c" PROPERTIES INCLUDE_DIRECTORIES "${DXSDK_DIR}/Include")
target_include_directories(halo_rebuilt PRIVATE "${SA}")

file(TO_NATIVE_PATH "${HALO_FOLDER}" _halo_folder)
string(REPLACE "\\" "\\\\" _halo_folder "${_halo_folder}")
set_source_files_properties("${SA}/loader.c" PROPERTIES COMPILE_DEFINITIONS "HALO_FOLDER=\"${_halo_folder}\"")

target_link_directories(halo_rebuilt PRIVATE "${DXSDK_DIR}/Lib/x86")
target_link_libraries(halo_rebuilt PRIVATE
    kernel32 user32 gdi32 advapi32 ole32 oleaut32 winmm ws2_32 shell32 version shlwapi comdlg32
    dsound dinput8 dxguid d3d9 d3dx9 legacy_stdio_definitions wininet
    ${THIRD_PARTY_LIBS} delayimp)
# the data image sits at the original addresses (0x400000..0x891000), so the exe must load elsewhere, unrelocated
target_link_options(halo_rebuilt PRIVATE /FIXED /DYNAMICBASE:NO /BASE:0x10000000 /SAFESEH:NO /OPT:NOREF /OPT:NOICF
    /LARGEADDRESSAWARE:NO /NODEFAULTLIB:msvcrt.lib /DELAYLOAD:binkw32.dll /DELAYLOAD:vorbisfile.dll /MAP)

if(EXISTS "${HALO_FX_OVERRIDE}")
    add_custom_command(TARGET halo_rebuilt POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${HALO_FX_OVERRIDE}"
                "$<TARGET_FILE_DIR:halo_rebuilt>/override/shaders/fx.bin" VERBATIM)
else()
    message(WARNING "HALO_FX_OVERRIDE (${HALO_FX_OVERRIDE}) not found: convert shaders\\fx.bin with "
                    "tools/convert_fx.py and put it in override/shaders/ next to halo_rebuilt.exe")
endif()
