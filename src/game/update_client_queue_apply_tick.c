// update_client_queue_apply_tick  (Ghidra: FUN_004730d0; renamed, no established name)
// address 0x4730d0, size 416 bytes
// name confidence: 0.25   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Applies a queued client update for a given tick to
// the server's per-player runtime state"); types/game.h update_record (body opaque, "the packed
// per-player payload the network module encodes"); update_client_queue_get_slot.c (this batch).
// No register or stack input besides the two output arrays (stack: out_actions at [esp+0x24], out_carry at [esp+0x28] in the
// disassembly's frame). update_client_queue_get_slot takes the base tick in EAX (0x4730da). The per-player iterator element is
// written from the queue slot (+0x8 + index*0x20) and folded into out_actions (0x20 bytes) / out_carry (0x10 bytes from
// slot+0x208) for every live element.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t update_client_base_tick; // 0x006f7e9c
extern int32_t update_client_unknown_ea0; // 0x006f7ea0
extern data_array *update_client_queues;  // 0x006f7ed0

extern update_record *update_client_queue_get_slot(int32_t tick); // this batch, 0x473500
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// `out_actions` receives 0x20-byte records (types/game.h player_action) and
// `out_carry` 0x10-byte ones (types/game.h client_update_carry), both per-player over the same
// iteration; only the low byte of the return value is meaningful.
uint32_t update_client_queue_apply_tick(player_action *out_actions,
    client_update_carry *out_carry)
{
    update_record *slot = update_client_queue_get_slot(update_client_base_tick); // 0x4730d0: EAX = the base tick

    // 0x4730e3..0x4730f1: get_slot leaves EDX alone, so the compare is the base tick itself against 0x006f7ea0
    if (slot == 0 || update_client_base_tick > (int32_t)update_client_unknown_ea0) {
        return 0;
    }

    {
        data_iterator player_iter;
        void *player_element;
        int16_t index = -1;
        uint8_t *slot_bytes = (uint8_t *)slot;

        player_iter.data = update_client_queues; // 0x006f7ed0 (0x4730f8)
        player_iter.next_index = 0;
        player_iter.index = k_datum_index_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_element = data_iterator_next(&player_iter);
        while (player_element != 0) {
            index = index + 1;
            if (index < *(int16_t *)&((struct update_record *)slot_bytes)->player_count) {
                uint8_t *record = slot_bytes + 8 + (int32_t)index * 0x20;
                uint8_t *dst = (uint8_t *)player_element;

                *(uint32_t *)(dst + 4) = *(uint32_t *)(record + 0);
                *(uint32_t *)(dst + 0xc) = *(uint32_t *)(record + 4);
                *(uint32_t *)(dst + 0x10) = *(uint32_t *)(record + 8);
                *(uint32_t *)(dst + 0x14) = *(uint32_t *)(record + 0xc);
                *(uint32_t *)(dst + 0x18) = *(uint32_t *)(record + 0x10);
                *(uint32_t *)(dst + 0x1c) = *(uint32_t *)(record + 0x14);
                *(uint16_t *)(dst + 0x20) = *(uint16_t *)(record + 0x18);
                *(uint16_t *)(dst + 0x22) = *(uint16_t *)(record + 0x1a);
                *(uint16_t *)(dst + 0x24) = *(uint16_t *)(record + 0x1c);
            }
            player_element = data_iterator_next(&player_iter);
        }

        index = -1;
        player_iter.data = update_client_queues;
        player_iter.next_index = 0;
        player_iter.index = k_datum_index_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_element = data_iterator_next(&player_iter);
        while (player_element != 0) {
            uint8_t *dst = (uint8_t *)player_element;
            uint32_t *out_record = (uint32_t *)(((uint8_t *)out_actions) + (int32_t)(++index) * 0x20);
            uint32_t *carry_src = (uint32_t *)(slot_bytes + 0x208 + (int32_t)index * 0x10);
            uint32_t *carry_dst = (uint32_t *)(((uint8_t *)out_carry) + (int32_t)index * 0x10);
            int32_t k;

            out_record[0] = ~*(uint32_t *)(dst + 8) & *(uint32_t *)(dst + 4);
            *(uint32_t *)(dst + 8) = *(uint32_t *)(dst + 4) & 0x4d0;
            out_record[1] = *(uint32_t *)(dst + 0xc);
            out_record[2] = *(uint32_t *)(dst + 0x10);
            out_record[3] = *(uint32_t *)(dst + 0x14);
            out_record[4] = *(uint32_t *)(dst + 0x18);
            out_record[5] = *(uint32_t *)(dst + 0x1c);
            *(uint16_t *)(out_record + 6) = *(uint16_t *)(dst + 0x20);
            *(uint16_t *)((uint8_t *)out_record + 0x1a) = *(uint16_t *)(dst + 0x22);
            *(uint16_t *)(out_record + 7) = *(uint16_t *)(dst + 0x24);

            for (k = 0; k < 4; k++) {
                carry_dst[k] = carry_src[k];
            }
            player_element = data_iterator_next(&player_iter);
        }
    }

    update_client_base_tick = update_client_base_tick + 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4730d0), from tools/pack.py 0x4730d0:

