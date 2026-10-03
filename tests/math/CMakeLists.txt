# Math module behaviour-equivalence test (HALO_BUILD_TESTS=ON). Same procedure as tests/math/run_difftest.py:
# two executables, one from the original src/math/*.c at HALO_MATH_REFERENCE_REF (extracted with git archive at
# configure time) and one from the converted src/math/*.cpp, both with the halo_game compile flags and the same driver,
# stubs and x87 shims; the test compares their outputs byte for byte.
set(HALO_MATH_REFERENCE_REF "c54d5e1f" CACHE STRING "git commit whose src/math/*.c is the difftest reference")
find_package(Git REQUIRED)

set(_ref_root "${CMAKE_CURRENT_BINARY_DIR}/reference_tree")
set(_ref_tar "${CMAKE_CURRENT_BINARY_DIR}/reference.tar")
execute_process(COMMAND "${GIT_EXECUTABLE}" archive --format=tar -o "${_ref_tar}" "${HALO_MATH_REFERENCE_REF}"
                        src/math types harness/msvc_compat.h
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RESULT_VARIABLE _git_result)
if(NOT _git_result EQUAL 0)
    message(FATAL_ERROR "git archive ${HALO_MATH_REFERENCE_REF} failed; set HALO_MATH_REFERENCE_REF")
endif()
file(MAKE_DIRECTORY "${_ref_root}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf "${_ref_tar}" WORKING_DIRECTORY "${_ref_root}")

set(_flags /std:c++20 /permissive- /GR- /W3 /Od /GS- /Oy- /Gy /wd4996)

function(halo_math_objects name root sources)
    add_library(${name} OBJECT ${sources})
    set_source_files_properties(${sources} PROPERTIES LANGUAGE CXX)
    target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/include" "${root}/types" "${DXSDK_DIR}/Include")
    target_compile_options(${name} PRIVATE ${_flags} "/FI${root}/harness/msvc_compat.h")
endfunction()

file(GLOB _ref_sources "${_ref_root}/src/math/*.c")
file(GLOB _new_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/math/*.cpp")
halo_math_objects(halo_math_reference "${_ref_root}" "${_ref_sources}")
halo_math_objects(halo_math_converted "${CMAKE_SOURCE_DIR}" "${_new_sources}")

add_library(halo_math_difftest_driver OBJECT math_difftest.cpp math_test_stubs.cpp)
target_include_directories(halo_math_difftest_driver PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/types"
                           "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(halo_math_difftest_driver PRIVATE ${_flags} "/FI${CMAKE_SOURCE_DIR}/harness/msvc_compat.h")
add_library(halo_math_difftest_x87 OBJECT "${CMAKE_SOURCE_DIR}/harness/x87_shims.c")
target_compile_options(halo_math_difftest_x87 PRIVATE /O2 /GS-)

foreach(kind reference converted)
    add_executable(math_difftest_${kind} $<TARGET_OBJECTS:halo_math_${kind}>
                   $<TARGET_OBJECTS:halo_math_difftest_driver> $<TARGET_OBJECTS:halo_math_difftest_x87>)
    target_link_libraries(math_difftest_${kind} PRIVATE kernel32 legacy_stdio_definitions)
endforeach()

add_test(NAME math_difftest
         COMMAND "${CMAKE_COMMAND}" -DREF=$<TARGET_FILE:math_difftest_reference>
                 -DNEW=$<TARGET_FILE:math_difftest_converted> -DOUT=${CMAKE_CURRENT_BINARY_DIR}
                 -P "${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake")
