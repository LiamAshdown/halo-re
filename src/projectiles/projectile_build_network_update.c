// projectile_build_network_update  (Ghidra: FUN_004c0f30; renamed per
// out/phase4/projectiles_types_notes.md: "projectile row +0x6c; object_try_and_get(0x20),
// type-row +0x10 as the message index")
// address 0x4c0f30, size 312 bytes
// name confidence: 0.7   rewrite confidence: 0.8 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: exact structural analog of src/items/weapon_build_network_update.c (0x4c5f10), minus
//   the magazine/age fields weapon_data carries and projectile_data does not; types/projectiles.h
//   projectile_data.network_state (0x27c), .network_baseline_index (0x27a), .network_sequence
//   (0x27b); types/objects.h object.position (0x05c), .velocity (0x068), .type (0x0b4),
//   object_type_definition.network_delta_message_type (network_delta_message_type, matches
//   k_projectile_network_delta_index); object_try_and_get mask _object_mask_projectile.
// register convention: matches weapon_build_network_update.c: the projectile index feeds
//   object_try_and_get's hidden index slot; the function's own four parameters are already a
//   plain __cdecl stack signature. arg2/arg3 are read nowhere in the body and are kept for
//   call-site compatibility only.
// UNSURE: same caveat as weapon_build_network_update.c about message_delta_encode_message's
//   "changed_offset"/"items"/"type_offset" roles.
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130, +0x0c off it is the hash_table this
    // function hashes the projectile's own datum_index through
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern int message_delta_encode_message(int flag, int message_type, int changed_offset,
    void **items, int type_offset, int count, char force_changed); // 0x4ec940
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// Builds and sends one network delta update for a projectile: update_type == 1 sends a full
// baseline (live position/velocity), anything else sends a delta against the stored
// projectile_network_state. Advances the per-object sequence byte, wrapping 0xff back to 0,
// whenever the send reports a positive result.
int32_t projectile_build_network_update(uint32_t projectile_index, uint32_t unused_arg2,
    uint32_t unused_arg3, int32_t update_type)
{
    object *obj = object_try_and_get(projectile_index, _object_mask_projectile);
    int32_t result;

    if (obj == 0) {
        return 0;
    }

    {
        // Contiguous on purpose -- see file header UNSURE note.
        struct {
            int32_t projectile_hash;  // local_20
            uint8_t baseline_index;   // projectile_data.network_baseline_index
            uint8_t sequence;         // projectile_data.network_sequence
            uint8_t is_first_update;  // (update_type == 0); UNSURE, see file header
        } header;
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
        int32_t message_type = object_type_definitions[obj->type]->network_delta_message_type; // network_delta_message_type
        void *header_ptr = &header;
        int32_t is_full_snapshot = (update_type == 1);
        void *items_array[1];
        void *type_offset;
        struct {
            real position[3];
            real velocity[3];
        } snapshot; // only filled/used when is_full_snapshot; declared here so its address
                    // stays valid through the encode call below

        header.projectile_hash = 0;
        if (projectile_index != (uint32_t)k_datum_index_none) {
            header.projectile_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), projectile_index);
            if (header.projectile_hash == -1) {
                header.projectile_hash = 0;
            }
        }
        header.sequence = proj->network_sequence;
        header.baseline_index = proj->network_baseline_index;
        header.is_first_update = (uint8_t)(update_type == 0);

        if (!is_full_snapshot) {
            items_array[0] = &proj->network_state;
            type_offset = 0;
        } else {
            void *net_ptr = &proj->network_state;

            snapshot.position[0] = obj->position.x;
            snapshot.position[1] = obj->position.y;
            snapshot.position[2] = obj->position.z;
            snapshot.velocity[0] = obj->velocity.i;
            snapshot.velocity[1] = obj->velocity.j;
            snapshot.velocity[2] = obj->velocity.k;

            items_array[0] = &snapshot;
            type_offset = &net_ptr;
        }

        result = message_delta_encode_message(is_full_snapshot, message_type,
            (int)&header_ptr, items_array, (int)type_offset, 1, 0);
    }

    if (0 < result) {
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
        uint8_t sequence = proj->network_sequence + 1;
        proj->network_sequence = sequence;
        if ((int8_t)sequence == -1) {
            proj->network_sequence = 0;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4c0f30):

int FUN_004c0f30(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  char cVar3;
  int **changed_offset;
  int **items;
  int **type_offset;
  int *local_2c;
  int *local_28;
  int *local_24;
  int local_20;
  undefined1 local_1c;
  undefined1 local_1b;
  undefined1 local_1a;
  int local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = object_try_and_get(0x20);
  if (iVar1 == 0) {
    iVar2 = 0;
  }
  else {
    local_20 = 0;
    if (param_1 != -1) {
      local_20 = hash_table_get();
      if (local_20 == -1) {
        local_20 = 0;
      }
    }
    local_1b = *(undefined1 *)(iVar1 + 0x27b);
    local_1c = *(undefined1 *)(iVar1 + 0x27a);
    local_1a = param_4 == 0;
    if (param_4 != 1) {
      type_offset = (int **)0x0;
      local_28 = &local_20;
      items = &local_24;
      changed_offset = &local_28;
      local_24 = (int *)(iVar1 + 0x27c);
    }
    else {
      local_18 = *(int *)(iVar1 + 0x5c);
      local_14 = *(undefined4 *)(iVar1 + 0x60);
      local_10 = *(undefined4 *)(iVar1 + 100);
      local_c = *(undefined4 *)(iVar1 + 0x68);
      local_8 = *(undefined4 *)(iVar1 + 0x6c);
      local_4 = *(undefined4 *)(iVar1 + 0x70);
      local_28 = &local_18;
      type_offset = &local_2c;
      items = &local_28;
      changed_offset = &local_24;
      local_2c = (int *)(iVar1 + 0x27c);
      local_24 = &local_20;
    }
    iVar2 = message_delta_encode_message
                      ((uint)(param_4 == 1),
                       *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar1 + 0xb4)] + 0x10),
                       (int)changed_offset,items,(int)type_offset,1,'\0');
    if ((0 < iVar2) &&
       (cVar3 = *(char *)(iVar1 + 0x27b) + '\x01', *(char *)(iVar1 + 0x27b) = cVar3, cVar3 == -1)) {
      *(undefined1 *)(iVar1 + 0x27b) = 0;
      return iVar2;
    }
  }
  return iVar2;
}
#endif
