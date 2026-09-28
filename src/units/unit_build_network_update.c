// unit_build_network_update  (Ghidra: unit_build_network_update, renamed)
// address 0x55aed0, size 563 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: every local this function packs matches a documented object/unit_data/biped_data
//   field exactly by offset: object.definition_tag/name_index/owner_linkage/position/velocity/
//   forward/up (objects.h), unit_data.flags bit 0x80000 (0x204, types/units.h), biped_data
//   network_update_sequence/network_body_vitality/network_shield_vitality/
//   network_shield_stunned/network_grenade_counts (0x527/0x530/0x534/0x538/0x52c,
//   types/units.h). message_delta_encode_message's signature and calling style follow the
//   sibling file src/units/unit_broadcast_state_change_event.c (same module, 0x566c00), written
//   in this module's earlier session.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly 0x55aed0..0x55b103: the biped creation encoder
// (biped type +0x64 hook; network action 0x1d), the counterpart of vehicle_encode_network_create 0x571f20 and the
// exact inverse of unit_network_create_update_apply 0x55b110. Arguments (object index, buffer, bit budget); the 0x8c
// byte record -- definition, network key (a new index-cache slot 0x6870d8 when unknown), owner team, machine key
// (0x687558 table), creator key (0 when unknown), position +0x5c, forward +0x74, up +0x80, velocity +0x68, the
// 0x30 bytes at +0x188, unit flag 0x80000, +0x344, update sequence +0x527, grenade counts +0x52c (at record +0x7d),
// body / shield vitality +0x530 / +0x534, stunned +0x538 -- is encoded with message_delta_encode_message (EAX buffer,
// EDX budget; type 0x1d, one item) and its bit count returned. The previous C packed an invented layout and passed
// the encoder neither buffer nor budget.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;                    // 0x008603b0
extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t *machine_table;                     // 0x00687558, hash_table at +0x0c
extern uint8_t network_object_index_cache[];       // 0x006870d8
extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, ESI table, ECX key
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key); // 0x4e9c20, EAX container
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX

typedef struct biped_network_create_record {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    uint8_t position[12];          // 0x14 <- +0x5c
    uint8_t forward[12];           // 0x20 <- +0x74
    uint8_t up[12];                // 0x2c <- +0x80
    uint8_t velocity[12];          // 0x38 <- +0x68
    uint8_t block_188[0x30];       // 0x44 <- +0x188
    uint8_t flag_80000;            // 0x74
    uint8_t pad_75[3];
    uint32_t scalar_344;           // 0x78
    uint8_t update_sequence;       // 0x7c
    uint8_t grenade_counts[2];     // 0x7d
    uint8_t pad_7f;
    uint32_t body_vitality;        // 0x80
    uint32_t shield_vitality;      // 0x84
    uint8_t shield_stunned;        // 0x88
    uint8_t pad_89[3];
    uint32_t zero_8c;              // 0x8c, cleared before the encode (0x55b0e5)
} biped_network_create_record;

