/**
 * @file src/math/x87.cpp
 * The x87 single-instruction helpers of halo::x87 (include/halo/core/x87.hpp).
 */

#include "halo/core/x87.hpp"

#if !(defined(_M_IX86) && defined(_MSC_VER))
#include <math.h>
#endif

namespace halo::x87 {

#if defined(_M_IX86) && defined(_MSC_VER)

/** FSIN on the 80-bit unit. */
double fsin(double x)
{
    double r;
    __asm { fld x }
    __asm { fsin }
    __asm { fstp r }
    return r;
}

/** FCOS on the 80-bit unit. */
double fcos(double x)
{
    double r;
    __asm { fld x }
    __asm { fcos }
    __asm { fstp r }
    return r;
}

/** FPTAN on the 80-bit unit; the pushed 1.0 is discarded. */
double ftan(double x)
{
    double r;
    __asm { fld x }
    __asm { fptan }
    __asm { fstp st(0) }
    __asm { fstp r }
    return r;
}

/** FPATAN: atan2(ST1 = y, ST0 = x). */
double fpatan(double y, double x)
{
    double r;
    __asm { fld y }
    __asm { fld x }
    __asm { fpatan }
    __asm { fstp r }
    return r;
}

/** FABS on a float. */
float fabsf(float x)
{
    float r;
    __asm { fld x }
    __asm { fabs }
    __asm { fstp r }
    return r;
}

/** FISTP in the current rounding mode (the engine leaves it at round-to-nearest). */
int32_t ROUND(float x)
{
    int32_t r;
    __asm { fld x }
    __asm { fistp r }
    return r;
}

/** Same instruction as ROUND under the name the decompiler used. */
int32_t fistp_round(float x)
{
    int32_t r;
    __asm { fld x }
    __asm { fistp r }
    return r;
}

#else

/* no x87 unit (WebAssembly): the C library's double-precision functions; results can differ in the last bits */

double fsin(double x)
{
    return ::sin(x);
}

double fcos(double x)
{
    return ::cos(x);
}

double ftan(double x)
{
    return ::tan(x);
}

double fpatan(double y, double x)
{
    return ::atan2(y, x);
}

float fabsf(float x)
{
    return x < 0.0f ? -x : x;
}

int32_t ROUND(float x)
{
    return static_cast<int32_t>(::lrintf(x));  // round to nearest even, the x87 default mode
}

int32_t fistp_round(float x)
{
    return static_cast<int32_t>(::lrintf(x));
}

#endif

/** Chops toward zero and keeps the low dword (the original runtime helper takes its input on the x87 stack). */
int32_t __ftol(double x)
{
    return static_cast<int32_t>(static_cast<long long>(x));
}

/** 64-bit multiply of two values given as dword pairs (the original runtime helper). */
int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high)
{
    return static_cast<int64_t>((static_cast<uint64_t>(static_cast<uint32_t>(a_high)) << 32) | static_cast<uint32_t>(a_low)) *
           static_cast<int64_t>((static_cast<uint64_t>(static_cast<uint32_t>(b_high)) << 32) | static_cast<uint32_t>(b_low));
}

/** 64-bit divide of a by the dword pair b; a zero divisor yields 0. */
int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high)
{
    const int64_t b = static_cast<int64_t>((static_cast<uint64_t>(static_cast<uint32_t>(b_high)) << 32) | static_cast<uint32_t>(b_low));
    return b ? static_cast<int32_t>(a / b) : 0;
}

}  // namespace halo::x87
