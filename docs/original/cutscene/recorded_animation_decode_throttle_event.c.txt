// recorded_animation_decode_throttle_event  (Ghidra: missed_44a0e0; renamed per
// types/cutscene.h recorded_animation_event_type: "_recorded_animation_event_throttle = 6,
// // unit_control_data.throttle i, j (k := 0)")
// address 0x44a0e0, size 34 bytes
// name confidence: 0.85 (types/cutscene.h documents the exact type number and target field)
// rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0" and
// the header's own comment on this event type. unit_control_data.throttle (real_vector3d) at
// +0x0c (types/units.h) matches: two dwords copied to +0x0c/+0x10 (i, j) and a literal 0 stored
// at +0x14 (k).
// register convention: cdecl stack parameters (state, control, header, cursor); objdump confirms
// only [esp+8] (control) and [esp+0x10] (cursor) are read (plus a callee-saved ESI, restored
// before ret). blam-cc: stack -> (state, control, header, cursor).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: stack -> (state, control, header, cursor)
// Compressed event type 6: copies two dwords from *cursor into control->throttle.i / .j, zeroes
// control->throttle.k, and advances *cursor by 8. state and header are unused.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void recorded_animation_decode_throttle_event(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    uint32_t *source;

    source = (uint32_t *)*cursor;
    *(uint32_t *)&control->throttle.i = source[0];
    *(uint32_t *)&control->throttle.j = source[1];
    control->throttle.k = 0.0f;
    *cursor += 8;
}

#if 0
Original Ghidra decompilation (0x44a0e0):

void FUN_0044a0e0(undefined4 param_1,int param_2,undefined4 param_3,int *param_4)

{
  undefined4 *puVar1;

  puVar1 = (undefined4 *)*param_4;
  *(undefined4 *)(param_2 + 0xc) = *puVar1;
  *(undefined4 *)(param_2 + 0x10) = puVar1[1];
  *(undefined4 *)(param_2 + 0x14) = 0;
  *param_4 = *param_4 + 8;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
