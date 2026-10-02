// hs_evaluate_game_skip_ticks  (not a Ghidra function; the evaluate handler of hs function 324 "game_skip_ticks" (short -> void))
// address 0x47fc60, size 78 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fc60, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fc60..0x47fcae: a tick count of at most 15 goes to word 0x0071976e with byte 0x0071976c = 1; returns 0.
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
extern int16_t main_globals_word_0071976e; // 0x0071976e
extern uint8_t main_globals_byte_0071976c; // 0x0071976c

void hs_evaluate_game_skip_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t ticks = *(int16_t *)&arguments[0];

    if (ticks <= 0xf) {
        main_globals_word_0071976e = ticks;
        main_globals_byte_0071976c = 1;
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
