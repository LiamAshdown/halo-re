// object_set_shield_depleted_flag
// address 0x4edb10, size 136 bytes
// name confidence: 0.4 (still FUN_004edb10 in Ghidra; named from the bit it tests/sets,
// object.vitality_flags bit 0x0008 == _object_shield_depleted_bit)
// rewrite confidence: 0.55
// evidence: types/objects.h object.vitality_flags (0x106, _object_shield_depleted_bit),
// object.current_shield_damage (0xe8, "flicker meter, clamped to 1.0"); types/tags.h
// Object.collision_model (0x7c) gates the effect_new_on_object call, matching the same idiom used by
// object_set_health_frozen_flag.
// register convention: uint32_t object_index in EDI (unaff_EDI).
// blam-cc: EDI=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void effect_new_on_object(); // effects module, 0x4507a0
    // The convention of this foreign callee is not established: different call sites in this
    // module pass different numbers of visible arguments, and it also takes values in EAX
    // and ECX that the decompiler never models. Declared with an empty parameter list so
    // every site in the module agrees on ONE declaration without fabricating arguments.
extern void object_regions_reset_permutation_lock(uint32_t object_index, int8_t unlock); // 0x4f03e0

void object_set_shield_depleted_flag(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if ((obj->vitality_flags & _object_shield_depleted_bit) != 0) {
        return;
    }

    if (((Object *)tag_instances[obj->definition_tag & 0xffff].data)->collision_model.tag_id.index != 0xffff) {
        effect_new_on_object(); // UNSURE: Ghidra shows no visible arguments
    }

    obj->vitality_flags |= _object_shield_depleted_bit;
    obj->current_shield_damage = 0.0f;

    // UNSURE: Ghidra shows this call with zero visible arguments (cc=unknown on both sides).
    // object_regions_reset_permutation_lock actually reads an object index and an unlock flag
    // (BL); the object index is assumed forwarded from this function's own object_index, and
    // the unlock flag is assumed to be the constant 0 (lock every eligible region, matching the
    // "shield just depleted" event this function fires on) since no source register for it is
    // visible at this call site.
    object_regions_reset_permutation_lock(object_index, 0);
}

#if 0
Original Ghidra decompilation (0x4edb10):

void FUN_004edb10(void)

{
  uint *puVar1;
  uint unaff_EDI;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  if ((*(byte *)((int)puVar1 + 0x106) & 8) == 0) {
    if (*(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c) != -1) {
      FUN_004507a0();
    }
    *(ushort *)((int)puVar1 + 0x106) = *(ushort *)((int)puVar1 + 0x106) | 8;
    puVar1[0x3a] = 0;
    FUN_004f03e0();
  }
  return;
}
#endif
