// equipment_build_creation_message  (Ghidra: FUN_004bbc90; renamed per
// out/phase4/items_types_notes.md, which reads this function field-by-field to derive
// equipment_creation_message)
// address 0x4bbc90, size 398 bytes
// name confidence: 0.6 (items_types_notes.md: "equipment_build_creation_message (0x4bbc90)
//   reads it field by field")   rewrite confidence: 0.55
// evidence: types/items.h equipment_creation_message (every field below matches its layout
//   exactly -- this function is the struct's own derivation); types/items.h equipment_data
//   (network_state at 0x248, network_baseline_index at 0x245); types/objects.h object
//   (definition_tag 0x000, forward 0x074, up 0x080, name_index 0x0b8, owner_linkage 0x0c0,
//   unknown_0c4 0x0c4); global 0x008603b0 object_data.
// register convention: none -- Ghidra recovered a plain __cdecl signature with four stack
//   parameters. param_2 and param_3 are read nowhere in the body and are kept as unused,
//   unnamed-purpose parameters for call-site compatibility.
// UNSURE: hash_table_get and network_index_cache_find_or_allocate_slot are opaque externals (objects/networking modules,
//   outside this batch's address range) that Ghidra decompiled with implicit register
//   arguments it could not recover (unaff_ESI, in_EAX); they are called exactly as Ghidra
//   shows, with only the visibly-passed arguments preserved, per the same convention used for
//   0x4ee5e0's callees.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130, not owned by this module; the
    // variable's value is the table root, and +0x0c off it is the hash_table this function hashes
    // an object datum_index through (objdump 0x4bbcb9: mov esi,ds:0x687130 / add esi,0xc)
extern uint8_t *network_message_table_b;   // 0x00687558, the parallel table the owner_linkage
    // handle is hashed through; src/units spells it the same way. UNSURE name in both places.
extern data_array *object_data; // 0x008603b0

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module.
    // Both arguments are register-passed and invisible to Ghidra; objdump resolves them --
    //   4bbcb9: mov esi,DWORD PTR ds:0x687130   ; ESI = table root
    //   4bbcbf: add esi,0xc                     ; ESI = root + 0x0c, the hash_table itself
    //   4bbcc2: call 0x4f05e0                   ; ECX = the datum_index being hashed
    // -- which matches the canonical declaration src/memory already uses for this symbol.
extern int32_t network_index_cache_find_or_allocate_slot(uint32_t key); // 0x4e9c20, networking module; opaque fallback
    // hash-table insert, see file header
extern int message_delta_encode_message(int flag, int message_type, int changed_offset,
    void **items, int type_offset, int count, char force_changed); // 0x4ec940

void equipment_build_creation_message(uint32_t item_index, uint32_t unused_arg2,
    uint32_t unused_arg3, uint32_t object_flags)
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    int32_t item_hash = 0;
    int32_t parent_hash = 0;
    int32_t owner_hash = 0;
    equipment_creation_message message;
    void *item_ptr;

    if (item_index != 0xffffffff) {
        item_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), item_index);
    }
    if (obj->creator_object != 0xffffffff) {
        parent_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), obj->creator_object);
        if (parent_hash == -1) {
            parent_hash = 0;
        }
    }
    if (obj->owner_linkage != 0xffffffff) {
        owner_hash = hash_table_get((hash_table *)(network_message_table_b + 0x0c), obj->owner_linkage);
        if (owner_hash == -1) {
            owner_hash = 0;
        }
    }
    if (item_hash == -1) {
        item_hash = network_index_cache_find_or_allocate_slot(item_index); // UNSURE: see file header
    }

    message.definition_tag = obj->definition_tag;
    message.object_hash = item_hash;
    message.name_index = obj->owner_team;
    // message.pad_0a left unset, matching Ghidra: the original never writes this byte pair.
    message.owner_hash = owner_hash;
    message.parent_hash = parent_hash;
    message.object_flags = object_flags;
    message.forward = obj->forward;
    message.up = obj->up;
    message.baseline_index = *((uint8_t *)obj + 0x245); // equipment_data.network_baseline_index
    {
        equipment_network_state *net = (equipment_network_state *)((uint8_t *)obj + 0x248);
        message.position = net->position;
        message.velocity = net->velocity;
        message.angular_velocity = net->angular_velocity;
    }

    item_ptr = &message;
    message_delta_encode_message(0, k_message_equipment_creation, 0, &item_ptr, 0, 1, 0);
}

#if 0
Original Ghidra decompilation (0x4bbc90):

void FUN_004bbc90(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 local_58;
  int local_54;
  undefined2 local_50;
  int local_4c;
  int local_48;
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
  local_4c = 0;
  if (puVar1[0x30] != -1) {
    local_4c = hash_table_get();
    if (local_4c == -1) {
      local_4c = 0;
    }
  }
  if (iVar3 == -1) {
    iVar3 = FUN_004e9c20(param_1);
  }
  local_58 = *puVar1;
  local_50 = *(undefined2 *)(puVar1 + 0x2e);
  local_44 = param_4;
  local_34 = puVar1[0x1d];
  local_30 = puVar1[0x1e];
  local_2c = puVar1[0x1f];
  local_28 = puVar1[0x20];
  local_24 = puVar1[0x21];
  local_20 = puVar1[0x22];
  local_4 = *(undefined1 *)((int)puVar1 + 0x245);
  local_40 = puVar1[0x92];
  local_3c = puVar1[0x93];
  local_38 = puVar1[0x94];
  local_1c = puVar1[0x95];
  local_18 = puVar1[0x96];
  local_14 = puVar1[0x97];
  local_10 = puVar1[0x98];
  local_c = puVar1[0x99];
  local_8 = puVar1[0x9a];
  param_1 = &local_58;
  param_4 = 0;
  local_54 = iVar3;
  local_48 = iVar2;
  message_delta_encode_message(0,0x1f,0,&param_1,0,1,'\0');
  return;
}
#endif
