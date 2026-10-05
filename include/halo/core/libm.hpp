/**
 * @file include/halo/core/libm.hpp
 * The C runtime math functions the engine modules call, as halo::libm functions with the exact C signatures (double in,
 * double out; float for the f variants). Qualified calls keep the C double precision: the <cmath> float overloads are not
 * selected, which would change the rounding of the original code.
 */
#pragma once

#if defined(_MSC_VER)
#include <corecrt_math.h>  // MSVC: <math.h> would find types/math.h, which is on the include path
#else
#include <math.h>          // other compilers put types/ on the quote-only include path (-iquote)
#endif

namespace halo::libm {

inline double sqrt(double x) { return ::sqrt(x); }
inline double fabs(double x) { return ::fabs(x); }
inline double floor(double x) { return ::floor(x); }
inline double ceil(double x) { return ::ceil(x); }
inline double pow(double x, double y) { return ::pow(x, y); }
inline double sin(double x) { return ::sin(x); }
inline double cos(double x) { return ::cos(x); }
inline double tan(double x) { return ::tan(x); }
inline double atan(double x) { return ::atan(x); }
inline double atan2(double y, double x) { return ::atan2(y, x); }
inline double acos(double x) { return ::acos(x); }
inline double asin(double x) { return ::asin(x); }
inline double exp(double x) { return ::exp(x); }
inline double exp2(double x) { return ::exp2(x); }
inline double log(double x) { return ::log(x); }
inline double log10(double x) { return ::log10(x); }
inline double fmod(double x, double y) { return ::fmod(x, y); }
inline float sqrtf(float x) { return ::sqrtf(x); }
inline float sinf(float x) { return ::sinf(x); }
inline float cosf(float x) { return ::cosf(x); }
inline float atan2f(float y, float x) { return ::atan2f(y, x); }
inline long lrint(double x) { return ::lrint(x); }
inline long lrintf(float x) { return ::lrintf(x); }
/** Whether x is a NaN (the C runtime's _isnan; neither build relaxes IEEE comparisons). */
inline int is_nan(double x) { return x != x; }

}  // namespace halo::libm
