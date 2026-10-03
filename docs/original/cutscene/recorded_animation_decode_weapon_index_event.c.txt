// recorded_animation_decode_weapon_index_event  (Ghidra: missed_44a0c0; renamed per
// types/cutscene.h recorded_animation_event_type: "_recorded_animation_event_weapon_index = 5,
// // unit_control_data.weapon_index")
// address 0x44a0c0, size 21 bytes
// name confidence: 0.85 (types/cutscene.h documents the exact type number and target field)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0".
// unit_control_data.weapon_index (int16_t) at +0x04 (types/units.h) matches the word this
// function writes.
// register convention: cdecl stack parameters (state, control, header, cursor); objdump confirms
// only [esp+8] (control) and [esp+0x10] (cursor) are read. blam-cc: stack -> (state, control,
// header, cursor).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (state, control, header, cursor)
// Compressed event type 5: copies one word from *cursor into control->weapon_index and advances
// *cursor by 2. state and header are unused.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void recorded_animation_decode_weapon_index_event(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->weapon_index = *(int16_t *)*cursor;
    *cursor += 2;
}

#if 0
Original Ghidra decompilation (0x44a0c0):

void FUN_0044a0c0(undefined4 param_1,int param_2,undefined4 param_3,int *param_4)

{
  *(undefined2 *)(param_2 + 4) = *(undefined2 *)*param_4;
  *param_4 = *param_4 + 2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
