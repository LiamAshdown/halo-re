// weapon_build_creation_message  (Ghidra: FUN_004c5a50; named per types/items.h
// weapon_creation_message comment block: "Built by weapon_build_creation_message (0x4c5a50)")
// address 0x4c5a50, size 433 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/items.h weapon_creation_message (every field, confirmed field-by-field
//   against the byte offsets below), weapon_network_state (position/velocity/age),
//   weapon_data.magazines[].rounds_loaded; types/objects.h object.definition_tag/name_index/
//   owner_linkage/unknown_0c4/forward/up; global hash_table at object_network_id_table+0x0c.
// register convention: item index in the first Ghidra-recognized parameter; the last
// (object_flags) is the fourth. The middle two parameters are never read in the decompiled
// body and are preserved only for call-site compatibility.
// blam-cc: stack -> (item_index, unused_param_2, unused_param_3, object_flags)
// UNSURE: matches equipment_build_creation_message.c's precedent -- hash_table_get and
// network_index_cache_find_or_allocate_slot are opaque externals decompiled with implicit register arguments Ghidra could
// not recover; called exactly as shown, with only the visibly-passed arguments preserved.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern network_id_table *object_network_id_table; // 0x00687130
    // variable's value is the table root, and +0x0c off it is the hash_table this function hashes
    // an object datum_index through (objdump 0x4bbcb9: mov esi,ds:0x687130 / add esi,0xc)
extern network_id_table *machine_table; // 0x00687558
    // handle is hashed through; src/units spells it the same way. UNSURE name in both places.
extern data_array *object_data;     // 0x008603b0
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module.
    // Both arguments are register-passed and invisible to Ghidra; objdump resolves them --
    //   4bbcb9: mov esi,DWORD PTR ds:0x687130   ; ESI = table root
    //   4bbcbf: add esi,0xc                     ; ESI = root + 0x0c, the hash_table itself
    //   4bbcc2: call 0x4f05e0                   ; ECX = the datum_index being hashed
    // -- which matches the canonical declaration src/memory already uses for this symbol.
extern uint8_t network_object_index_cache[]; // 0x006870d8
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key); // 0x4e9c20, EAX container, stack key
    // fallback hash-table insert, see file header
extern int message_delta_encode_message(int flag, int message_type, int changed_offset,
    void **items, int type_offset, int count, char force_changed); // 0x4ec940

// Builds and encodes a weapon creation network message (type 0x20).
void weapon_build_creation_message(datum_index item_index, uint32_t unused_param_2,
    uint32_t unused_param_3, uint32_t object_flags)
{
    object *item_obj;
    weapon_data *wd;
    weapon_creation_message message;
    void *items[2];
    int32_t object_hash = 0;
    int32_t owner_hash = 0;
    int32_t parent_hash = 0;

    (void)unused_param_2;
    (void)unused_param_3;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    if (item_index != (datum_index)0xffffffff) {
        object_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
    }
    if (item_obj->creator_object != (uint32_t)0xffffffff) {
        parent_hash = hash_table_get(&object_network_id_table->id_to_index, item_obj->creator_object);
        if (parent_hash == -1) parent_hash = 0;
    }
    if (item_obj->owner_linkage != (uint32_t)0xffffffff) {
        owner_hash = hash_table_get(&machine_table->id_to_index, item_obj->owner_linkage);
        if (owner_hash == -1) owner_hash = 0;
    }
    if (object_hash == -1) {
        object_hash = network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)item_index); // FIXED: EAX = 0x6870d8 (0x4c5adb), stack = item_index
    }

    message.definition_tag = item_obj->definition_tag;
    message.object_hash = object_hash;
    message.name_index = item_obj->owner_team;
    message.owner_hash = owner_hash;
    message.parent_hash = parent_hash;
    message.object_flags = object_flags;
    message.position = wd->network_state.position;
    message.forward = item_obj->forward;
    message.up = item_obj->up;
    message.velocity = wd->network_state.velocity;
    message.baseline_index = wd->network_baseline_index;
    message.rounds_unloaded[0] = *(int16_t *)((uint8_t *)wd + 0x2e4 + 0x24);       // weapon_network_state.rounds_unloaded[0]
    message.rounds_unloaded[1] = *(int16_t *)((uint8_t *)wd + 0x2e4 + 0x26);       // weapon_network_state.rounds_unloaded[1]
    message.age = *(float *)((uint8_t *)wd + 0x2e4 + 0x28);                       // weapon_network_state.age
    message.rounds_loaded[0] = wd->magazines[0].rounds_loaded;
    message.rounds_loaded[1] = wd->magazines[1].rounds_loaded;

    // The 4th argument is an array of record pointers, not the record itself: the
    // original writes `items[0] = &message; items[1] = 0;` and then passes `items`.
    items[0] = &message;
    items[1] = 0;
    message_delta_encode_message(0, k_message_weapon_creation, 0, items, 0, 1, 0);
}

#if 0
Original Ghidra decompilation (0x4c5a50):

void FUN_004c5a50(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

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
  undefined1 local_10;
  undefined2 local_e;
  undefined2 local_c;
  undefined4 local_8;
  undefined2 local_4;
  undefined2 local_2;

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
  local_10 = *(undefined1 *)((int)puVar1 + 0x2e1);
  local_40 = puVar1[0xb9];
  local_3c = puVar1[0xba];
  local_38 = puVar1[0xbb];
  local_1c = puVar1[0xbc];
  local_18 = puVar1[0xbd];
  local_14 = puVar1[0xbe];
  local_e = *(undefined2 *)(puVar1 + 0xc2);
  local_c = *(undefined2 *)((int)puVar1 + 0x30a);
  local_8 = puVar1[0xc3];
  local_4 = *(undefined2 *)(puVar1 + 0xae);
  local_2 = *(undefined2 *)(puVar1 + 0xb1);
  param_1 = &local_58;
  param_4 = 0;
  local_54 = iVar3;
  local_48 = iVar2;
  message_delta_encode_message(0,0x20,0,&param_1,0,1,'\0');
  return;
}
#endif
