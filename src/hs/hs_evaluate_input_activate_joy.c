// hs_evaluate_input_activate_joy  (not a Ghidra function; the evaluate handler of hs function 447 "input_activate_joy" (short, short -> boolean))
// address 0x481890, size 140 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481890, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481890..0x48191c: a joystick (first short) below the count (0x006b1844) whose owner (+0x00 of its 0x240-byte record at 0x006b1a98)
//   is -1, for a player (second short) whose joystick (0x006b2ce8[player]) is -1, is bound both ways and gives 1;
//   otherwise 0; in a zeroed dword.
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
extern int32_t input_device_count; // 0x006b1844
extern uint8_t input_device_to_slot[]; // 0x006b1a98, 0x240 each
extern int32_t joystick_slot_devices[]; // 0x006b2ce8

void hs_evaluate_input_activate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int32_t joystick = *(int16_t *)&arguments[0];
    int32_t player = *(int16_t *)&arguments[1];
    uint8_t bound = 0;

    if (joystick < input_device_count && *(int32_t *)(input_device_to_slot + joystick * 0x240) == -1 &&
        joystick_slot_devices[player] == -1) {
        *(int32_t *)(input_device_to_slot + joystick * 0x240) = player;
        joystick_slot_devices[player] = joystick;
        bound = 1;
    }
    hs_thread_return((int32_t)bound, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
