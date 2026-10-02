// recorded_animation_decode_animation_state_event_v1  (Ghidra: missed_44a650; renamed per
// types/cutscene.h recorded_animation_animation_state_set_event_v1 comment "0x00 type 2 ...
// 0x04 0x44a650")
// address 0x44a650, size 21 bytes
// name confidence: 0.85 (types/cutscene.h ties this exact address to the type-2 v1 set event)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "Layouts come from the `bysw` records
// 0x686f0c..0x686fc4: `animation_event_v1` 4 (-2,-2), the set events 6/6/6/6 ... The handler
// bodies agree: 0x44a650 byte, 0x44a670 byte, 0x44a690 word, 0x44a6b0 word ... Each one's cursor
// advance equals the record size." types/cutscene.h
// recorded_animation_animation_state_set_event_v1 (size 6, animation_state at +0x04) matches
// exactly.
// register convention: recorded_animation_v1_event_proc (control, event, cursor), all cdecl
// stack arguments (objdump: [esp+4]=event, [esp+8]=control, [esp+0xc]=cursor -- Ghidra's own
// param_1/param_2/param_3 already match this typedef's order one-for-one).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: stack -> (control, event, cursor)
// Uncompressed (v1..v3) event type 2: copies event->animation_state into control->animation_state
// and advances *cursor by the record's fixed size (6 bytes).
void recorded_animation_decode_animation_state_event_v1(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_animation_state_set_event_v1 *set_event =
        (recorded_animation_animation_state_set_event_v1 *)event;

    control->animation_state = set_event->animation_state;
    *cursor += sizeof(recorded_animation_animation_state_set_event_v1);
}

#if 0
Original Ghidra decompilation (0x44a650):

void FUN_0044a650(undefined1 *param_1,int param_2,int *param_3)

{
  *param_1 = *(undefined1 *)(param_2 + 4);
  *param_3 = *param_3 + 6;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
