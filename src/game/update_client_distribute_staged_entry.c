// update_client_distribute_staged_entry  (Ghidra: FUN_00473270; renamed, no established name)
// address 0x473270, size 152 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Distributes the currently staged client update entry
// into a per-player output array and advances the update tick counter");
// update_client_stage_entry.c (this batch, 0x473090) for the staged 8-dword record.
// UNSURE: `out` is written as a flat array of 0x20-byte per-player records (one per data_iterator
// element, over an unnamed array); DAT_006f7ec4/0x006f7ec8 are not attested in any header this
// module owns.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern uint32_t update_client_staged[8];   // 0x006f7ea4, see update_client_stage_entry.c
extern uint32_t update_client_unknown_ec8;  // 0x006f7ec8, UNSURE
extern int32_t update_client_unknown_ec4;    // 0x006f7ec4, UNSURE
extern int32_t update_client_base_tick;       // 0x006f7e9c

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// UNSURE: see header. Copies the staged 8-dword entry into every element of `out` (0x20 bytes
// each, one per iterated element), overwriting dword 0 with a masked value derived from
// update_client_unknown_ec8, and decrements update_client_unknown_ec4 the first time through.
// Advances update_client_base_tick and returns a packed (0, success) result.
uint32_t update_client_distribute_staged_entry(uint8_t *out)
{
    uint32_t masked = ~update_client_unknown_ec8 & update_client_staged[0];
    data_iterator iter;
    void *element;
    int32_t index = -1;

    update_client_unknown_ec8 = update_client_staged[0] & 0x4d0;

    iter.data = 0; // UNSURE: iteration source not recovered
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    element = data_iterator_next(&iter);
    while (element != 0) {
        uint32_t *record;
        int32_t i;

        index = index + 1;
        record = (uint32_t *)(out + (int32_t)index * 0x20);
        for (i = 0; i < 8; i++) {
            record[i] = update_client_staged[i];
        }
        record[0] = masked;
        if (index == 0) {
            update_client_unknown_ec4 = update_client_unknown_ec4 - 1;
        }
        element = data_iterator_next(&iter);
    }

    update_client_base_tick = update_client_base_tick + 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x473270), from tools/pack.py 0x473270:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00473270(int param_1)

{
  int iVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  uint *puVar5;
  uint *puVar6;

  uVar4 = ~DAT_006f7ec8 & DAT_006f7ea4;
  DAT_006f7ec8 = DAT_006f7ea4 & 0x4d0;
  iVar3 = -1;
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar3 = iVar3 + 1;
    puVar2 = (uint *)(iVar3 * 0x20 + param_1);
    puVar5 = &DAT_006f7ea4;
    puVar6 = puVar2;
    for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    *puVar2 = uVar4;
    if (iVar3 == 0) {
      _DAT_006f7ec4 = _DAT_006f7ec4 + -1;
    }
    iVar1 = data_iterator_next();
  }
  DAT_006f7e9c = DAT_006f7e9c + 1;
  return CONCAT31((int3)((uint)DAT_006f7e9c >> 8),1);
}
#endif
