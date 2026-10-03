/**
 * @file src/math/math_initialize.cpp
 * Math_initialize: table set-up and the matrix4x3_multiply cpu dispatch.
 */

#include "halo/math/math.hpp"
#include "halo/math/globals.hpp"

#include "crt.h"
#include "tags.h"
#include "halo/shell/api.hpp"


namespace halo::math {

void math_initialize()
{
    int32_t i;

    sphere_point_table_init();
    periodic_function_tables_init();

    globals().matrix4x3_multiply_procedure = matrix4x3_multiply;

    for (i = 0; i < halo::shell::globals().argc; i++) {
        char *arg = halo::shell::globals().argv[i];
        if (*arg == '-' && stricmp("-noSSE", arg) == 0) {
            return;
        }
    }

    if (globals().safe_mode == 0) {
        if (halo::shell::cpu_get_type(0x1d) != 0) {
            globals().matrix4x3_multiply_procedure = matrix4x3_multiply_sse;
            return;
        }
        if (halo::shell::cpu_get_type(0x1a) != 0) {
            globals().matrix4x3_multiply_procedure = matrix4x3_multiply_3dnow;
        }
    }
}

}  // namespace halo::math
