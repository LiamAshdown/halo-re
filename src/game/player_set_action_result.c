// player_set_action_result  (Ghidra: player_set_action_result, already named)
// address 0x47b940, size 86 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: types/hs.h hs_function_definitions[0x20a] (0x00688b58) and hs_function_definition
//   (parameter_count +0x1a, parameters +0x1c); src/hs/hs_evaluate_typed_arguments.c's
//   established (thread_index, parameter_count, expected_types, first) signature; the sibling
//   player_examine_nearby_vehicle.c (this batch) for the same vitality-flags byte-0x107 idiom
//   (bit 0x20 here instead of 0x01).
// register convention: none -- all three are genuine stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "hs.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

// Evaluates function_index's one object argument; once ready, and unless it is -1, sets
// object::vitality_flags bit 0x2000 on it (byte 0x106 bit 0x20) and returns from the HS thread.
void player_set_action_result(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *args = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (args != 0) {
        object *target = (object *)((object_header *)object_data->data)[args[0] & 0xffff].data;
        *((uint8_t *)&target->vitality_flags) |= 0x20;
        hs_thread_return(0, thread_index);
    }
}

#if 0
Original Ghidra decompilation (0x47b940), from tools/pack.py 0x47b940:

void player_set_action_result(short param_1,undefined4 param_2,undefined4 param_3)

{
  byte *pbVar1;
  uint *puVar2;

  puVar2 = (uint *)hs_evaluate_typed_arguments
                             (param_2,(int)*(short *)((&PTR_DAT_00688b58)[param_1] + 0x1a),
                              (&PTR_DAT_00688b58)[param_1] + 0x1c,param_3);
  if (puVar2 != (uint *)0x0) {
    pbVar1 = (byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*puVar2 & 0xffff) * 0xc) + 0x106
                     );
    *pbVar1 = *pbVar1 | 0x20;
    hs_thread_return();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
