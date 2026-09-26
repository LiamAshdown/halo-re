// network_event_feed_flush  (Ghidra: FUN_004e8040; named per this rewrite)
// address 0x4e8040, size 405 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("Resolves queued unit-index event records to
// network indices and sends them as a batched message-0x26 update, then clears the queue.");
// network_event_feed_queue_append.c (this batch, 0x4e7ff0, the same queue layout: count at +4,
// keys at +8 stride 8, payloads at +0x88 stride 0x30); types/objects.h hash_table (the manual
// bucket walk here matches hash_table_get's own bucket-chain shape exactly, just inlined).
// register convention: none beyond the one recognized stack parameter (the queue pointer).
// UNSURE: the manual hash-table walk assumes PTR_DAT_00687558 is the same wrapper as
// remote_player_index_remap_table (hash_table embedded at +0xc), matching this batch's other
// hash_table_get call sites against that global; not independently re-derived here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"

extern data_array *player_data; // 0x0087a480
extern void *remote_player_index_remap_table; // 0x00687558

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server
    // UNSURE: signature inferred from this call site only

// Resolves every queued event record's unit index (queue+8, stride 8) to its object_data slot's
// player-count and player-table entry, keeping only records whose slot resolves to a live entry
// matching (or wildcard-matching) the record's salt. For each surviving record, looks up its raw
// key's network hash through the same hash table this batch's other builders use, replacing the
// key in place. Encodes all surviving records as one batched message-0x26 update and sends it,
// then clears the queue's count.
void network_event_feed_flush(int32_t *queue)
{
    int32_t remaining;
    int32_t *key_slot;
    int32_t *payload_slot;
    void *survivors_key[16];
    void *survivors_payload[16];
    void *survivors_extra[16];
    int32_t survivor_count;
    int32_t i;
    int32_t raw_key;
    int32_t hash;
    hash_table *table;
    hash_node *node;
    char force_changed;
    void **type_offset_arg;
    int16_t entry_identifier;
    int16_t entry_salt;
    uint8_t *entry;

    for (i = 0; i < 16; i = i + 1) {
        survivors_key[i] = 0;
    }
    for (i = 0; i < 16; i = i + 1) {
        survivors_payload[i] = 0;
    }

    remaining = queue[1];
    survivor_count = 0;
    if (0 < remaining) {
        payload_slot = queue + 0x22;
        key_slot = queue;
        do {
            key_slot = key_slot + 2;
            raw_key = *key_slot;
            if (raw_key != -1 && (int16_t)raw_key >= 0 &&
                (int16_t)raw_key < player_data->maximum_count) {
                entry = (uint8_t *)player_data->data + (int16_t)raw_key * player_data->size;
                entry_identifier = *(int16_t *)entry;
                if (entry_identifier != 0) {
                    entry_salt = (int16_t)((uint32_t)raw_key >> 16);
                    if (entry_salt == 0 || entry_identifier == entry_salt) {
                        survivors_key[survivor_count] = key_slot;
                        survivors_payload[survivor_count] = payload_slot;
                        survivors_extra[survivor_count] = entry + 0x130;
                        survivor_count = survivor_count + 1;
                    }
                }
            }
            payload_slot = payload_slot + 0xc;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    remaining = 0;
    if (0 < survivor_count) {
        table = (hash_table *)((uint8_t *)remote_player_index_remap_table + 0x0c);
        do {
            raw_key = *(int32_t *)survivors_key[remaining];
            hash = 0;
            if (raw_key != -1) {
                hash = -1;
                if (table->initialized == 1) {
                    int32_t bucket = (raw_key < 0 ? -raw_key : raw_key) % table->bucket_count;
                    for (node = table->buckets[bucket].first; node != 0; node = node->next) {
                        if (node->key == raw_key) {
                            hash = node->value;
                            break;
                        }
                    }
                }
                if (hash == -1) {
                    hash = 0;
                }
            }
            *(int32_t *)survivors_key[remaining] = hash;
            remaining = remaining + 1;
        } while (remaining < survivor_count);
    }

    force_changed = (char)*queue != 1;
    type_offset_arg = force_changed ? survivors_extra : 0;
    message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, (uint32_t)force_changed, 0x26, (int32_t)survivors_key,
        survivors_payload, (int32_t)type_offset_arg, survivor_count, force_changed);
    network_session_broadcast_to_flagged(network_server_pointer, 1, 0, (char)*queue, 0, 0, 2);
    queue[1] = 0;
}

#if 0
Original Ghidra decompilation (0x4e8040), from tools/pack.py 0x4e8040:

void FUN_004e8040(int *param_1)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  short sVar7;
  int *piVar8;
  int *piVar9;
  void **ppvVar10;
  bool force_changed;
  undefined4 *puVar11;
  int local_c4;
  undefined4 local_c0 [16];
  void *local_80 [16];
  undefined4 local_40 [16];

  puVar11 = local_c0;
  for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  ppvVar10 = local_80;
  for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
    *ppvVar10 = (void *)0x0;
    ppvVar10 = ppvVar10 + 1;
  }
  iVar6 = param_1[1];
  local_c4 = 0;
  if (0 < iVar6) {
    piVar8 = param_1 + 0x22;
    piVar9 = param_1;
    do {
      piVar9 = piVar9 + 2;
      iVar1 = *piVar9;
      if (((iVar1 != -1) && (sVar5 = (short)iVar1, -1 < sVar5)) &&
         (sVar5 < *(short *)(DAT_0087a480 + 0x20))) {
        psVar2 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar5 +
                          *(int *)(DAT_0087a480 + 0x34));
        sVar5 = *psVar2;
        if ((sVar5 != 0) && ((sVar7 = (short)((uint)iVar1 >> 0x10), sVar7 == 0 || (sVar5 == sVar7)))
           ) {
          local_c0[local_c4] = piVar9;
          local_80[local_c4] = piVar8;
          local_40[local_c4] = psVar2 + 0x98;
          local_c4 = local_c4 + 1;
        }
      }
      piVar8 = piVar8 + 0xc;
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
  }
  iVar6 = 0;
  if (0 < local_c4) {
    do {
      iVar1 = *(int *)local_c0[iVar6];
      iVar3 = 0;
      if (iVar1 != -1) {
        iVar3 = -1;
        if (PTR_DAT_00687558[0xc] == '\x01') {
          iVar4 = iVar1;
          if (iVar1 < 0) {
            iVar4 = -iVar1;
          }
          for (piVar8 = *(int **)(*(int *)(PTR_DAT_00687558 + 0x14) + 4 +
                                 (iVar4 % *(int *)(PTR_DAT_00687558 + 0x10)) * 8);
              piVar8 != (int *)0x0; piVar8 = (int *)piVar8[2]) {
            if (*piVar8 == iVar1) {
              iVar3 = piVar8[1];
              break;
            }
          }
        }
        if (iVar3 == -1) {
          iVar3 = 0;
        }
      }
      *(int *)local_c0[iVar6] = iVar3;
      iVar6 = iVar6 + 1;
    } while (iVar6 < local_c4);
  }
  force_changed = (char)*param_1 != '\x01';
  if (force_changed) {
    puVar11 = local_40;
  }
  else {
    puVar11 = (undefined4 *)0x0;
  }
  message_delta_encode_message
            ((uint)force_changed,0x26,(int)local_c0,local_80,(int)puVar11,local_c4,force_changed);
  FUN_004e1a80(1,&DAT_00871de0,(char)*param_1,0,0,2);
  param_1[1] = 0;
  return;
}
#endif
