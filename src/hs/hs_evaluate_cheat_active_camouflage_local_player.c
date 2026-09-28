// hs_evaluate_cheat_active_camouflage_local_player  (not a Ghidra function; the evaluate procedure of hs function "cheat_active_camouflage_local_player"; no C existed, so running it
//   from the console trapped as unlisted_47cee0)
// address 0x47cee0, size 64 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x47cee0: evaluates the typed arguments (0x48a850); when they are ready, passes the short local player index in AX to 0x45a720 (named cheat_make_player_invincible in its C file). Returns 0 through hs_thread_return (EAX 0, ECX thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, EAX value, ECX thread

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types,
    char first); // 0x48a850
extern void cheat_make_player_invincible(int16_t local_player_slot); // 0x45a720, AX

void hs_evaluate_cheat_active_camouflage_local_player(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        cheat_make_player_invincible(*(int16_t *)arguments);
        hs_thread_return(0, thread_index);
    }
}
