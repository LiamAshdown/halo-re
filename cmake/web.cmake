# The browser build (Emscripten, WebAssembly + WebGL 2), included by CMakeLists.txt when configuring with emcmake:
#
#   emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/web --parallel
#   python tools/serve_web.py --web build/web --halo "<Halo folder>" --fx <converted fx.bin>
#
# then open http://127.0.0.1:8080/. The Halo files are never part of the build: the page streams them from the local
# server. OpenGL renderer only (HALO_D3D9 0), the POSIX platform layer, threads (the server sends the cross-origin
# isolation headers they need).
#
# Not configured or built yet: written before the Emscripten SDK was installed (docs/BROWSER_PORT.md, milestone 5).

set(WEB_COMPILE_OPTIONS
    -std=c++20 -fno-exceptions -fno-rtti
    -fshort-wchar                     # wchar_t is a UTF-16 unit, as on Windows (halo/platform/wchar16.h)
    -fno-builtin-wcslen               # or clang turns 16-bit counting loops (halo_wcslen too) into the C library's 32-bit wcslen
    -fms-extensions -fdeclspec        # __declspec(align(n)) and the like
    -fno-strict-aliasing -fwrapv      # the reconstructed code type-puns and relies on wrapping arithmetic, as MSVC allows
    -pthread
    -w
    "SHELL:-include ${CMAKE_SOURCE_DIR}/include/halo/clang_compat.h")
set(WEB_DEFINITIONS HALO_D3D9=0 HALO_WCHAR16_COMPAT=1)
set(WEB_INCLUDES
    "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/third_party" "${CMAKE_SOURCE_DIR}/standalone/data")
# types/ holds math.h, memory.h, input.h...: quote-only, so <math.h> still finds the C library's
set(WEB_QUOTE_INCLUDES "SHELL:-iquote ${CMAKE_SOURCE_DIR}/types" "SHELL:-iquote ${CMAKE_SOURCE_DIR}/standalone/data")

