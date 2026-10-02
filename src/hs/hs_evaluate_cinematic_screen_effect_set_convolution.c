// hs_evaluate_cinematic_screen_effect_set_convolution  (not a Ghidra function; the evaluate handler of hs function 423 "cinematic_screen_effect_set_convolution" (short, short, real, real, real -> void))
// address 0x4811c0, size 86 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4811c0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4811c0..0x481216: cinematic_screen_effect_set_convolution(DX = the second short, stack: the first short zero-extended, the three
//   reals), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void cinematic_screen_effect_set_convolution(int16_t convolution_type, int16_t extra_passes,
    float radius_lower_bound, float radius_upper_bound, float duration); // 0x5121d0, blam-cc: EDX, stack

void hs_evaluate_cinematic_screen_effect_set_convolution(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    cinematic_screen_effect_set_convolution(*(int16_t *)&arguments[1], *(int16_t *)&arguments[0], *(float *)&arguments[2],
        *(float *)&arguments[3], *(float *)&arguments[4]);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
