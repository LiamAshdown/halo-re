/**
 * @file include/halo/core/crt.hpp
 * The C runtime headers the engine modules use (<cmath>, <cstdlib>, <cstdio>, <cwchar>). types/ shadows <math.h> on the
 * include path, so the CRT math declarations are pulled in first; every module includes this header instead of
 * declaring libc functions itself.
 */
#pragma once

#if defined(_MSC_VER)
#include <corecrt_math.h>  // MSVC: <math.h> would find types/math.h, which is on the include path
#else
#include <math.h>          // other compilers put types/ on the quote-only include path (-iquote)
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
