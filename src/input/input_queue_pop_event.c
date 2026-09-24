// input_queue_pop_event  (Ghidra: FUN_004922b0)
// address 0x4922b0, size 133 bytes
// name confidence: 0.65   rewrite confidence: 0.6
// evidence: types/input.h names this "input_queue_pop_event 0x4922b0 (pop: scans 0x00712d04 -
// 8k, copies two dwords, clears the kind word)". 0x00712d04 is
// event_queue.events[0][7] (0x00712ccc + 7*8); the pop walks slot 7 down to slot 0 of the
// requested queue and returns the highest-index slot whose kind word is nonzero, or tries all
// four queues (0..3) when queue_index is -1.
// register convention: plain stack arguments, confirmed by objdump: out_event at [esp+4]
// (0x4922c8 / 0x492310), queue_index at [esp+8] (0x4922bc); the -1 case recurses with
// push esi ; push edi. Returns AL (0 from the xor at 0x4922b6, 1 at 0x492334).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern input_event_queue event_queue; // 0x00712cc0

// Pops the newest queued event of queue_index (0..3), or the newest across all four queues when
// queue_index is -1. Returns 1 and fills *out_event on success, 0 (queue disabled, or nothing
// queued) otherwise; the popped slot's kind is cleared.
uint8_t input_queue_pop_event(ui_input_event *out_event, int16_t queue_index)
{
    int32_t slot;
    ui_input_event *record;

    if (event_queue.enabled == 0) {
        return 0;
    }
    if (queue_index != -1) {
        for (slot = 7; slot >= 0; slot--) {
            record = &event_queue.events[queue_index][slot];
            if (record->kind != 0) {
                *out_event = *record;
                record->kind = 0;
                return 1;
            }
        }
        return 0;
    }
    for (queue_index = 0; queue_index < 4; queue_index++) {
        if (input_queue_pop_event(out_event, queue_index) != 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4922b0):

uint FUN_004922b0(undefined4 *param_1,short param_2)

{
  uint in_EAX;
  uint uVar1;
  short *psVar2;
  int iVar3;

  uVar1 = in_EAX & 0xffffff00;
  if (DAT_00712cc0 != '\0') {
    if (param_2 != -1) {
      iVar3 = 7;
      psVar2 = (short *)(&DAT_00712d04 + param_2 * 0x40);
      do {
        if (*psVar2 != 0) {
          iVar3 = iVar3 + param_2 * 8;
          *param_1 = (&DAT_00712ccc)[iVar3 * 2];
          param_1[1] = (&DAT_00712cd0)[iVar3 * 2];
          *(undefined2 *)(&DAT_00712ccc + iVar3 * 2) = 0;
          return CONCAT31((int3)((uint)iVar3 >> 8),1);
        }
        iVar3 = iVar3 + -1;
        psVar2 = psVar2 + -4;
      } while (-1 < iVar3);
      return uVar1;
    }
    iVar3 = 0;
    do {
      if (3 < (short)iVar3) {
        return uVar1;
      }
      uVar1 = FUN_004922b0(param_1,iVar3);
      iVar3 = iVar3 + 1;
    } while ((char)uVar1 == '\0');
  }
  return uVar1;
}
#endif
