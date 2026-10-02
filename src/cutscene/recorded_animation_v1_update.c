// recorded_animation_v1_update  (Ghidra: missed_44a8b0; renamed per types/cutscene.h's codec
// table comment: "update: 0x44a590 compressed, 0x44a8b0 v1. Both fire every event whose delay
// fits in *event_ticks ... and return 0 only when the end event is due exactly now ... 1
// otherwise")
// address 0x44a8b0, size 114 bytes
// name confidence: 0.75 (types/cutscene.h names the codec and describes this entry point's job)
// rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md "The dispatch is 0x44a8b0: `event.type == 1` means
// end, the handler comes from `0x686ea8[type]`, and a NULL handler means the cursor moves past
// the 4-byte header. The loop runs while `*event_ticks >= delay`." types/cutscene.h
// recorded_animation_event_v1 (type/delay_ticks) and recorded_animation_v1_event_handlers at
// 0x686ea8 (size 0x17) match exactly.
// register convention: recorded_animation_update_proc (state, control, event_ticks, cursor),
// all cdecl stack arguments -- same call-site shape as recorded_animation_compressed_update
// (0x44aa90, recorded_animations_update). state (the first argument) is loaded but never used,
// matching types/cutscene.h's "the v1 codec never touches [the decoder state]".
// Restructuring note: the original is a do-while whose body tests `event->type == 1` first and
// jumps directly to the shared end-of-function check; that check is exactly
// "if the current event is the end marker and its delay equals *event_ticks, return 0, else
// return 1" whether reached from inside the loop or by falling out of it when the next event's
// delay has not yet elapsed. This is rewritten as a single while-loop whose condition folds in
// that same type/delay test (matching the equivalent restructuring already used by
// unit_control_data_unpack.c for a similar Ghidra do-while/goto shape); the observable behaviour
// (which events fire, in what order, with what cursor/tick side effects, and the return value)
// is unchanged.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern recorded_animation_v1_event_proc
    recorded_animation_v1_event_handlers[k_recorded_animation_event_type_count]; // 0x00686ea8

// blam-cc: stack -> (state, control, event_ticks, cursor)
// Uncompressed (versions 1..3) codec update. Repeatedly looks at the event record at *cursor:
// while it is not the end marker (type 1) and its delay_ticks has already elapsed
// (*event_ticks >= delay_ticks), dispatches to recorded_animation_v1_event_handlers[type] (which
// advances *cursor itself), or -- for a type with no handler -- advances *cursor past just the
// 4-byte header, then subtracts the consumed delay from *event_ticks. Stops at the first event
// whose delay has not yet elapsed, or at the end event; returns 0 only when that final event is
// the end marker and its delay exactly matches *event_ticks (animation finished this tick), 1
// otherwise. state is unused (the v1 codec carries no decoder state between events).
uint8_t recorded_animation_v1_update(recorded_animation_decoder_state *state, unit_control_data *control,
    int32_t *event_ticks, uint8_t **cursor)
{
    recorded_animation_event_v1 *event;
    recorded_animation_v1_event_proc handler;

    (void)state;

    event = (recorded_animation_event_v1 *)*cursor;

    while (event->type != _recorded_animation_event_end && event->delay_ticks <= *event_ticks) {
        handler = recorded_animation_v1_event_handlers[event->type];
        if (handler != (recorded_animation_v1_event_proc)0) {
            handler(control, event, cursor);
        } else {
            *cursor = (uint8_t *)(event + 1);
        }
        *event_ticks -= event->delay_ticks;
        event = (recorded_animation_event_v1 *)*cursor;
    }

    if (event->type == _recorded_animation_event_end && *event_ticks == event->delay_ticks) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x44a8b0):

uint FUN_0044a8b0(undefined4 param_1,undefined4 param_2,uint *param_3,int *param_4)

{
  ushort uVar1;
  uint uVar2;
  ushort *puVar3;

  puVar3 = (ushort *)*param_4;
  uVar2 = (uint)puVar3[1];
  if ((int)uVar2 <= (int)*param_3) {
    do {
      uVar1 = *puVar3;
      uVar2 = (uint)uVar1;
      if (uVar1 == 1) goto LAB_0044a90c;
      if (*(code **)(&DAT_00686ea8 + (short)uVar1 * 4) == (code *)0x0) {
        *param_4 = (int)(puVar3 + 2);
      }
      else {
        (**(code **)(&DAT_00686ea8 + (short)uVar1 * 4))(param_2,puVar3,param_4);
      }
      uVar2 = (uint)puVar3[1];
      *param_3 = *param_3 - uVar2;
      puVar3 = (ushort *)*param_4;
    } while ((int)(uint)puVar3[1] <= (int)*param_3);
  }
  if (*puVar3 == 1) {
LAB_0044a90c:
    if (*param_3 == (uint)puVar3[1]) {
      return uVar2 & 0xffffff00;
    }
  }
  return CONCAT31((int3)(uVar2 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
