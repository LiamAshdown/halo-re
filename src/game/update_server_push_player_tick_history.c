// update_server_push_player_tick_history  (Ghidra: FUN_00472cc0; renamed, no established name)
// address 0x472cc0, size 480 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Pushes a new server-tick history entry for every
// player into a 32-deep per-player ring buffer used for network delta reconciliation");
// update_server_new.c / update_server_queue_get_history_entry.c (this batch) for the sibling
// server-queue globals.
// UNSURE: this is the lowest-confidence, most literal transcription in the batch -- a
// near-verbatim, raw-pointer transliteration of Ghidra's own decompile (itself already
// low-confidence at 0.4) rather than a field-accurate rewrite. None of the per-record offsets
// are attested in any header this module owns. Kept this way rather than guessed at further
// given the time available for this batch; a follow-up pass with the real disassembly is needed
// before this file can be trusted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t update_server_tick;              // 0x006f1d8c
extern uint32_t update_server_history_raw[32 * (0x308 / 4)]; // 0x006f1d94, raw dword view of
                                                              // update_server_history

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern void update_client_advance_read_cursor(void); // this batch, 0x4734b0

// UNSURE: see header.
void update_server_push_player_tick_history(void)
{
    uint32_t *ring_slot;
    uint16_t *player_count;
    void *element;
    data_iterator iter;

    if (update_server_tick < update_server_tick + 1 && update_server_tick - 0x1f <= update_server_tick) {
        ring_slot = &update_server_history_raw[(update_server_tick & 0x1f) * 0xc2];
    } else {
        ring_slot = 0;
    }

    ring_slot[0] = (uint32_t)update_server_tick;
    update_server_tick = update_server_tick + 1;

    player_count = (uint16_t *)(ring_slot + 1);
    *player_count = 0;

    iter.data = 0; // UNSURE: iteration source not recovered (likely update_server_queues)
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    element = data_iterator_next(&iter);

    while (element != 0) {
        uint32_t *entry = (uint32_t *)element;
        uint8_t *entry_bytes = (uint8_t *)element;
        int32_t write_index = *(int32_t *)(entry_bytes + 0x38);
        uint8_t queue_empty = write_index == *(int32_t *)(entry_bytes + 0x34);
        uint32_t *record_ptr = queue_empty ? 0 : *(uint32_t **)(*(int32_t *)(entry_bytes + 0x30) + write_index * 4);
        uint8_t have_record = !queue_empty;
        uint32_t local_record[11];

        if (have_record) {
            uint32_t *refcount = record_ptr + 1;

            *refcount = *refcount - 1;
            if (*refcount == 0) {
                write_index = *(int32_t *)(entry_bytes + 0x38);
                queue_empty = write_index == *(int32_t *)(entry_bytes + 0x34);
                if (queue_empty) {
                    record_ptr = 0;
                } else {
                    record_ptr = *(uint32_t **)(*(int32_t *)(entry_bytes + 0x30) + write_index * 4);
                    *(int32_t *)(entry_bytes + 0x38) = (write_index + 1) % *(int32_t *)(entry_bytes + 0x28);
                }
                have_record = !queue_empty;
            }

            {
                int32_t i;

                for (i = 0; i < 11; i++) {
                    local_record[i] = record_ptr[i];
                }
            }
            entry_bytes[0x40] = 1;
            {
                int32_t i;

                for (i = 0; i < 8; i++) {
                    entry[0x11 + i] = record_ptr[3 + i]; // entry+0x44, 8 dwords
                }
            }

            if (have_record) {
                int32_t i;
                uint32_t *dst;

                for (i = 0; i < 8; i++) {
                    entry[2 + i] = local_record[3 + i]; // entry+8, 8 dwords
                }
                dst = ring_slot + (uint32_t)(uint16_t)*player_count * 8 + 2;
                for (i = 0; i < 8; i++) {
                    dst[i] = local_record[3 + i];
                }
                dst = ring_slot + (uint32_t)(uint16_t)*player_count * 4 + 0x82;
                dst[0] = (local_record[1] == 0) ? 0x0100 : 0x0001; // UNSURE: packed byte layout,
                    // see Ghidra's CONCAT11(local_2c[1]==0, 1) into the low word
                dst[1] = local_record[0];
                dst[2] = local_record[2];
                dst[3] = local_record[1];
                *player_count = *player_count + 1;
                element = data_iterator_next(&iter);
                continue;
            }
        }

        {
            int32_t i;
            uint32_t *dst = ring_slot + (uint32_t)(uint16_t)*player_count * 8 + 2;

            for (i = 0; i < 8; i++) {
                dst[i] = entry[2 + i]; // entry+8, 8 dwords
            }
            dst = ring_slot + (uint32_t)(uint16_t)*player_count * 4 + 0x82;
            dst[0] = 0;
            dst[1] = 0xffffffff;
            dst[2] = 0; // UNSURE: Ghidra's local_44/local_40 here are uninitialized locals in its
            dst[3] = 0; // own rendering; modeled as 0 pending a disassembly pass
        }

        *player_count = *player_count + 1;
        element = data_iterator_next(&iter);
    }

    if (0) {
        update_client_advance_read_cursor(); // Ghidra reaches this only when the iterator's very first call returns
                         // NULL, which this loop's `while` structure already short-circuits;
                         // kept here, unreachable, so the call is not silently dropped.
    }
}

