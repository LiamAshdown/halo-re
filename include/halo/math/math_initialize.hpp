/**
 * @file include/halo/math/math_initialize.hpp
 * Math_initialize: table set-up and the matrix4x3_multiply cpu dispatch.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Builds the sphere direction table and the periodic function tables, then installs the matrix4x3_multiply
 * variant for the CPU (SSE, then 3DNow!) unless -noSSE is on the command line or safe_mode is set.
 * @address 0x004cd3f0
 */
void math_initialize();

}  // namespace halo::math
