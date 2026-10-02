// recorded_animation_decode_weapon_index_event_v1  (Ghidra: missed_44a6b0; renamed per
// types/cutscene.h recorded_animation_weapon_index_set_event_v1 comment "0x00 type 5 ...
// 0x04 0x44a6b0")
// address 0x44a6b0, size 24 bytes
// name confidence: 0.85 (types/cutscene.h ties this exact address to the type-5 v1 set event)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "the set events 6/6/6/6 ... 0x44a6b0 word ...
// Each one's cursor advance equals the record size." types/cutscene.h
// recorded_animation_weapon_index_set_event_v1 (size 6, weapon_index at +0x04) matches exactly.
// register convention: recorded_animation_v1_event_proc (control, event, cursor), all cdecl
// stack arguments, same slots as recorded_animation_decode_animation_state_event_v1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (control, event, cursor)
// Uncompressed (v1..v3) event type 5: copies event->weapon_index into control->weapon_index and
// advances *cursor by the record's fixed size (6 bytes).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void recorded_animation_decode_weapon_index_event_v1(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_weapon_index_set_event_v1 *set_event =
        (recorded_animation_weapon_index_set_event_v1 *)event;

    control->weapon_index = set_event->weapon_index;
    *cursor += sizeof(recorded_animation_weapon_index_set_event_v1);
}

#if 0
Original Ghidra decompilation (0x44a6b0):

void FUN_0044a6b0(int param_1,int param_2,int *param_3)

{
  *(undefined2 *)(param_1 + 4) = *(undefined2 *)(param_2 + 4);
  *param_3 = *param_3 + 6;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
