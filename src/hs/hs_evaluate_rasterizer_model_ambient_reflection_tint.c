// hs_evaluate_rasterizer_model_ambient_reflection_tint  (not a Ghidra function; the evaluate handler of hs "rasterizer_model_ambient_reflection_tint" (real, real, real, real -> void))
// address 0x481050, size 104 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x481050, only reachable through that pointer.
//   Campaign track: 4 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x481050: the four reals are copied to the tint block pointed to by 0x71cfc0 when present; returns 0.
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
extern uint32_t *rasterizer_model_ambient_reflection_tint; // 0x0071cfc0, the model ambient reflection tint (4 floats)

void hs_evaluate_rasterizer_model_ambient_reflection_tint(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (rasterizer_model_ambient_reflection_tint != 0) {
            rasterizer_model_ambient_reflection_tint[0] = (uint32_t)arguments[0];
            rasterizer_model_ambient_reflection_tint[1] = (uint32_t)arguments[1];
            rasterizer_model_ambient_reflection_tint[2] = (uint32_t)arguments[2];
            rasterizer_model_ambient_reflection_tint[3] = (uint32_t)arguments[3];
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
