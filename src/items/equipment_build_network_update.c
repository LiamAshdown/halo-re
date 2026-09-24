// equipment_build_network_update  (Ghidra: FUN_004bc0f0; renamed per
// out/phase4/items_types_notes.md: "equipment_build_network_update (0x4bc0f0) reads 0x245 and
// 0x246, points the delta encoder at 0x248, and post-increments 0x246 with a 0xff -> 0 wrap")
// address 0x4bc0f0, size 338 bytes
// name confidence: 0.6   rewrite confidence: 0.4
// evidence: types/items.h equipment_data (network_state at 0x248, network_baseline_index
//   0x245, network_sequence 0x246); types/objects.h object (position 0x05c, velocity 0x068,
//   angular_velocity 0x08c, type 0x0b4), object_type_definition (unknown_10 -- documented by
//   types/items.h as network_delta_message_type: "+0x10 int32 network_delta_message_type,
//   index into the network message group"); object_try_and_get mask _object_mask_equipment.
// register convention: item_index in object_try_and_get's hidden ECX slot, matching the
//   two-arg canonical extern used across this codebase (object_apply_damage.c etc); the
//   function's own four parameters are already a plain __cdecl stack signature in Ghidra's
//   output (no unrecognized registers). arg2/arg3 are read nowhere in the body and are kept
//   for call-site compatibility only.
// UNSURE: message_delta_encode_message's "changed_offset" parameter (declared `int` in its own
//   canonical prototype) is fed a pointer here, exactly as equipment_build_creation_message
//   feeds its "items" parameter a pointer-to-pointer. The three bytes right after the item hash
//   on the stack (baseline_index, sequence, is_first_update) are never read back by name in
//   Ghidra's decompilation, which strongly suggests message_delta_encode_message reads them as
//   a wider header blob through the same pointer rather than Ghidra simply losing dead code;
//   they are kept byte-for-byte contiguous with the hash below for that reason, but the claim
//   is not verified against message_delta_encode_message's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130, not owned by this module; the
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

// Builds and sends one network delta update for an equipment item: update_type == 1 sends a
// full baseline (the live position/velocity/angular_velocity), anything else sends a delta
// against the stored equipment_network_state. Advances the per-object sequence byte, wrapping
// 0xff back to 0, whenever the send reports a positive result.
int32_t equipment_build_network_update(uint32_t item_index, uint32_t unused_arg2,
    uint32_t unused_arg3, int32_t update_type)
{
    object *obj = object_try_and_get(item_index, _object_mask_equipment);
    int32_t result;

    if (obj == 0) {
        return 0;
    }

    {
        // Contiguous on purpose -- see file header UNSURE note.
        struct {
            int32_t item_hash;        // local_2c
            uint8_t baseline_index;   // local_28, equipment_data.network_baseline_index
            uint8_t sequence;         // local_27, equipment_data.network_sequence
            uint8_t is_first_update;  // local_26 = (update_type == 0); UNSURE, see file header
        } header;
        equipment_network_state *net = (equipment_network_state *)((uint8_t *)obj + 0x248);
        int32_t message_type = object_type_definitions[obj->type]->unknown_10; // network_delta_message_type
        void *header_ptr = &header;
        int32_t is_full_snapshot = (update_type == 1);
        void *items_array[1];
        void *type_offset;
        real snapshot[9]; // only filled/used when is_full_snapshot; declared here so its
                          // address stays valid through the encode call below

        header.item_hash = 0;
        if (item_index != (uint32_t)k_datum_index_none) {
            header.item_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), item_index);
            if (header.item_hash == -1) {
                header.item_hash = 0;
            }
        }
        header.sequence = *((uint8_t *)obj + 0x246);
        header.baseline_index = *((uint8_t *)obj + 0x245);
        header.is_first_update = (uint8_t)(update_type == 0);

        if (!is_full_snapshot) {
            items_array[0] = net;
            type_offset = 0;
        } else {
            void *net_ptr = net;

            snapshot[0] = obj->position.x;
            snapshot[1] = obj->position.y;
            snapshot[2] = obj->position.z;
            snapshot[3] = obj->velocity.i;
            snapshot[4] = obj->velocity.j;
            snapshot[5] = obj->velocity.k;
            snapshot[6] = obj->angular_velocity.i;
            snapshot[7] = obj->angular_velocity.j;
            snapshot[8] = obj->angular_velocity.k;

            items_array[0] = snapshot;
            type_offset = &net_ptr;
        }

        result = message_delta_encode_message(is_full_snapshot, message_type,
            (int)&header_ptr, items_array, (int)type_offset, 1, 0);
    }

    if (0 < result) {
        uint8_t sequence = *((uint8_t *)obj + 0x246) + 1;
        *((uint8_t *)obj + 0x246) = sequence;
        if ((int8_t)sequence == -1) {
            *((uint8_t *)obj + 0x246) = 0;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4bc0f0):

int FUN_004bc0f0(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  char cVar3;
  int **changed_offset;
  int **items;
  int **type_offset;
  int *local_38;
  int *local_34;
  int *local_30;
  int local_2c;
  undefined1 local_28;
  undefined1 local_27;
  undefined1 local_26;
  int local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = object_try_and_get(8);
  if (iVar1 == 0) {
    iVar2 = 0;
  }
  else {
    local_2c = 0;
    if (param_1 != -1) {
      local_2c = hash_table_get();
      if (local_2c == -1) {
        local_2c = 0;
      }
    }
    local_27 = *(undefined1 *)(iVar1 + 0x246);
    local_28 = *(undefined1 *)(iVar1 + 0x245);
    local_26 = param_4 == 0;
    if (param_4 != 1) {
      type_offset = (int **)0x0;
      local_34 = &local_2c;
      items = &local_30;
      changed_offset = &local_34;
      local_30 = (int *)(iVar1 + 0x248);
    }
    else {
      local_24 = *(int *)(iVar1 + 0x5c);
      local_20 = *(undefined4 *)(iVar1 + 0x60);
      local_1c = *(undefined4 *)(iVar1 + 100);
      local_18 = *(undefined4 *)(iVar1 + 0x68);
      local_14 = *(undefined4 *)(iVar1 + 0x6c);
      local_10 = *(undefined4 *)(iVar1 + 0x70);
      local_c = *(undefined4 *)(iVar1 + 0x8c);
      local_8 = *(undefined4 *)(iVar1 + 0x90);
      local_4 = *(undefined4 *)(iVar1 + 0x94);
      local_34 = &local_24;
      type_offset = &local_38;
      items = &local_34;
      changed_offset = &local_30;
      local_38 = (int *)(iVar1 + 0x248);
      local_30 = &local_2c;
    }
    iVar2 = message_delta_encode_message
                      ((uint)(param_4 == 1),
                       *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar1 + 0xb4)] + 0x10),
                       (int)changed_offset,items,(int)type_offset,1,'\0');
    if ((0 < iVar2) &&
       (cVar3 = *(char *)(iVar1 + 0x246) + '\x01', *(char *)(iVar1 + 0x246) = cVar3, cVar3 == -1)) {
      *(undefined1 *)(iVar1 + 0x246) = 0;
      return iVar2;
    }
  }
  return iVar2;
}
#endif