# ---- the game code
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*/*.cpp")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/[a-z]+/[a-z0-9_]+_win32\\.cpp$")
file(GLOB GAMESPY_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/gamespy/*.c")

# ---- the engine globals: the data image without MSVC's ordered sections and with its linker aliases as alias
# definitions (tools/web_data.py). The clustered globals keep their definition order (eq_*.cpp list them by original
# address), and zero ones stay in .data next to the others; standalone_data_layout_check() reports any that moved.
file(GLOB DATA_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/standalone/data/*.cpp")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/web_data.py" "${CMAKE_BINARY_DIR}/data" ${DATA_SOURCES}
    RESULT_VARIABLE web_data_result)
if(NOT web_data_result EQUAL 0)
    message(FATAL_ERROR "tools/web_data.py failed")
endif()
set(WEB_DATA_SOURCES "")
foreach(source ${DATA_SOURCES})
    get_filename_component(name "${source}" NAME)
    list(APPEND WEB_DATA_SOURCES "${CMAKE_BINARY_DIR}/data/${name}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
endforeach()
# -fms-compatibility: the tables store function addresses in void * slots, as MSVC allows
set_source_files_properties(${WEB_DATA_SOURCES} PROPERTIES COMPILE_OPTIONS "-fno-zero-initialized-in-bss;-fms-compatibility")

# ---- third-party code
set(MOJO_DIR "${CMAKE_SOURCE_DIR}/third_party/mojoshader")
add_library(mojoshader STATIC
    "${MOJO_DIR}/mojoshader.c" "${MOJO_DIR}/mojoshader_common.c" "${MOJO_DIR}/mojoshader_effects.c"
    "${MOJO_DIR}/profiles/mojoshader_profile_common.c" "${MOJO_DIR}/profiles/mojoshader_profile_glsl.c")
target_include_directories(mojoshader PUBLIC "${MOJO_DIR}")
target_compile_definitions(mojoshader PUBLIC MOJOSHADER_NO_VERSION_INCLUDE=1 MOJOSHADER_EFFECT_SUPPORT=1 MOJOSHADER_FLIP_RENDERTARGET=1
    MOJOSHADER_DEPTH_CLIPPING=1 SUPPORT_PROFILE_D3D=0 SUPPORT_PROFILE_BYTECODE=0 SUPPORT_PROFILE_HLSL=0 SUPPORT_PROFILE_GLSL120=0
    SUPPORT_PROFILE_ARB1=0 SUPPORT_PROFILE_ARB1_NV=0 SUPPORT_PROFILE_METAL=0 SUPPORT_PROFILE_SPIRV=0 SUPPORT_PROFILE_GLSPIRV=0)
target_compile_options(mojoshader PRIVATE -w -pthread)

add_library(stb_vorbis STATIC third_party/stb/stb_vorbis.c)
target_compile_definitions(stb_vorbis PRIVATE STB_VORBIS_NO_STDIO STB_VORBIS_NO_PUSHDATA_API)
target_compile_options(stb_vorbis PRIVATE -w -pthread)

# ---- the page
add_executable(halo ${GAME_SOURCES} ${GAMESPY_SOURCES} ${WEB_DATA_SOURCES} "${CMAKE_SOURCE_DIR}/standalone/web_main.cpp")
set_target_properties(halo PROPERTIES SUFFIX ".html")
set_source_files_properties(${GAMESPY_SOURCES} PROPERTIES LANGUAGE C COMPILE_OPTIONS "-w;-pthread;-fshort-wchar;-fno-builtin-wcslen;-fms-extensions;-fdeclspec;-iquote;${CMAKE_SOURCE_DIR}/types")
target_compile_options(halo PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${WEB_COMPILE_OPTIONS}> ${WEB_QUOTE_INCLUDES} "SHELL:-sUSE_SDL=2")
target_compile_definitions(halo PRIVATE ${WEB_DEFINITIONS})
target_include_directories(halo PRIVATE ${WEB_INCLUDES})
target_link_libraries(halo PRIVATE mojoshader stb_vorbis)
target_link_options(halo PRIVATE
    -pthread
    "SHELL:-sUSE_SDL=2"
    "SHELL:-sMIN_WEBGL_VERSION=2" "SHELL:-sMAX_WEBGL_VERSION=2"
    "SHELL:-sGL_ENABLE_GET_PROC_ADDRESS=1"     # the renderer loads every GL entry point by name (SDL_GL_GetProcAddress)
    "SHELL:-sWASMFS=1"                          # fetch backend (the Halo folder) and OPFS (saves)
    "SHELL:-sPROXY_TO_PTHREAD=1"                # main() runs on a worker: the file backends block, which the page thread may not
    "SHELL:-sOFFSCREENCANVAS_SUPPORT=1"         # that worker draws: the canvas goes to it as an OffscreenCanvas
    "SHELL:-sOFFSCREENCANVASES_TO_PTHREAD=#canvas"
    "SHELL:-sINITIAL_MEMORY=2048MB"             # map memory sits at 0x40000000 (+27 MB); the heap grows past it
    "SHELL:-sSTACK_SIZE=8MB" "SHELL:-sDEFAULT_PTHREAD_STACK_SIZE=1MB" "SHELL:-sPTHREAD_POOL_SIZE=8"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=callMain,stringToNewUTF8"
    "SHELL:-sEXPORTED_FUNCTIONS=_main,_malloc,_free"  # malloc/free: the page thread hands network messages to net_web.cpp
    "SHELL:-sEMULATE_FUNCTION_POINTER_CASTS=1"  # the engine calls through tables whose entries have other signatures, as x86 allows
    "SHELL:-sENVIRONMENT=web,worker"
    "SHELL:-sASSERTIONS=1"
    "--profiling-funcs"                         # function names in stack traces
    "SHELL:-Wl,--Map=${CMAKE_BINARY_DIR}/halo.map"
    "--shell-file" "${CMAKE_SOURCE_DIR}/web/shell.html")
set_property(TARGET halo APPEND PROPERTY LINK_DEPENDS "${CMAKE_SOURCE_DIR}/web/shell.html")  # relink when the page changes
# debug builds trap null and out-of-range loads and stores where they happen (SAFE_HEAP)
target_link_options(halo PRIVATE $<$<CONFIG:Debug>:SHELL:-sSAFE_HEAP=2>)  # 2: no alignment checks; the engine reads packed records unaligned, as x86 allows
# bisecting optimizer-sensitive code: src/ globs listed here (e.g. "cache/*.cpp") compile at -O0 (a debugging aid, empty normally)
set(HALO_WEB_O0_SOURCES "" CACHE STRING "src/ globs compiled at -O0 (semicolon list)")
foreach(pattern ${HALO_WEB_O0_SOURCES})
    file(GLOB o0_sources "${CMAKE_SOURCE_DIR}/src/${pattern}")
    set_property(SOURCE ${o0_sources} APPEND PROPERTY COMPILE_OPTIONS -O0)
endforeach()

# AddressSanitizer for finding heap overruns; the data image stays uninstrumented, since redzones between its globals
# would break the original layout the engine runs across
option(HALO_WEB_ASAN "build with AddressSanitizer" OFF)
if(HALO_WEB_ASAN)
    target_compile_options(halo PRIVATE -fsanitize=address)
    target_link_options(halo PRIVATE -fsanitize=address "SHELL:-sBINARYEN_EXTRA_PASSES=--pass-arg=max-func-params@24")  # the sanitizer has 17-parameter functions; fpcast-emu allows 16 by default
    set_property(SOURCE ${WEB_DATA_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -fno-sanitize=address)
endif()
