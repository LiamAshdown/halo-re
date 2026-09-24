// player_update_queue_pop_current  (Ghidra: FUN_00479fb0; named per
// out/phase4/game_functions.md: "Peeks the head record of the event queue, decrementing its
// reference count and removing it only once no references remain.")
// address 0x479fb0, size 108 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x479fb0
//   --stop-address=0x47a01c) and against both call sites (0x474590 at 0x47468d, 0x4740a0):
//   types/game.h player_update_queue (queue 0x00, has_current 0x18, current[8] 0x1c) and
//   player_update_record (0x2c: field0, references_remaining 0x04, reference_count 0x08, and a
//   player_action tail at 0x0c). "dec [ebp+0x4]" is the countdown, the 11-dword rep movs is the
//   whole record, and the "lea esi,[ebp+0xc] / lea edi,[ebx+0x1c] / 8 dwords" pair is the
//   player_action tail into player_update_queue::current -- which is what fixes the record type.
// CORRECTED by review, two things the first pass got wrong:
//   - the return type. 0x479fb4 is "or eax,0xffffffff" and 0x479fcd is "mov al,0x1", so on
//     success EAX is 0xffffff01, not 1. Only AL is meaningful, and both callers test it with
//     "cmp al,0x1". Declared uint8_t.
//   - the parameter order, which was (queue, out) against a stated blam-cc of EAX -> out,
//     EBX -> queue. House style is that the C parameter order follows the register order.
// register convention: an 11-dword output record in EAX (in_EAX); the queue in EBX (unaff_EBX).
//   // blam-cc: EAX -> out, EBX -> queue
// UNSURE: player_update_record::field0's identity (forwarded whole to 0x476cf0). The
//   "re-peek after the refcount reaches zero, then pop" sequence is preserved literally even
//   though its defensive empty-queue branch (record = NULL, then a rep movs from NULL) cannot be
//   reached in practice -- nothing else can run between the initial peek and this re-peek --
//   exactly as compiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// blam-cc: EAX -> out, EBX -> queue
// Peeks the queue's head record and decrements its references_remaining. If that reaches zero,
// actually advances the queue's read cursor past it. Either way, copies the (still-valid) 11
// dwords of that record into `out`, latches has_current, and copies the record's player_action
// tail into the queue's own `current`. Returns 1, unless the queue was empty to begin with
// (returns 0 with `out` left holding only the three -1 sentinels) or the defensive re-peek
// branch above finds the queue unexpectedly empty (returns 0, though `out` is still written from
// a NULL record in that unreachable case, exactly as compiled).
uint8_t player_update_queue_pop_current(player_update_record *out, player_update_queue *queue)
{
    uint32_t *raw_out = (uint32_t *)out;
    uint32_t *record;
    uint8_t result;
    int32_t i;

    raw_out[0] = 0xffffffff;
    raw_out[1] = 0xffffffff;
    raw_out[2] = 0xffffffff;

    if (queue->queue.read_index == queue->queue.write_index) {
        return 0;
    }
    record = (uint32_t *)queue->queue.records[queue->queue.read_index];
    result = 1;

    record[1] = record[1] - 1;
    if (record[1] == 0) {
        if (queue->queue.read_index == queue->queue.write_index) {
            record = 0; // UNSURE: unreachable in practice, see header note
            result = 0;
        } else {
            record = (uint32_t *)queue->queue.records[queue->queue.read_index];
            queue->queue.read_index = (queue->queue.read_index + 1) % queue->queue.capacity;
            result = 1;
        }
    }

    for (i = 0; i < 11; i++) {
        raw_out[i] = record[i];
    }
    queue->has_current = 1;
    for (i = 0; i < 8; i++) {
        queue->current[i] = (int32_t)record[3 + i];
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x479fb0), from tools/pack.py 0x479fb0:

int FUN_00479fb0(void)

{
  undefined4 *in_EAX;
  uint3 uVar3;
  int iVar1;
  int iVar2;
  int *unaff_EBX;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int *piVar6;
  int *piVar7;
  bool bVar8;

  *in_EAX = 0xffffffff;
  in_EAX[1] = 0xffffffff;
  in_EAX[2] = 0xffffffff;
  iVar2 = unaff_EBX[4];
  uVar3 = (uint3)((uint)iVar2 >> 8);
  if (iVar2 == unaff_EBX[3]) {
    puVar4 = (undefined4 *)0x0;
    iVar2 = (uint)uVar3 << 8;
  }
  else {
    puVar4 = *(undefined4 **)(unaff_EBX[2] + iVar2 * 4);
    iVar2 = CONCAT31(uVar3,1);
  }
  if ((char)iVar2 == '\x01') {
    piVar6 = puVar4 + 1;
    *piVar6 = *piVar6 + -1;
    if (*piVar6 == 0) {
      iVar2 = unaff_EBX[4];
      bVar8 = iVar2 == unaff_EBX[3];
      if (bVar8) {
        puVar4 = (undefined4 *)0x0;
      }
      else {
        puVar4 = *(undefined4 **)(unaff_EBX[2] + iVar2 * 4);
        iVar1 = iVar2 + 1;
        iVar2 = iVar1 / *unaff_EBX;
        unaff_EBX[4] = iVar1 % *unaff_EBX;
      }
      iVar2 = CONCAT31((int3)((uint)iVar2 >> 8),!bVar8);
    }
    puVar5 = puVar4;
    for (iVar1 = 0xb; iVar1 != 0; iVar1 = iVar1 + -1) {
      *in_EAX = *puVar5;
      puVar5 = puVar5 + 1;
      in_EAX = in_EAX + 1;
    }
    *(undefined1 *)(unaff_EBX + 6) = 1;
    piVar6 = puVar4 + 3;
    piVar7 = unaff_EBX + 7;
    for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
      *piVar7 = *piVar6;
      piVar6 = piVar6 + 1;
      piVar7 = piVar7 + 1;
    }
  }
  return iVar2;
}
#endif
