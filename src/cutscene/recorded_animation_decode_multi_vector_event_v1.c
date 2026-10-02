// recorded_animation_decode_multi_vector_event_v1  (Ghidra: missed_44a820; renamed per
// types/cutscene.h recorded_animation_event_type comment
// "_recorded_animation_event_v1_multi_vector_first = 12,  // 0x44a820: 12 skips looking,
// _recorded_animation_event_v1_multi_vector_last = 15,   // 13 aiming, 14 facing, 15 none")
// address 0x44a820, size 104 bytes
// name confidence: 0.85 (types/cutscene.h ties this exact address to the type-12..15 v1 event
// and spells out the skip pattern)   rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "0x44a820 multi ... Each one's cursor advance
// equals the record size." types/cutscene.h recorded_animation_multi_vector_set_event_v1 (size
// 0x10, real_vector3d vector at +0x04, "0x00 types 9..15") matches exactly; the three
// unit_control_data direction vectors at +0x1c/+0x28/+0x34 match the skip order confirmed by
// objdump: `cmp word [event],0xe` (skip facing), then `cmp word [event],0xd` (skip aiming), then
// `cmp word [event],0xc` (skip looking) -- so type 15 writes all three, 14 skips facing, 13 skips
// aiming, 12 skips looking.
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
// Uncompressed (v1..v3) event types 12..15: copies event->vector into up to three of
// control->facing_vector / aiming_vector / looking_vector, skipping exactly one of them
// depending on event->type (14 skips facing, 13 skips aiming, 12 skips looking, 15 skips none),
// then advances *cursor by the record's fixed size (0x10 bytes).
void recorded_animation_decode_multi_vector_event_v1(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_multi_vector_set_event_v1 *vector_event =
        (recorded_animation_multi_vector_set_event_v1 *)event;

    if (event->type != 0xe) {
        control->facing_vector = vector_event->vector;
    }
    if (event->type != 0xd) {
        control->aiming_vector = vector_event->vector;
    }
    if (event->type != 0xc) {
        control->looking_vector = vector_event->vector;
    }
    *cursor += sizeof(recorded_animation_multi_vector_set_event_v1);
}

#if 0
Original Ghidra decompilation (0x44a820):

void FUN_0044a820(int param_1,short *param_2,int *param_3)

{
  if (*param_2 != 0xe) {
    *(undefined4 *)(param_1 + 0x1c) = *(undefined4 *)(param_2 + 2);
    *(undefined4 *)(param_1 + 0x20) = *(undefined4 *)(param_2 + 4);
    *(undefined4 *)(param_1 + 0x24) = *(undefined4 *)(param_2 + 6);
  }
  if (*param_2 != 0xd) {
    *(undefined4 *)(param_1 + 0x28) = *(undefined4 *)(param_2 + 2);
    *(undefined4 *)(param_1 + 0x2c) = *(undefined4 *)(param_2 + 4);
    *(undefined4 *)(param_1 + 0x30) = *(undefined4 *)(param_2 + 6);
  }
  if (*param_2 != 0xc) {
    *(undefined4 *)(param_1 + 0x34) = *(undefined4 *)(param_2 + 2);
    *(undefined4 *)(param_1 + 0x38) = *(undefined4 *)(param_2 + 4);
    *(undefined4 *)(param_1 + 0x3c) = *(undefined4 *)(param_2 + 6);
  }
  *param_3 = *param_3 + 0x10;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
