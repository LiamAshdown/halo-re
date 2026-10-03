// hs_evaluate_hud_set_timer_warning_time  (not a Ghidra function; the evaluate handler of hs function 407 "hud_set_timer_warning_time" (short, short -> void))
// address 0x480da0, size 85 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480da0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480da0..0x480df5: HUD messaging (*0x006b3a40) +0x47e = (minutes * 60 + seconds, as a word) * 30; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern hud_messaging_globals *hud_messaging;

void hs_evaluate_hud_set_timer_warning_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint16_t seconds = (uint16_t)(*(uint16_t *)&arguments[0] * 0x3c + *(uint16_t *)&arguments[1]);

    *(uint16_t *)&hud_messaging->timer_warning_ticks = (uint16_t)((uint32_t)seconds * 0x1e);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
