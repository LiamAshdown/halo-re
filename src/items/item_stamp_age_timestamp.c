// item_stamp_age_timestamp  (Ghidra: missed_4bc460, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the weapon and equipment
// object_type_definition rows)
// address 0x4bc460, size 38 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: both the weapon row (0x0069b748) and the equipment row (0x0069b810) carry this exact
//   address at +0x7c (override_call_7c), confirmed by reading both rows out of .data at file
//   offset VA-0x400000; the garbage row's +0x7c is null. Dispatched by
//   src/objects/object_type_override_call_0x7c.c, which scans a type's subdefinitions from index
//   15 down and calls the first non-null +0x7c it finds -- so this is each concrete type's own
//   final override, not inherited from item/object. types/objects.h object.unknown_00c, whose
//   meaning as a game-tick stamp is established by out/phase4/projectiles_types_notes.md
//   ("types/objects.h: object + 0x0c is a game-tick stamp, not a datum handle") and reused here
//   by src/items/weapon_is_old_enough.c and equipment_is_old_enough.c, both of which test this
//   same field against a per-type minimum age. global 0x006f1d6c game_time_globals (+0x0c the
//   game tick).
// register convention: object index is a plain stack cdecl parameter -- confirmed against
//   objdump -d -M intel bin/halo.exe (0x4bc460 `mov eax,[esp+0x4]`), which also settles
//   src/objects/object_type_override_call_0x7c.c's own header comment ("register convention:
//   object index in ESI"): the dispatcher's caller does push ESI onto the stack right before the
//   call (`push esi` / `call edi` at 0x4f47ad), so the callee reads it as an ordinary stack
//   argument, not through the register.
// blam-cc: stack -> object_index
// UNSURE of the name: this is the write side of the age-timestamp read by weapon_is_old_enough /
//   equipment_is_old_enough, so it (re)starts that clock; nothing in this batch pins the exact
//   game event that triggers the override_call_7c dispatch (item drop, pickup, or similar).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern void *game_time_globals; // 0x006f1d6c, +0x0c the game tick

// The weapon and equipment rows' override_call_7c hook. Stamps object.unknown_00c with the
// current game tick, restarting the age clock that weapon_is_old_enough / equipment_is_old_enough
// test.
void item_stamp_age_timestamp(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    obj->unknown_00c = *(int32_t *)((uint8_t *)game_time_globals + 0xc);
}

#if 0
Original Ghidra decompilation (0x4bc460):

void missed_4bc460(uint param_1)

{
  *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xc) =
       *(undefined4 *)(DAT_006f1d6c + 0xc);
  return;
}
#endif
