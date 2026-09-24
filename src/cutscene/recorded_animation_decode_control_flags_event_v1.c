// recorded_animation_decode_control_flags_event_v1  (Ghidra: missed_44a690; renamed per
// types/cutscene.h recorded_animation_control_flags_set_event_v1 comment "0x00 type 4 ...
// 0x04 0x44a690, unit_control_flags")
// address 0x44a690, size 24 bytes
// name confidence: 0.85 (types/cutscene.h ties this exact address to the type-4 v1 set event)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "the set events 6/6/6/6 ... 0x44a690 word ...
// Each one's cursor advance equals the record size." types/cutscene.h
// recorded_animation_control_flags_set_event_v1 (size 6, control_flags at +0x04) matches exactly.
// register convention: recorded_animation_v1_event_proc (control, event, cursor), all cdecl
// stack arguments, same slots as recorded_animation_decode_animation_state_event_v1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (control, event, cursor)
// Uncompressed (v1..v3) event type 4: copies event->control_flags into control->control_flags and
// advances *cursor by the record's fixed size (6 bytes).
void recorded_animation_decode_control_flags_event_v1(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_control_flags_set_event_v1 *set_event =
        (recorded_animation_control_flags_set_event_v1 *)event;

    control->control_flags = set_event->control_flags;
    *cursor += sizeof(recorded_animation_control_flags_set_event_v1);
}

#if 0
Original Ghidra decompilation (0x44a690):

void FUN_0044a690(int param_1,int param_2,int *param_3)

{
  *(undefined2 *)(param_1 + 2) = *(undefined2 *)(param_2 + 4);
  *param_3 = *param_3 + 6;
  return;
}
#endif
