// recorded_animation_decode_looking_vector_event_v1  (Ghidra: missed_44a760; renamed per
// types/cutscene.h recorded_animation_event_type comment "_recorded_animation_event_v1_looking_vector
// = 11,  // 0x44a760")
// address 0x44a760, size 38 bytes
// name confidence: 0.85 (types/cutscene.h ties this exact address to the type-11 v1 event)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "0x44a700/730/760 a single vector ... Each one's
// cursor advance equals the record size." types/cutscene.h
// recorded_animation_multi_vector_set_event_v1 (size 0x10, real_vector3d vector at +0x04) and
// unit_control_data.looking_vector at +0x34 match exactly.
// register convention: recorded_animation_v1_event_proc (control, event, cursor), all cdecl
// stack arguments, same slots as recorded_animation_decode_animation_state_event_v1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: stack -> (control, event, cursor)
// Uncompressed (v1..v3) event type 11: copies event->vector into control->looking_vector and
// advances *cursor by the record's fixed size (0x10 bytes).
void recorded_animation_decode_looking_vector_event_v1(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_multi_vector_set_event_v1 *vector_event =
        (recorded_animation_multi_vector_set_event_v1 *)event;

    control->looking_vector = vector_event->vector;
    *cursor += sizeof(recorded_animation_multi_vector_set_event_v1);
}

#if 0
Original Ghidra decompilation (0x44a760):

void FUN_0044a760(int param_1,int param_2,int *param_3)

{
  *(undefined4 *)(param_1 + 0x34) = *(undefined4 *)(param_2 + 4);
  *(undefined4 *)(param_1 + 0x38) = *(undefined4 *)(param_2 + 8);
  *(undefined4 *)(param_1 + 0x3c) = *(undefined4 *)(param_2 + 0xc);
  *param_3 = *param_3 + 0x10;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
