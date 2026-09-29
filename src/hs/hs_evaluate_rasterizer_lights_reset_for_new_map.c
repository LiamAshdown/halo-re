// hs_evaluate_rasterizer_lights_reset_for_new_map  (not a Ghidra function; the evaluate handler of hs function 420 "rasterizer_lights_reset_for_new_map" (no parameters -> void))
// address 0x4810c0, size 42 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4810c0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4810c0..0x4810ea: zeroes 0x8c0 dwords at 0x006bc510 and 0x4002 dwords at 0x006be810, and 0x0071d134; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint32_t lens_flare_object_visibility_table[0x8c0]; // 0x006bc510
extern uint32_t lens_flare_marker_visibility[0x4002]; // 0x006be810
extern int32_t lens_flare_instance_count; // 0x0071d134

void hs_evaluate_rasterizer_lights_reset_for_new_map(int16_t function_index, uint32_t thread_index, char first)
{
    int32_t i;

    for (i = 0; i < 0x8c0; i++) {
        lens_flare_object_visibility_table[i] = 0;
    }
    for (i = 0; i < 0x4002; i++) {
        lens_flare_marker_visibility[i] = 0;
    }
    lens_flare_instance_count = 0;
    hs_thread_return(0, thread_index);
}
