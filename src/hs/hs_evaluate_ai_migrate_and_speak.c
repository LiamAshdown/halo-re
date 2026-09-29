// hs_evaluate_ai_migrate_and_speak  (not a Ghidra function; the evaluate handler of hs "ai_migrate_and_speak" (ai, ai, string -> void))
// address 0x47d840, size 132 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d840, only reachable through that pointer.
//   Campaign track: 8 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d840: EDX = the source ai (+0x0), stack (target ai +0x4, 1, is "advance" by the string +0x8); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void ai_squads_merge(uint32_t source_reference, uint32_t target_encounter_index, char notify,
    char is_platoon_merge); // 0x433590, EDX source, stack (target, notify, platoon)

void hs_evaluate_ai_migrate_and_speak(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        char *verb = (char *)arguments[2];
        char advance = 0;

        // 0x47d877: "advance" selects the platoon merge; the "retreat" compare's result is unused
        if (_stricmp(verb, "advance") == 0) {
            advance = 1;
        } else {
            _stricmp(verb, "retreat");
        }
        ai_squads_merge((uint32_t)arguments[0], (uint32_t)arguments[1], 1, advance);
        hs_thread_return(0, thread_index);
    }
}
