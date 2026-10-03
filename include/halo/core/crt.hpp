/**
 * @file include/halo/core/crt.hpp
 * The C runtime headers the engine modules use (<cmath>, <cstdlib>, <cstdio>, <cwchar>). types/ shadows <math.h> on the
 * include path, so the CRT math declarations are pulled in first; every module includes this header instead of
 * declaring libc functions itself.
 */
#pragma once

#include <corecrt_math.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
