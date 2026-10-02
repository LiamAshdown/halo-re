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
// FIXED (register inputs, objdump): EBX carries target_tick (read at 0x4734b0, `mov eax,ebx`
//   right before the first update_client_queue_get_slot call); the rewrite already had a
//   target_tick C parameter but never annotated it and passed a literal 0 to that first call
//   instead. The second get_slot call inside the loop uses a different, still-unresolved
//   register (Ghidra's extraout_EDX) and is left as-is.
//   // blam-cc: EBX -> target_tick, EDX -> record

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t update_client_unknown_ea0; // 0x006f7ea0

extern update_record *update_client_queue_get_slot(int32_t tick); // this batch, 0x473500

// REWRITTEN (objdump 0x4734b0..0x4734fb, 2026-09-24): EBX is the tick and EDX a pointer to the 0x304-byte
//   update record, which is copied into the queue slot after its tick word (`mov esi,edx; rep movs`, 0xc1
//   dwords). When the tick runs ahead of the cursor at 0x6f7ea0, every skipped tick's slot is fetched and the
//   first word of THIS slot's record is set to 0xffff (the original writes [ebp], this slot, each time), and
//   the cursor becomes the tick. The draft copied nothing, never stored the cursor, and passed 0 as the tick in
//   the loop; hooked, the local player's actions never reached the queue (in game: could not move or shoot).
//   update_client_queue_get_slot (0x473500) preserves EDX, which is why EDX survives the first call.
// blam-cc: EBX -> target_tick, EDX -> record
void update_client_advance_read_cursor(int32_t target_tick, const uint32_t *record)
{
    update_record *slot = update_client_queue_get_slot(target_tick);
    int32_t tick;

    if (slot == 0) {
        return;
    }
    slot->tick = target_tick;
    memcpy((uint8_t *)slot + 4, record, 0xc1 * 4);
    if (target_tick > update_client_unknown_ea0) {
        for (tick = update_client_unknown_ea0 + 1; tick < target_tick; tick++) {
            update_client_queue_get_slot(tick);
            ((struct update_record *)slot)->player_count = 0xffff;
        }
        update_client_unknown_ea0 = target_tick;
    }
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
