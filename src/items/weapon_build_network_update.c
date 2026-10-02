// weapon_build_network_update  (Ghidra: FUN_004c5f10; named from
// out/phase4/items_functions.md, "Sends an incremental or full network state update for an
// item, using a per-tag-type dispatch table and a sequence counter")
// address 0x4c5f10, size 346 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: exact structural analog of equipment_build_network_update.c (0x4bc0f0); types/items.h
//   weapon_data (network_state at 0x2e4, network_baseline_index 0x2e1, network_sequence 0x2e2,
//   magazines[].rounds_unloaded); types/objects.h object (position 0x05c, velocity 0x068, type
//   0x0b4), object_type_definition.network_delta_message_type (network_delta_message_type); object_try_and_get
//   mask _object_mask_weapon.
// register convention: matches equipment_build_network_update.c: item_index feeds
//   object_try_and_get's hidden index slot; the function's own four parameters are already a
//   plain __cdecl stack signature. arg2/arg3 are read nowhere in the body and are kept for
//   call-site compatibility only.
// UNSURE: same caveat as equipment_build_network_update.c about message_delta_encode_message's
// "changed_offset"/"items"/"type_offset" roles -- the full-snapshot fields (position, velocity,
// both magazines' rounds_unloaded, age) are kept byte-for-byte contiguous with the hash for that
// reason, but the claim is not verified against message_delta_encode_message's own body.
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_id_table *object_network_id_table; // 0x00687130
    // variable's value is the table root, and +0x0c off it is the hash_table this function hashes
    // an object datum_index through (objdump 0x4bbcb9: mov esi,ds:0x687130 / add esi,0xc)
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module.
    // Both arguments are register-passed and invisible to Ghidra; objdump resolves them --
    //   4bbcb9: mov esi,DWORD PTR ds:0x687130   ; ESI = table root
    //   4bbcbf: add esi,0xc                     ; ESI = root + 0x0c, the hash_table itself
    //   4bbcc2: call 0x4f05e0                   ; ECX = the datum_index being hashed
    // -- which matches the canonical declaration src/memory already uses for this symbol.
    // see
    // equipment_build_creation_message.c
extern int message_delta_encode_message(int flag, int message_type, int changed_offset,
    void **items, int type_offset, int count, char force_changed); // 0x4ec940
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// Builds and sends one network delta update for a weapon item: update_type == 1 sends a full
// baseline (live position/velocity plus both magazines' reserve and the network age), anything
// else sends a delta against the stored weapon_network_state. Advances the per-object sequence
// byte, wrapping 0xff back to 0, whenever the send reports a positive result.
int32_t weapon_build_network_update(uint32_t item_index, uint32_t unused_arg2,
    uint32_t unused_arg3, int32_t update_type)
{
    object *obj = object_try_and_get(item_index, _object_mask_weapon);
    int32_t result;

    if (obj == 0) {
        return 0;
    }

    {
        // Contiguous on purpose -- see file header UNSURE note.
        struct {
            int32_t item_hash;        // local_34
            uint8_t baseline_index;   // weapon_data.network_baseline_index
            uint8_t sequence;         // weapon_data.network_sequence
            uint8_t is_first_update;  // (update_type == 0); UNSURE, see file header
        } header;
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
        int32_t message_type = object_type_definitions[obj->type]->network_delta_message_type; // network_delta_message_type
        void *header_ptr = &header;
        int32_t is_full_snapshot = (update_type == 1);
        void *items_array[1];
        void *type_offset;
        struct {
            real position[3];
            real velocity[3];
            int16_t rounds_unloaded[2];
            real age;
        } snapshot; // only filled/used when is_full_snapshot; declared here so its address
                    // stays valid through the encode call below

        header.item_hash = 0;
        if (item_index != (uint32_t)k_datum_index_none) {
            header.item_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
            if (header.item_hash == -1) {
                header.item_hash = 0;
            }
        }
        header.sequence = wd->network_sequence;
        header.baseline_index = wd->network_baseline_index;
        header.is_first_update = (uint8_t)(update_type == 0);

        if (!is_full_snapshot) {
            items_array[0] = &wd->network_state;
            type_offset = 0;
        } else {
            void *net_ptr = &wd->network_state;

            snapshot.position[0] = obj->position.x;
            snapshot.position[1] = obj->position.y;
            snapshot.position[2] = obj->position.z;
            snapshot.velocity[0] = obj->velocity.i;
            snapshot.velocity[1] = obj->velocity.j;
            snapshot.velocity[2] = obj->velocity.k;
            snapshot.rounds_unloaded[0] = wd->magazines[0].rounds_unloaded;
            snapshot.rounds_unloaded[1] = wd->magazines[1].rounds_unloaded;
            snapshot.age = wd->age;

            items_array[0] = &snapshot;
            type_offset = &net_ptr;
        }

        result = message_delta_encode_message(is_full_snapshot, message_type,
            (int)&header_ptr, items_array, (int)type_offset, 1, 0);
    }

    if (0 < result) {
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
        uint8_t sequence = wd->network_sequence + 1;
        wd->network_sequence = sequence;
        if ((int8_t)sequence == -1) {
            wd->network_sequence = 0;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4c5f10):

int FUN_004c5f10(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  char cVar3;
  int **changed_offset;
  int **items;
  int **type_offset;
  int *local_40;
  int *local_3c;
  int *local_38;
  int local_34;
  undefined1 local_30;
  undefined1 local_2f;
  undefined1 local_2e;
  int local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined2 local_8;
  undefined2 local_6;
  undefined4 local_4;

  iVar1 = object_try_and_get(4);
  if (iVar1 == 0) {
    iVar2 = 0;
  }
  else {
    local_34 = 0;
    if (param_1 != -1) {
      local_34 = hash_table_get();
      if (local_34 == -1) {
        local_34 = 0;
      }
    }
    local_2f = *(undefined1 *)(iVar1 + 0x2e2);
    local_30 = *(undefined1 *)(iVar1 + 0x2e1);
    local_2e = param_4 == 0;
    if (param_4 != 1) {
      type_offset = (int **)0x0;
      local_3c = &local_34;
      items = &local_38;
      changed_offset = &local_3c;
      local_38 = (int *)(iVar1 + 0x2e4);
    }
    else {
      local_2c = *(int *)(iVar1 + 0x5c);
      local_28 = *(undefined4 *)(iVar1 + 0x60);
      local_24 = *(undefined4 *)(iVar1 + 100);
      local_20 = *(undefined4 *)(iVar1 + 0x68);
      local_1c = *(undefined4 *)(iVar1 + 0x6c);
      local_18 = *(undefined4 *)(iVar1 + 0x70);
      local_8 = *(undefined2 *)(iVar1 + 0x2b6);
      local_6 = *(undefined2 *)(iVar1 + 0x2c2);
      local_4 = *(undefined4 *)(iVar1 + 0x30c);
      local_3c = &local_2c;
      type_offset = &local_40;
      items = &local_3c;
      changed_offset = &local_38;
      local_40 = (int *)(iVar1 + 0x2e4);
      local_38 = &local_34;
    }
    iVar2 = message_delta_encode_message
                      ((uint)(param_4 == 1),
                       *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar1 + 0xb4)] + 0x10),
                       (int)changed_offset,items,(int)type_offset,1,'\0');
    if ((0 < iVar2) &&
       (cVar3 = *(char *)(iVar1 + 0x2e2) + '\x01', *(char *)(iVar1 + 0x2e2) = cVar3, cVar3 == -1)) {
      *(undefined1 *)(iVar1 + 0x2e2) = 0;
      return iVar2;
    }
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
