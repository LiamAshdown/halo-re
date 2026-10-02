// math_initialize  (Ghidra: math_initialize, already named)
// address 0x4cd3f0, size 134 bytes
// name confidence: 0.8   rewrite confidence: 0.65
// evidence: out/phase4/math_types_notes.md "matrix multiply dispatch": installs the scalar
//   matrix4x3_multiply @0x4cc0d0 by default, then upgrades to a CPU-selected SIMD build unless
//   -noSSE is on the command line or safe_mode (0x007196f4, types/shell.h) is set. Misattributed-functions note 1: the
//   function Ghidra calls "matrix4x3_multiply_sse" (0x4cc3a0, selected for cpu_get_type(0x1a))
//   is actually AMD 3DNow!; the real SSE build is the unnamed 0x4cc250 (selected first, for
//   cpu_get_type(0x1d)) -- both externs below use the corrected names.
// register convention: __cdecl, no arguments.

#include "crt.h"
#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void sphere_point_table_init(void); // 0x4cd0e0
extern void periodic_function_tables_init(void); // 0x4cc8d0
extern int cpu_get_type(int feature); // 0x5402a0, system module

extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0, scalar
extern void matrix4x3_multiply_sse(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc250 -- true SSE build, see header note
extern void matrix4x3_multiply_3dnow(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc3a0 -- AMD 3DNow! build, Ghidra names it matrix4x3_multiply_sse

extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664
extern int32_t shell_argc; // 0x00721e94
extern char **shell_argv; // 0x00721e90
extern int32_t safe_mode; // 0x007196f4, shell command-line BOOL (types/shell.h), also set by the fatal-error path

// One-time engine startup routine: initializes the math subsystem's tables (the unit-sphere
// sample table and the periodic/transition wave tables) and selects the best available
// matrix4x3_multiply implementation for the CPU, unless the command line disables it with
// -noSSE or safe_mode is set.
void math_initialize(void)
{
    int32_t i;

    sphere_point_table_init();
    periodic_function_tables_init();

    matrix4x3_multiply_procedure = matrix4x3_multiply;

    for (i = 0; i < shell_argc; i++) {
        char *arg = shell_argv[i];
        if (*arg == '-' && stricmp("-noSSE", arg) == 0) {
            return;
        }
    }

    if (safe_mode == 0) {
        if (cpu_get_type(0x1d) != 0) {
            matrix4x3_multiply_procedure = matrix4x3_multiply_sse;
            return;
        }
        if (cpu_get_type(0x1a) != 0) {
            matrix4x3_multiply_procedure = matrix4x3_multiply_3dnow;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4cd3f0):

void __cdecl math_initialize(void)

{
  char *_Str2;
  int iVar1;
  int iVar2;

  sphere_point_table_init();
  periodic_function_tables_init();
  iVar2 = 0;
  PTR_matrix4x3_multiply_00696664 = matrix4x3_multiply;
  if (0 < DAT_00721e94) {
    do {
      _Str2 = *(char **)(DAT_00721e90 + iVar2 * 4);
      if ((*_Str2 == '-') && (iVar1 = __stricmp("-noSSE",_Str2), iVar1 == 0)) {
        return;
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00721e94);
  }
  if (DAT_007196f4 == 0) {
    iVar2 = cpu_get_type(0x1d);
    if (iVar2 != 0) {
      PTR_matrix4x3_multiply_00696664 = &LAB_004cc250;
      return;
    }
    iVar2 = cpu_get_type(0x1a);
    if (iVar2 != 0) {
      PTR_matrix4x3_multiply_00696664 = matrix4x3_multiply_sse;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