uint FUN_004730d0(int param_1,int param_2)

{
  int iVar1;
  undefined4 *puVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint *puVar6;
  undefined4 *puVar7;
  short sVar8;
  undefined8 uVar9;

  uVar9 = update_client_queue_get_slot();
  uVar3 = (uint)uVar9;
  if ((uVar3 != 0) && ((int)((ulonglong)uVar9 >> 0x20) <= DAT_006f7ea0)) {
    sVar8 = -1;
    iVar4 = data_iterator_next();
    while (iVar4 != 0) {
      sVar8 = sVar8 + 1;
      if ((int)sVar8 < (int)(uint)*(ushort *)(uVar3 + 4)) {
        iVar5 = sVar8 * 0x20;
        iVar1 = iVar5 + 8 + uVar3;
        *(undefined4 *)(iVar4 + 4) = *(undefined4 *)(iVar5 + 8 + uVar3);
        *(undefined4 *)(iVar4 + 0xc) = *(undefined4 *)(iVar1 + 4);
        *(undefined4 *)(iVar4 + 0x10) = *(undefined4 *)(iVar1 + 8);
        *(undefined4 *)(iVar4 + 0x14) = *(undefined4 *)(iVar1 + 0xc);
        *(undefined4 *)(iVar4 + 0x18) = *(undefined4 *)(iVar1 + 0x10);
        *(undefined4 *)(iVar4 + 0x1c) = *(undefined4 *)(iVar1 + 0x14);
        *(undefined2 *)(iVar4 + 0x20) = *(undefined2 *)(iVar1 + 0x18);
        *(undefined2 *)(iVar4 + 0x22) = *(undefined2 *)(iVar1 + 0x1a);
        *(undefined2 *)(iVar4 + 0x24) = *(undefined2 *)(iVar1 + 0x1c);
      }
      iVar4 = data_iterator_next();
    }
    sVar8 = -1;
    iVar4 = data_iterator_next();
    while (iVar4 != 0) {
      sVar8 = sVar8 + 1;
      puVar6 = (uint *)(sVar8 * 0x20 + param_1);
      *puVar6 = ~*(uint *)(iVar4 + 8) & *(uint *)(iVar4 + 4);
      *(uint *)(iVar4 + 8) = *(uint *)(iVar4 + 4) & 0x4d0;
      puVar6[1] = *(uint *)(iVar4 + 0xc);
      puVar6[2] = *(uint *)(iVar4 + 0x10);
      puVar6[3] = *(uint *)(iVar4 + 0x14);
      puVar6[4] = *(uint *)(iVar4 + 0x18);
      puVar6[5] = *(uint *)(iVar4 + 0x1c);
      *(undefined2 *)(puVar6 + 6) = *(undefined2 *)(iVar4 + 0x20);
      *(undefined2 *)((int)puVar6 + 0x1a) = *(undefined2 *)(iVar4 + 0x22);
      *(undefined2 *)(puVar6 + 7) = *(undefined2 *)(iVar4 + 0x24);
      iVar4 = sVar8 * 0x10;
      puVar2 = (undefined4 *)(iVar4 + 0x208 + uVar3);
      puVar7 = (undefined4 *)(iVar4 + param_2);
      *puVar7 = *puVar2;
      puVar7[1] = puVar2[1];
      puVar7[2] = puVar2[2];
      puVar7[3] = puVar2[3];
      iVar4 = data_iterator_next();
    }
    DAT_006f7e9c = DAT_006f7e9c + 1;
    return CONCAT31((int3)((uint)DAT_006f7e9c >> 8),1);
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
