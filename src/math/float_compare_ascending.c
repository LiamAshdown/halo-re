// float_compare_ascending  (Ghidra: float_compare_ascending, already named)
// address 0x405360, size 45 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: out/phase4/math_functions.md; __cdecl signature already recovered by Ghidra
//   (`int __cdecl float_compare_ascending(float *param_1, float *param_2)`), zero callers
//   inside this module's slice -- almost certainly a qsort() comparator (the module already
//   uses `qsort` @0x623410 as a callee elsewhere in the ai/math boundary).
// Parameters are const void * (the qsort comparator type) to agree with the extern in
//   src/ai/actor_squad_action_execute.c, whose two qsort calls sort 8-byte records whose
//   first member is the float key.
// register convention: __cdecl, both arguments on the stack; no register-passed arguments.
//   Confirmed against objdump: `mov ecx,[esp+0x4]` / `mov edx,[esp+0x8]` load both pointers
//   from the stack immediately, with no register argument read before that.

#include "crt.h"
#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Three-way float comparator in qsort() order: -1 if *a < *b, 1 if *b < *a, 0 otherwise
// (including when either is NaN, since neither x87 comparison then succeeds).
int32_t float_compare_ascending(const void *a_value, const void *b_value)
{
    const real *a = (const real *)a_value;
    const real *b = (const real *)b_value;

    if (*a < *b) {
        return -1;
    }
    if (*b < *a) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x405360):

int __cdecl float_compare_ascending(float *param_1,float *param_2)

{
  if (*param_1 < *param_2) {
    return -1;
  }
  if (*param_2 < *param_1) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
