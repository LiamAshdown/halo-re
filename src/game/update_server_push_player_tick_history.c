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
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern int32_t update_server_tick;              // 0x006f1d8c
extern uint32_t update_server_history[32 * (0x308 / 4)]; // 0x006f1d94, raw dword view of
                                                              // update_server_history

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern void update_client_advance_read_cursor(int32_t target_tick, const uint32_t *record); // 0x4734b0, EBX tick, EDX record
extern data_array *update_server_queues; // 0x006f1d90

// UNSURE: see header.
// REWRITTEN (first-boot track, objdump 0x472cc0..0x472e9f): the queues iterated are update_server_queues
//   (0x006f1d90; the old version iterated nothing and dereferenced NULL), and update_client_advance_read_cursor
//   (EBX the tick, EDX the slot's count word) always runs at the end. Per tick: the ring slot (0x308 bytes, tick &
//   0x1f, NULL when the tick counter overflowed) gets the tick and a zero count; then for every queue: when its
//   ring has a record (read +0x38 != write +0x34; records at +0x30, capacity +0x28) the record's refcount (+4)
//   drops, and at zero the record is popped (read advances modulo the capacity); either way its 11 dwords are the
//   tick's input -- the queue's +0x44 (and +0x8) get the record's 8 input dwords (+0xc), +0x40 = 1 -- and the
//   slot gets the 8 dwords at +8 + 0x20 * n and a summary {1, record +4 == 0, ?, ?; record +0, record +8,
//   record +4} at +0x208 + 0x10 * n. A queue without a record copies its own +8 input and summary {0,...; -1}.
//   The summary bytes/dwords the original leaves as stack garbage (bytes 2..3, and the last two dwords of an
//   empty queue's summary) are written as 0 here.
// blam-cc: none
void update_server_push_player_tick_history(void)
{
    int32_t tick = update_server_tick;
    uint8_t *slot;
    uint16_t *count;
    data_iterator iterator;
    uint8_t *queue;

    update_server_tick = tick + 1;
    slot = (tick < tick + 1 && tick >= (tick + 1) - 0x20)
        ? (uint8_t *)update_server_history + (tick & 0x1f) * 0x308 : 0;
    *(int32_t *)slot = tick;
    count = (uint16_t *)(slot + 4);
    *count = 0;

    iterator.data = update_server_queues;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)update_server_queues ^ 0x69746572; // 'iter'
    for (queue = (uint8_t *)data_iterator_next(&iterator); queue != 0; queue = (uint8_t *)data_iterator_next(&iterator)) {
        int32_t read = *(int32_t *)(queue + 0x38);
        uint32_t *record = 0;
        uint8_t have = 0;
        uint32_t *summary = (uint32_t *)(slot + 0x208 + *count * 0x10);
        int32_t i;

        if (read != *(int32_t *)(queue + 0x34)) {
            record = ((uint32_t **)*(uint32_t *)(queue + 0x30))[read];
            have = 1;
            record[1] -= 1;
            if (record[1] == 0) {
                if (read != *(int32_t *)(queue + 0x34)) {
                    record = ((uint32_t **)*(uint32_t *)(queue + 0x30))[read];
                    *(int32_t *)(queue + 0x38) = (read + 1) % *(int32_t *)(queue + 0x28);
                } else {
                    record = 0;
                    have = 0;
                }
            }
            if (have) {
                uint32_t local_record[11];

                for (i = 0; i < 11; i++) {
                    local_record[i] = record[i];
                }
                for (i = 0; i < 8; i++) {
                    ((uint32_t *)(queue + 0x44))[i] = local_record[3 + i];
                }
                queue[0x40] = 1;
                for (i = 0; i < 8; i++) {
                    ((uint32_t *)(queue + 8))[i] = local_record[3 + i];
                    ((uint32_t *)(slot + 8 + *count * 0x20))[i] = local_record[3 + i];
                }
                summary[0] = 1u | (uint32_t)(local_record[1] == 0) << 8;
                summary[1] = local_record[0];
                summary[2] = local_record[2];
                summary[3] = local_record[1];
                *count += 1;
                continue;
            }
        }
        for (i = 0; i < 8; i++) {
            ((uint32_t *)(slot + 8 + *count * 0x20))[i] = ((uint32_t *)(queue + 8))[i];
        }
        summary[0] = 0;
        summary[1] = 0xffffffff;
        summary[2] = 0;
        summary[3] = 0;
        *count += 1;
    }
    update_client_advance_read_cursor(tick, (const uint32_t *)count);
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
