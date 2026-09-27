// hs_evaluate_ai_kill_silent  (not a Ghidra function; the evaluate handler of hs "ai_kill_silent" (ai -> void))
// address 0x47d290, size 67 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d290, only reachable through that pointer.
//   Campaign track: 6 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d290: EAX = the ai reference, BL = 1 (silent); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_reference_notify_actors(uint32_t packed_reference, uint8_t flag); // 0x432bd0, EAX, BL

void hs_evaluate_ai_kill_silent(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_notify_actors((uint32_t)arguments[0], 1);
        hs_thread_return(0, thread_index);
    }
}
