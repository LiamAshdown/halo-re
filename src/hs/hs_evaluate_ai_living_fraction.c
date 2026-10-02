// hs_evaluate_ai_living_fraction  (not a Ghidra function; the evaluate handler of hs "ai_living_fraction" (ai -> real))
// address 0x47e9b0, size 141 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47e9b0, only reachable through that pointer.
//   Campaign track: 18 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47e9b0: stat kind 0 (EDI) with the member total out; living / total (fild / fidiv), 0.0 when the total is not positive.
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
extern uint32_t ai_reference_get_stat_pair(uint32_t packed_reference, int16_t stat_kind, int32_t *out_member_count,
    uint32_t *out_extra); // 0x432f90, stack (reference, out, extra), EDI kind

void hs_evaluate_ai_living_fraction(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t total = 0;
        int32_t living = (int32_t)ai_reference_get_stat_pair((uint32_t)arguments[0], 0, &total, 0);
        float fraction = 0.0f;

        if (total > 0) {
            fraction = (float)living / (float)total;
        }
        hs_thread_return(*(int32_t *)&fraction, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
