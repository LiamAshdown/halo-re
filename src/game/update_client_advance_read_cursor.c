// update_client_advance_read_cursor  (Ghidra: FUN_004734b0; renamed, no established name)
// address 0x4734b0, size 76 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Advances the client update-queue read cursor toward a
// target tick, discarding any stale intervening entries"); update_client_queue_get_slot.c (this
// batch).
// UNSURE: this is a low-confidence, mostly-literal transcription; `unaff_EBX` (the target tick)
// and `extraout_EDX` (the tick value the second get_slot call leaves behind) are register
// artifacts Ghidra could not bind to real parameters, and this function's own signature is
// modeled loosely around them.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t update_client_unknown_ea0; // 0x006f7ea0

extern update_record *update_client_queue_get_slot(int32_t tick); // this batch, 0x473500

// UNSURE: see header.
void update_client_advance_read_cursor(int32_t target_tick)
{
    update_record *slot = update_client_queue_get_slot(0); // UNSURE: real tick argument not recovered
    int32_t cursor = update_client_unknown_ea0;

    if (slot != 0) {
        slot->tick = target_tick;

        if (update_client_unknown_ea0 < target_tick) {
            int32_t previous = update_client_unknown_ea0;

            while (previous + 1 < target_tick) {
                update_client_queue_get_slot(0); // UNSURE: real tick argument not recovered
                slot->player_count = 0xffff;
                previous = target_tick; // UNSURE: mirrors Ghidra's `extraout_EDX` re-read, which
                                          // this transcription cannot reproduce exactly
            }
            cursor = target_tick;
        }
    }
    update_client_unknown_ea0 = cursor;
}

#if 0
Original Ghidra decompilation (0x4734b0), from tools/pack.py 0x4734b0:

void FUN_004734b0(void)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int extraout_EDX;
  int unaff_EBX;
  int *piVar4;
  int *piVar5;
  undefined8 uVar6;

  uVar6 = update_client_queue_get_slot();
  piVar2 = (int *)uVar6;
  iVar3 = DAT_006f7ea0;
  if (piVar2 != (int *)0x0) {
    *piVar2 = unaff_EBX;
    piVar4 = (int *)((ulonglong)uVar6 >> 0x20);
    piVar5 = piVar2 + 1;
    for (iVar3 = 0xc1; iVar3 != 0; iVar3 = iVar3 + -1) {
      *piVar5 = *piVar4;
      piVar4 = piVar4 + 1;
      piVar5 = piVar5 + 1;
    }
    iVar3 = DAT_006f7ea0;
    iVar1 = DAT_006f7ea0;
    if (DAT_006f7ea0 < unaff_EBX) {
      while (iVar3 = unaff_EBX, iVar1 + 1 < unaff_EBX) {
        update_client_queue_get_slot();
        *(undefined2 *)(piVar2 + 1) = 0xffff;
        iVar1 = extraout_EDX;
      }
    }
  }
  DAT_006f7ea0 = iVar3;
  return;
}
#endif