#if 0
Original Ghidra decompilation (0x472cc0), from tools/pack.py 0x472cc0:

void FUN_00472cc0(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  uint *puVar5;
  uint *puVar6;
  bool bVar7;
  uint *local_68;
  uint *local_64;
  uint local_4c;
  uint local_44;
  uint local_40;
  undefined4 local_3c;
  uint local_2c [11];

  if (((int)DAT_006f1d8c < (int)(DAT_006f1d8c + 1)) &&
     ((int)(DAT_006f1d8c - 0x1f) <= (int)DAT_006f1d8c)) {
    local_68 = &DAT_006f1d94 + (DAT_006f1d8c & 0x1f) * 0xc2;
  }
  else {
    local_68 = (uint *)0x0;
  }
  iVar2 = DAT_006f1d8c + 1;
  *local_68 = DAT_006f1d8c;
  DAT_006f1d8c = iVar2;
  puVar4 = local_68 + 1;
  *(ushort *)puVar4 = 0;
  iVar2 = data_iterator_next();
  do {
    if (iVar2 == 0) {
      FUN_004734b0();
      return;
    }
    iVar3 = *(int *)(iVar2 + 0x38);
    bVar7 = iVar3 == *(int *)(iVar2 + 0x34);
    if (bVar7) {
      local_64 = (uint *)0x0;
    }
    else {
      local_64 = *(uint **)(*(int *)(iVar2 + 0x30) + iVar3 * 4);
    }
    bVar7 = !bVar7;
    if (bVar7) {
      puVar5 = local_64 + 1;
      *puVar5 = *puVar5 - 1;
      if (*puVar5 == 0) {
        iVar3 = *(int *)(iVar2 + 0x38);
        bVar7 = iVar3 == *(int *)(iVar2 + 0x34);
        if (bVar7) {
          local_64 = (uint *)0x0;
        }
        else {
          local_64 = *(uint **)(*(int *)(iVar2 + 0x30) + iVar3 * 4);
          *(int *)(iVar2 + 0x38) = (iVar3 + 1) % *(int *)(iVar2 + 0x28);
        }
        bVar7 = !bVar7;
      }
      puVar5 = local_64;
      puVar6 = local_2c;
      for (iVar3 = 0xb; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      *(undefined1 *)(iVar2 + 0x40) = 1;
      puVar5 = local_64 + 3;
      puVar6 = (uint *)(iVar2 + 0x44);
      for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      if (!bVar7) goto LAB_00472e2e;
      puVar5 = local_2c + 3;
      puVar6 = (uint *)(iVar2 + 8);
      for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      puVar5 = local_2c + 3;
      puVar6 = local_68 + (uint)(ushort)*puVar4 * 8 + 2;
      for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      puVar5 = local_68 + (uint)(ushort)*puVar4 * 4 + 0x82;
      uVar1 = local_3c >> 0x10;
      local_3c._2_2_ = (undefined2)uVar1;
      local_3c._0_2_ = CONCAT11(local_2c[1] == 0,1);
      *puVar5 = local_3c;
      puVar5[1] = local_2c[0];
      puVar5[2] = local_2c[2];
      puVar5[3] = local_2c[1];
    }
    else {
LAB_00472e2e:
      puVar5 = (uint *)(iVar2 + 8);
      puVar6 = local_68 + (uint)(ushort)*puVar4 * 8 + 2;
      for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      puVar5 = local_68 + (uint)(ushort)*puVar4 * 4 + 0x82;
      local_4c = local_4c & 0xffffff00;
      *puVar5 = local_4c;
      puVar5[1] = 0xffffffff;
      puVar5[2] = local_44;
      puVar5[3] = local_40;
    }
    *(ushort *)puVar4 = (ushort)*puVar4 + 1;
    iVar2 = data_iterator_next();
  } while( true );
}
#endif
