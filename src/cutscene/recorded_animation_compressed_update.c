// recorded_animation_compressed_update  (Ghidra: missed_44a590; renamed per types/cutscene.h's
// codec table comment: "update: 0x44a590 compressed, 0x44a8b0 v1. Both fire every event whose
// delay fits in *event_ticks ... and return 0 only when the end event is due exactly now ...
// 1 otherwise")
// address 0x44a590, size 169 bytes
// name confidence: 0.75 (types/cutscene.h names the codec and describes this entry point's job)
// rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md "There is no fixed record. The event byte holds
// the type in bits 2..7 and the delay encoding in bits 0..1 (jump table at 0x44a63c: 0 ticks/1
// byte, 1 tick/1 byte, u8/2 bytes, u16/3 bytes). End is `(byte & 0xfc) == 4`. Handlers live in
// `0x686d98[type]` and take (state, control, header, cursor)." types/cutscene.h
// recorded_animation_compressed_delay and recorded_animation_update_proc match exactly; the
// global list gives recorded_animation_compressed_event_handlers at 0x686d98, size 0x17.
// register convention: recorded_animation_update_proc (state, control, event_ticks, cursor), all
// cdecl stack arguments -- objdump-confirmed at the call site 0x44aa90 (recorded_animations_update),
// which pushes (record+0x54, record+0x14, &record->event_ticks (+0xc), record+0x10) in that
// order through the same codec-table indirection used by the begin entry point.
// UNSURE: the jump table at 0x44a63c is data, not code (immediately after this function's `ret`);
// objdump disassembles it as garbage instructions, which is expected and not a rewrite error.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern recorded_animation_compressed_event_proc
    recorded_animation_compressed_event_handlers[k_recorded_animation_event_type_count]; // 0x00686d98

// blam-cc: stack -> (state, control, event_ticks, cursor)
// Compressed (version 4) codec update. Repeatedly looks at the event byte at *cursor: bits 0..1
// select how many ticks after the previous event it fires and how many header bytes to step over
// (recorded_animation_compressed_delay); while that delay has already elapsed (*event_ticks >=
// delay) and the event is not the end marker ((byte & 0xfc) == 4), advances *cursor past the
// header, dispatches to recorded_animation_compressed_event_handlers[byte >> 2] if that slot is
// non-NULL, and subtracts the consumed delay from *event_ticks. Stops at the first event whose
// delay has not yet elapsed, or at the end event; returns 0 only when that final event is the end
// marker and its delay exactly matches *event_ticks (animation finished this tick), 1 otherwise.
uint8_t recorded_animation_compressed_update(recorded_animation_decoder_state *state,
    unit_control_data *control, int32_t *event_ticks, uint8_t **cursor)
{
    uint8_t *event;
    int32_t delay;
    int32_t header_bytes;
    recorded_animation_compressed_event_proc handler;

    for (;;) {
        event = *cursor;

        switch (*event & k_recorded_animation_compressed_delay_mask) {
        case _recorded_animation_delay_none:
            delay = 0;
            header_bytes = 1;
            break;
        case _recorded_animation_delay_one_tick:
            delay = 1;
            header_bytes = 1;
            break;
        case _recorded_animation_delay_byte:
            delay = event[1];
            header_bytes = 2;
            break;
        default: // _recorded_animation_delay_word
            delay = *(uint16_t *)(event + 1);
            header_bytes = 3;
            break;
        }

        if (*event_ticks < delay || (*event & 0xfc) == 4) {
            break;
        }

        *cursor = event + header_bytes;
        handler = recorded_animation_compressed_event_handlers[*event >> k_recorded_animation_compressed_type_shift];
        if (handler != (recorded_animation_compressed_event_proc)0) {
            handler(state, control, event, cursor);
        }
        *event_ticks -= delay;
    }

    if ((*event & 0xfc) == 4 && *event_ticks == delay) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x44a590):

uint FUN_0044a590(undefined4 param_1,undefined4 param_2,int *param_3,undefined4 *param_4)

{
  byte *pbVar1;
  int iVar2;
  byte *pbVar3;

  while( true ) {
    pbVar1 = (byte *)*param_4;
    switch(*pbVar1 & 3) {
    case 0:
      pbVar3 = (byte *)0x0;
      iVar2 = 1;
      break;
    case 1:
      pbVar3 = (byte *)0x1;
      iVar2 = 1;
      break;
    case 2:
      pbVar3 = (byte *)(uint)pbVar1[1];
      iVar2 = 2;
      break;
    case 3:
      pbVar3 = (byte *)(uint)*(ushort *)(pbVar1 + 1);
      iVar2 = 3;
    }
    if ((*param_3 < (int)pbVar3) || ((*pbVar1 & 0xfc) == 4)) break;
    *param_4 = pbVar1 + iVar2;
    if (*(code **)(&DAT_00686d98 + (uint)(*pbVar1 >> 2) * 4) != (code *)0x0) {
      (**(code **)(&DAT_00686d98 + (uint)(*pbVar1 >> 2) * 4))(param_1,param_2,pbVar1,param_4);
    }
    *param_3 = *param_3 - (int)pbVar3;
  }
  if (((*pbVar1 & 0xfc) == 4) && (pbVar1 = (byte *)*param_3, pbVar1 == pbVar3)) {
    return (uint)pbVar1 & 0xffffff00;
  }
  return CONCAT31((int3)((uint)pbVar1 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
