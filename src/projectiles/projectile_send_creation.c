// projectile_send_creation  (Ghidra: FUN_004c0b10; renamed per
// out/phase4/projectiles_types_notes.md: "message 0x1e")
// address 0x4c0b10, size 387 bytes
// name confidence: 0.7   rewrite confidence: 0.85 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: exact structural analog of src/items/equipment_build_creation_message.c (0x4bbc90);
//   types/projectiles.h projectile_creation_message (every field below matches its layout
//   exactly -- this function is the struct's own derivation) and projectile_data.network_state
//   (0x27c), .network_baseline_index (0x27a); types/objects.h object (definition_tag 0x000,
//   name_index 0x0b8, owner_linkage 0x0c0, unknown_0c4 0x0c4, forward 0x074, up 0x080,
//   angular_velocity 0x08c).
// register convention: none -- Ghidra recovered a single stack parameter, the projectile index.
// blam-cc: stack -> projectile_index
// UNSURE: hash_table_get and network_index_cache_find_or_allocate_slot are opaque externals (objects/networking modules,
//   outside this batch's address range) that Ghidra decompiled with implicit register
//   arguments it could not recover; called exactly as the equipment sibling function
//   establishes.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130, +0x0c off it is the hash_table this
    // function hashes the projectile's own datum_index through
extern uint8_t *network_message_table_b; // 0x00687558, the parallel table object.owner_linkage
    // is hashed through
extern data_array *object_data; // 0x008603b0

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern int32_t network_index_cache_find_or_allocate_slot(uint32_t key); // 0x4e9c20, networking module; opaque fallback
    // hash-table insert, see src/items/equipment_build_creation_message.c
extern int message_delta_encode_message(int flag, int message_type, int changed_offset,
    void **items, int type_offset, int count, char force_changed); // 0x4ec940

int32_t projectile_send_creation(uint32_t projectile_index)
{
    object *obj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
    int32_t projectile_hash = 0;
    int32_t creating_object_hash = 0;
    int32_t owner_hash = 0;
    projectile_creation_message message;
    void *message_ptr;

    if (projectile_index != 0xffffffff) {
        projectile_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), projectile_index);
    }
    if (obj->creator_object != 0xffffffff) {
        creating_object_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), obj->creator_object);
        if (creating_object_hash == -1) {
            creating_object_hash = 0;
        }
    }
    if (obj->owner_linkage != 0xffffffff) {
        owner_hash = hash_table_get((hash_table *)(network_message_table_b + 0x0c), obj->owner_linkage);
        if (owner_hash == -1) {
            owner_hash = 0;
        }
    }
    if (projectile_hash == -1) {
        projectile_hash = network_index_cache_find_or_allocate_slot(projectile_index); // UNSURE: see file header
    }

    message.definition_tag = obj->definition_tag;
    message.object_hash = projectile_hash;
    message.owner_team = obj->owner_team;
    // message.pad_0a left unset, matching Ghidra: the original never writes this byte pair.
    message.owner_hash = owner_hash;
    message.creating_object_hash = creating_object_hash;
    message.forward = obj->forward;
    message.up = obj->up;
    message.angular_velocity = obj->angular_velocity;
    message.baseline_index = *((uint8_t *)obj + 0x27a); // projectile_data.network_baseline_index
    {
        projectile_network_state *net = (projectile_network_state *)((uint8_t *)obj + 0x27c);
        message.position = net->position;
        message.velocity = net->velocity;
    }

    message_ptr = &message;
    return message_delta_encode_message(0, k_message_projectile_creation, 0, &message_ptr, 0, 1, 0);  // the original returns this call's result (EAX) unchanged
}

#if 0
Original Ghidra decompilation (0x4c0b10):

void FUN_004c0b10(undefined4 *param_1)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 local_54;
  int local_50;
  undefined2 local_4c;
  int local_48;
  int local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined1 local_4;

  iVar3 = 0;
  puVar1 = *(undefined4 **)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)param_1 & 0xffff) * 0xc);
  if (param_1 != (void *)0xffffffff) {
    iVar3 = hash_table_get();
  }
  iVar2 = 0;
  if (puVar1[0x31] != -1) {
    iVar2 = hash_table_get();
    if (iVar2 == -1) {
      iVar2 = 0;
    }
  }
  local_48 = 0;
  if (puVar1[0x30] != -1) {
    local_48 = hash_table_get();
    if (local_48 == -1) {
      local_48 = 0;
    }
  }
  if (iVar3 == -1) {
    iVar3 = FUN_004e9c20(param_1);
  }
  local_54 = *puVar1;
  local_4c = *(undefined2 *)(puVar1 + 0x2e);
  local_34 = puVar1[0x1d];
  local_30 = puVar1[0x1e];
  local_2c = puVar1[0x1f];
  local_28 = puVar1[0x20];
  local_24 = puVar1[0x21];
  local_20 = puVar1[0x22];
  local_10 = puVar1[0x23];
  local_c = puVar1[0x24];
  local_8 = puVar1[0x25];
  local_4 = *(undefined1 *)((int)puVar1 + 0x27a);
  local_40 = puVar1[0x9f];
  local_3c = puVar1[0xa0];
  local_38 = puVar1[0xa1];
  local_1c = puVar1[0xa2];
  local_18 = puVar1[0xa3];
  local_14 = puVar1[0xa4];
  param_1 = &local_54;
  local_50 = iVar3;
  local_44 = iVar2;
  message_delta_encode_message(0,0x1e,0,&param_1,0,1,'\0');
  return;
}
#endif