int32_t unit_build_network_update(uint32_t object_index, int32_t buffer, int32_t bit_budget)
{
    uint8_t *biped = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    hash_table *keys = &object_network_id_table->id_to_index;
    biped_network_create_record record;
    void *item = &record;
    int32_t key = 0;
    int32_t creator = 0;
    int32_t machine = 0;

    if (object_index != 0xffffffff) {
        key = hash_table_get(keys, (int32_t)object_index);
    }
    if (*(int32_t *)&((unit_object *)biped)->base.creator_object != -1) {
        creator = hash_table_get(keys, *(int32_t *)&((unit_object *)biped)->base.creator_object);
        if (creator == -1) {
            creator = 0;
        }
    }
    if (*(int32_t *)&((unit_object *)biped)->base.owner_linkage != -1) {
        machine = hash_table_get((hash_table *)(machine_table + 0xc), *(int32_t *)&((unit_object *)biped)->base.owner_linkage);
        if (machine == -1) {
            machine = 0;
        }
    }
    if (key == -1) {
        key = network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)object_index);
    }
    record.definition = *(datum_index *)biped;
    record.network_key = key;
    record.owner_team = ((unit_object *)biped)->base.owner_team;
    record.machine_key = machine;
    record.creator_key = creator;
    memcpy(record.forward, biped + 0x74, 12);
    memcpy(record.up, biped + 0x80, 12);
    memcpy(record.position, biped + 0x5c, 12);
    memcpy(record.velocity, biped + 0x68, 12);
    memcpy(record.block_188, biped + 0x188, 0x30);
    record.flag_80000 = (uint8_t)((((unit_object *)biped)->unit.flags >> 0x13) & 1);
    record.scalar_344 = *(uint32_t *)(biped + 0x344);
    record.update_sequence = biped[0x527];
    record.body_vitality = *(uint32_t *)&((biped_object *)biped)->biped.network_body_vitality;
    record.shield_vitality = *(uint32_t *)&((biped_object *)biped)->biped.network_shield_vitality;
    record.shield_stunned = biped[0x538];
    memcpy(record.grenade_counts, biped + 0x52c, 2);
    record.zero_8c = 0;
    return message_delta_encode_message(buffer, bit_budget, 0, 0x1d, 0, &item, 0, 1, 0);
}

#if 0
Original Ghidra decompilation (0x55aed0):

void FUN_0055aed0(uint param_1)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *local_94;
  undefined4 local_90;
  int local_8c;
  undefined2 local_88;
  int local_84;
  int local_80;
  undefined4 local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  byte local_1c;
  undefined4 local_18;
  undefined1 local_14;
  undefined2 local_13;
  undefined4 local_10;
  undefined4 local_c;
  undefined1 local_8;
  undefined4 local_4;

  iVar3 = 0;
  puVar1 = *(undefined4 **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (param_1 != 0xffffffff) {
    iVar3 = hash_table_get();
  }
  iVar2 = 0;
  if (puVar1[0x31] != -1) {
    iVar2 = hash_table_get();
    if (iVar2 == -1) {
      iVar2 = 0;
    }
  }
  local_84 = 0;
  if (puVar1[0x30] != -1) {
    local_84 = hash_table_get();
    if (local_84 == -1) {
      local_84 = 0;
    }
  }
  if (iVar3 == -1) {
    iVar3 = FUN_004e9c20(param_1);
  }
  local_90 = *puVar1;
  local_88 = *(undefined2 *)(puVar1 + 0x2e);
  local_70 = puVar1[0x1d];
  local_6c = puVar1[0x1e];
  local_68 = puVar1[0x1f];
  local_64 = puVar1[0x20];
  local_60 = puVar1[0x21];
  local_5c = puVar1[0x22];
  local_7c = puVar1[0x17];
  local_78 = puVar1[0x18];
  local_74 = puVar1[0x19];
  local_58 = puVar1[0x1a];
  local_54 = puVar1[0x1b];
  local_50 = puVar1[0x1c];
  local_4c = puVar1[0x62];
  local_48 = puVar1[99];
  local_44 = puVar1[100];
  local_40 = puVar1[0x65];
  local_3c = puVar1[0x66];
  local_38 = puVar1[0x67];
  local_34 = puVar1[0x68];
  local_30 = puVar1[0x69];
  local_2c = puVar1[0x6a];
  local_28 = puVar1[0x6b];
  local_24 = puVar1[0x6c];
  local_20 = puVar1[0x6d];
  local_1c = (byte)((uint)puVar1[0x81] >> 0x13) & 1;
  local_18 = puVar1[0xd1];
  local_14 = *(undefined1 *)((int)puVar1 + 0x527);
  local_10 = puVar1[0x14c];
  local_c = puVar1[0x14d];
  local_8 = *(undefined1 *)(puVar1 + 0x14e);
  local_13 = *(undefined2 *)(puVar1 + 0x14b);
  local_94 = &local_90;
  local_4 = 0;
  local_8c = iVar3;
  local_80 = iVar2;
  message_delta_encode_message(0,0x1d,0,&local_94,0,1,'\0');
  return;
}
#endif
