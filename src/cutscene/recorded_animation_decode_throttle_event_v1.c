// recorded_animation_decode_throttle_event_v1  (Ghidra: missed_44a6d0; renamed per
// types/cutscene.h recorded_animation_throttle_set_event_v1 comment "0x00 type 6 ...
// 0x04 0x44a6d0 stores i, j and zeroes k")
// address 0x44a6d0, size 35 bytes
// name confidence: 0.85 (types/cutscene.h ties this exact address to the type-6 v1 set event)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "0x44a6d0 two floats plus k=0 ... Each one's
// cursor advance equals the record size." types/cutscene.h recorded_animation_throttle_set_event_v1
// (size 0x0c, real_vector2d throttle at +0x04) matches exactly.
// register convention: recorded_animation_v1_event_proc (control, event, cursor), all cdecl
// stack arguments, same slots as recorded_animation_decode_animation_state_event_v1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (control, event, cursor)
// Uncompressed (v1..v3) event type 6: copies event->throttle.i / .j into control->throttle.i / .j,
// zeroes control->throttle.k, and advances *cursor by the record's fixed size (0x0c bytes).
void recorded_animation_decode_throttle_event_v1(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_throttle_set_event_v1 *set_event =
        (recorded_animation_throttle_set_event_v1 *)event;

    control->throttle.i = set_event->throttle.i;
    control->throttle.j = set_event->throttle.j;
    control->throttle.k = 0.0f;
    *cursor += sizeof(recorded_animation_throttle_set_event_v1);
}

#if 0
Original Ghidra decompilation (0x44a6d0):

void FUN_0044a6d0(int param_1,int param_2,int *param_3)

{
  *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(param_2 + 4);
  *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(param_2 + 8);
  *(undefined4 *)(param_1 + 0x14) = 0;
  *param_3 = *param_3 + 0xc;
  return;
}
#endif
