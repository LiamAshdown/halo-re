// recorded_animation_decode_control_flags_event  (Ghidra: missed_44a0a0; renamed per
// types/cutscene.h recorded_animation_event_type: "_recorded_animation_event_control_flags = 4,
// // unit_control_data.control_flags")
// address 0x44a0a0, size 21 bytes
// name confidence: 0.85 (types/cutscene.h documents the exact type number and target field)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0".
// unit_control_data.control_flags (uint16_t) at +0x02 (types/units.h) matches the word this
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
// Compressed event type 4: copies one word from *cursor into control->control_flags and advances
// *cursor by 2. state and header are unused.
void recorded_animation_decode_control_flags_event(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->control_flags = *(uint16_t *)*cursor;
    *cursor += 2;
}

#if 0
Original Ghidra decompilation (0x44a0a0):

void FUN_0044a0a0(undefined4 param_1,int param_2,undefined4 param_3,int *param_4)

{
  *(undefined2 *)(param_2 + 2) = *(undefined2 *)*param_4;
  *param_4 = *param_4 + 2;
  return;
}
#endif
