/**
 * @file src/math/math_initialize.cpp
 * Math_initialize: table set-up and the matrix4x3_multiply cpu dispatch.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/math/math.hpp"
#include "halo/math/globals.hpp"

#include "crt.h"
#include "tags.h"

extern "C" {
extern int cpu_get_type(int feature);
extern int32_t shell_argc;
extern char **shell_argv;
extern int32_t safe_mode;
}

namespace halo::math {

void math_initialize()
{
    int32_t i;

    sphere_point_table_init();
    periodic_function_tables_init();

    globals().matrix4x3_multiply_procedure = matrix4x3_multiply;

    for (i = 0; i < shell_argc; i++) {
        char *arg = shell_argv[i];
        if (*arg == '-' && stricmp("-noSSE", arg) == 0) {
            return;
        }
    }

    if (safe_mode == 0) {
        if (cpu_get_type(0x1d) != 0) {
            globals().matrix4x3_multiply_procedure = matrix4x3_multiply_sse;
            return;
        }
        if (cpu_get_type(0x1a) != 0) {
            globals().matrix4x3_multiply_procedure = matrix4x3_multiply_3dnow;
        }
    }
}

}  // namespace halo::math
