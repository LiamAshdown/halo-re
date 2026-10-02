// hs_evaluate_ai_automatic_migration_target  (not a Ghidra function; the evaluate handler of hs "ai_automatic_migration_target" (ai, boolean -> void))
// address 0x47e380, size 68 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47e380, only reachable through that pointer.
//   Campaign track: 72 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47e380: EAX = ai reference (+0x0), BL = the boolean (+0x4 byte); returns 0.
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
extern void ai_reference_squad_set_unknown_10(uint32_t packed_reference, uint8_t value); // 0x435ab0, EAX, BL

void hs_evaluate_ai_automatic_migration_target(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_squad_set_unknown_10((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
