// recorded_animation_decode_animation_state_event  (Ghidra: missed_44a060; renamed per
// types/cutscene.h recorded_animation_event_type: "_recorded_animation_event_animation_state = 2,
// // unit_control_data.animation_state")
// address 0x44a060, size 17 bytes
// name confidence: 0.85 (types/cutscene.h documents the exact type number and target field)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0" and
// "The dispatch is ... Handlers live in `0x686d98[type]` and take (state, control, header,
// cursor)". types/cutscene.h's recorded_animation_compressed_event_proc typedef gives that exact
// 4 argument shape, and unit_control_data.animation_state at +0x00 (types/units.h) matches the
// single byte this function writes.
// register convention: cdecl stack parameters (state, control, header, cursor), same slots as
// recorded_animation_decode_char_difference_event; objdump confirms only [esp+8] (control) and
// [esp+0x10] (cursor) are read -- state and header are unused. blam-cc: stack -> (state, control,
// header, cursor).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (state, control, header, cursor)
// Compressed event type 2: copies one byte from *cursor into control->animation_state and
// advances *cursor by 1. state and header are unused.
void recorded_animation_decode_animation_state_event(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->animation_state = *(int8_t *)*cursor;
    *cursor += 1;
}

#if 0
Original Ghidra decompilation (0x44a060):

void FUN_0044a060(undefined4 param_1,undefined1 *param_2,undefined4 param_3,int *param_4)

{
  *param_2 = *(undefined1 *)*param_4;
  *param_4 = *param_4 + 1;
  return;
}
#endif
