// recorded_animation_decode_aiming_speed_event  (Ghidra: missed_44a080; renamed per
// types/cutscene.h recorded_animation_event_type: "_recorded_animation_event_aiming_speed = 3,
// // unit_control_data.aiming_speed")
// address 0x44a080, size 18 bytes
// name confidence: 0.85 (types/cutscene.h documents the exact type number and target field)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0".
// unit_control_data.aiming_speed at +0x01 (types/units.h) matches the single byte this function
// writes one past control's base.
// register convention: cdecl stack parameters (state, control, header, cursor), same slots as
// recorded_animation_decode_animation_state_event; objdump confirms only [esp+8] (control) and
// [esp+0x10] (cursor) are read. blam-cc: stack -> (state, control, header, cursor).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (state, control, header, cursor)
// Compressed event type 3: copies one byte from *cursor into control->aiming_speed and advances
// *cursor by 1. state and header are unused.
void recorded_animation_decode_aiming_speed_event(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->aiming_speed = *(int8_t *)*cursor;
    *cursor += 1;
}

#if 0
Original Ghidra decompilation (0x44a080):

void FUN_0044a080(undefined4 param_1,int param_2,undefined4 param_3,int *param_4)

{
  *(undefined1 *)(param_2 + 1) = *(undefined1 *)*param_4;
  *param_4 = *param_4 + 1;
  return;
}
#endif
