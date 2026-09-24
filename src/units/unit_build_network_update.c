// unit_build_network_update  (Ghidra: unit_build_network_update, renamed)
// address 0x55aed0, size 563 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: every local this function packs matches a documented object/unit_data/biped_data
//   field exactly by offset: object.definition_tag/name_index/owner_linkage/position/velocity/
//   forward/up (objects.h), unit_data.flags bit 0x80000 (0x204, types/units.h), biped_data
//   network_update_sequence/network_body_vitality/network_shield_vitality/
//   network_shield_stunned/network_grenade_counts (0x527/0x530/0x534/0x538/0x52c,
//   types/units.h). message_delta_encode_message's signature and calling style follow the
//   sibling file src/units/unit_broadcast_state_change_event.c (same module, 0x566c00), written
//   in this module's earlier session.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern int32_t hash_table_get(int32_t key);                                    // 0x4f05e0, UNSURE signature
extern int32_t message_delta_encode_message(uint32_t a, uint32_t size, uint32_t b, void *fields,
                                             uint32_t c, uint32_t d, uint8_t e); // 0x4ec940, UNSURE signature
extern int32_t FUN_004e9c20(uint32_t object_index); // UNSURE module: a fallback resolved index
extern void *memcpy(void *dst, const void *src, uint32_t n);

// Packs a unit's key simulation state (tag, name, resolved owner/animation-graph handles,
// transform, network flags, health/shield and grenade counts) into a flat record and submits it
// as network update message type 0x1d. Field layout preserved exactly from Ghidra's stack frame;
// see file header for the offsets each one is read from.
void unit_build_network_update(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    int32_t resolved_graph = 0;
    int32_t resolved_owner = 0;
    int32_t resolved_tag = 0;

    if (object_index != k_datum_index_none) {
        resolved_tag = hash_table_get(object_index);
    }
    if (obj->unknown_0c4 != (uint32_t)k_datum_index_none) {
        resolved_graph = hash_table_get(obj->unknown_0c4);
        if (resolved_graph == -1) resolved_graph = 0;
    }
    if (obj->owner_linkage != (uint32_t)k_datum_index_none) {
        resolved_owner = hash_table_get(obj->owner_linkage);
        if (resolved_owner == -1) resolved_owner = 0;
    }
    if (resolved_tag == -1) {
        resolved_tag = FUN_004e9c20(object_index); // UNSURE
    }

    {
        struct {
            datum_index definition_tag;
            int16_t name_index;
            real_vector3d forward;
            real_vector3d up;
            real_point3d position;
            real_vector3d velocity;
            uint8_t unknown_188[0x30]; // object+0x188, untouched by the objects module; packed wholesale
            uint8_t unit_flag_bit_0x80000;
            uint32_t unknown_344_bits;  // object 0x344, Ghidra puVar1[0xd1] (dword index)
            uint8_t network_update_sequence;
            uint32_t network_body_vitality_bits;
            uint32_t network_shield_vitality_bits;
            uint8_t network_shield_stunned;
            int16_t network_grenade_counts;
            int32_t resolved_tag;
            int32_t resolved_owner;
            int32_t resolved_graph;
        } fields;

        fields.definition_tag = obj->definition_tag;
        fields.name_index = obj->name_index;
        fields.forward = obj->forward;
        fields.up = obj->up;
        fields.position = obj->position;
        fields.velocity = obj->velocity;
        memcpy(fields.unknown_188, obj->unknown_188, sizeof(fields.unknown_188));
        fields.unit_flag_bit_0x80000 = (unit->flags >> 0x13) & 1;
        // 0xd1 * 4 = 0x344, i.e. unit_data.unknown_344 -- the earlier rewrite named 0x334
        fields.unknown_344_bits = *(uint32_t *)&unit->unknown_344;
        fields.network_update_sequence = biped->network_update_sequence;
        fields.network_body_vitality_bits = *(uint32_t *)&biped->network_body_vitality;
        fields.network_shield_vitality_bits = *(uint32_t *)&biped->network_shield_vitality;
        fields.network_shield_stunned = biped->network_shield_stunned;
        fields.network_grenade_counts = biped->network_grenade_counts;
        fields.resolved_tag = resolved_tag;
        fields.resolved_owner = resolved_owner;
        fields.resolved_graph = resolved_graph;

        message_delta_encode_message(0, 0x1d, 0, &fields, 0, 1, 0);
    }
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
