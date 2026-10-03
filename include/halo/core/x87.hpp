/**
 * @file include/halo/core/x87.hpp
 * Single x87 instructions that the original code executes inline (fsin, fcos, fptan, fpatan, fabs, fistp), plus the two
 * 64-bit helper calls of the original runtime. Each function is the one instruction, so the results match the original
 * bit for bit (same 80-bit unit, same rounding). Defined in src/math/x87.cpp.
 */
#pragma once

#include <stdint.h>

namespace halo::x87 {

double fsin(double x);
double fcos(double x);
double ftan(double x);
double fpatan(double y, double x);
float fabsf(float x);
int32_t ROUND(float x);
int32_t fistp_round(float x);
int32_t __ftol(double x);
int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high);
int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high);

}  // namespace halo::x87
