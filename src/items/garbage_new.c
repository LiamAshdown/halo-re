// garbage_new  (Ghidra: missed_4bc490, created by hand this pass -- Ghidra never recovered it as
// a function; only reachable through the garbage object_type_definition row)
// address 0x4bc490, size 124 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: the garbage row (0x0069b8d8) carries this address at +0x28 (query_create), the same
//   column family as equipment_new/weapon_new/item_new in this batch and
//   src/objects/object_type_definitions_query_0x28.c's generic dispatcher, called once from
//   object_new_with_datum_role_control right after a placement is activated. types/objects.h
//   object.next_tracked_object (0x110, "singly linked list rooted at object_globals 0x08"),
//   object_globals.first_tracked_object (0x08), object_flags (_object_in_tracked_list_bit
//   0x10000, "object_list_membership_set; the garbage column of the memory dump",
//   _object_definition_flag0_bit 0x40000, _object_connected_to_map_bit 0x80000);
//   types/math.h/objects.h random_seed_global (0x00719cd0). global 0x006b8cbc
//   object_globals_pointer.
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family.
// blam-cc: stack -> object_index
// UNSURE: the field this writes, garbage_data + 0x00 (object + 0x22c), has no name in
//   types/items.h (`garbage_data.unknown_22c`); written here as a raw offset with a comment
//   rather than guessed into the header. It is a random int16 in [300, 600), which reads as a
//   garbage item's random despawn countdown -- garbage_update (0x4bc510, this batch) counts the
//   same field down to zero and then deletes the object.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;               // 0x008603b0
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern uint32_t random_seed_global;            // 0x00719cd0

// The garbage row's query_create hook (object_type_definition +0x28). Links a freshly activated
// garbage object into the object_globals tracked list (unless it, or some later object, already
// linked it), marks it definition-flagged and connected-to-map, and seeds its random despawn
// countdown (garbage_data + 0x00) to a value in [300, 600) ticks. Always reports success.
uint8_t garbage_new(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int16_t *despawn_countdown = (int16_t *)((uint8_t *)obj + k_item_extension_offset); // garbage_data + 0x00, UNSURE field name

    if ((obj->flags & (_object_in_tracked_list_bit | _object_unknown_20000_bit)) == 0) {
        obj->next_tracked_object = object_globals_pointer->first_tracked_object;
        object_globals_pointer->first_tracked_object = object_index;
        obj->flags |= _object_in_tracked_list_bit;
    }
    obj->flags |= _object_definition_flag0_bit | _object_connected_to_map_bit;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    *despawn_countdown = (int16_t)(((random_seed_global >> 0x10) * 300) >> 0x10) + 300;

    return 1;
}

#if 0
Original Ghidra decompilation (0x4bc490):

undefined4 missed_4bc490(uint param_1)

{
  int iVar1;
  int iVar2;

  iVar2 = DAT_006b8cbc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if ((*(uint *)(iVar1 + 0x10) & 0x30000) == 0) {
    *(undefined4 *)(iVar1 + 0x110) = *(undefined4 *)(DAT_006b8cbc + 8);
    *(uint *)(iVar2 + 8) = param_1;
    *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x10000;
  }
  *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0xc0000;
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  *(short *)(iVar1 + 0x22c) = (short)((random_seed_global >> 0x10) * 300 >> 0x10) + 300;
  return CONCAT31((int3)((uint)iVar1 >> 8),1);
}
#endif
