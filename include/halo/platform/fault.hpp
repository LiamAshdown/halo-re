/**
 * @file include/halo/platform/fault.hpp
 * Guarded blocks: HALO_TRY { ... } HALO_EXCEPT(filter) { ... } is structured exception handling (__try / __except) under
 * MSVC, so a fault inside the block runs the handler as in the original; other compilers run the block unguarded and
 * never evaluate the filter or the handler.
 */
#pragma once

#if defined(_MSC_VER)
#include <excpt.h>
#define HALO_TRY __try
#define HALO_EXCEPT(filter) __except (filter)
#else
#define HALO_TRY if (true)
#define HALO_EXCEPT(filter) else
#endif
