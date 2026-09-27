// actor_get_threat_weapon_object_index  (Ghidra: actor_get_threat_weapon_object_index, already named)
// address 0x4282c0, size 176 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4282c0..0x42836f.)
// evidence: types/ai.h actor.unknown_161/active_unit_index(0x158)/unit_index(0x18)/
//   actor_definition_tag(0x5c). Calls unit_get_weapon_object_index (0x569970, UNSURE
//   signature, not in this rewrite range).
//   UNSURE: unit+0x2f2 (a weapon-slot index) and unit+0x2f8+slot*4 (a per-slot weapon object
//   array) have no established names in types/units.h; accessed as raw offsets. The Actor
//   tag flags bit 0x40 tested here is ActorFlags bit 6, "crouch_when_not_in_combat" per its
//   comment list in types/tags.h, which does not obviously fit a "has a fallback weapon"
//   test; kept literal rather than reinterpreted.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX unit, CX slot

// blam-cc: EAX -> actor_index
// Returns the weapon object index of the actor's currently perceived hostile unit's selected
// weapon slot, if it has one; otherwise, if the actor controls its own unit and that unit's
// Actor tag does not have flag bit 0x40 set, falls back to that unit's own weapon.
datum_index actor_get_threat_weapon_object_index(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index result = (datum_index)k_datum_index_none;

    if (self->unknown_161 != 0 && self->active_unit_index != (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        int16_t slot = *(int16_t *)((uint8_t *)unit_object + 0x2f2); // UNSURE offset

        result = (datum_index)k_datum_index_none;
        if (slot != -1) {
            result = *(datum_index *)((uint8_t *)unit_object + 0x2f8 + slot * 4); // UNSURE offset
        }
        if (result != (datum_index)k_datum_index_none) {
            return result;
        }
    }

    if (self->unit_index != (datum_index)k_datum_index_none) {
        uint8_t *variant_tag = (uint8_t *)tag_instances[self->actor_variant_tag & 0xffff].data; // 0x42832f: actor +0x5c
        if ((*variant_tag & 0x40) == 0) {
            // 0x42834a: EAX = unit_index, CX = that unit's current weapon slot (+0x2f2); tail jmp
            object *own_unit = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
            return unit_get_weapon_object_index(self->unit_index, *(int16_t *)((uint8_t *)own_unit + 0x2f2));
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4282c0):

int actor_get_threat_weapon_object_index(void)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;

  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar4 = -1;
  if ((*(char *)(iVar3 + 0x161) != '\0') && (*(uint *)(iVar3 + 0x158) != 0xffffffff)) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar3 + 0x158) & 0xffff) * 0xc);
    sVar1 = *(short *)(iVar2 + 0x2f2);
    iVar4 = -1;
    if (sVar1 != -1) {
      iVar4 = *(int *)(iVar2 + 0x2f8 + sVar1 * 4);
    }
    if (iVar4 != -1) {
      return iVar4;
    }
  }
  if ((*(int *)(iVar3 + 0x18) != -1) &&
     ((**(byte **)((*(uint *)(iVar3 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 0x40) == 0)) {
    iVar4 = unit_get_weapon_object_index();
    return iVar4;
  }
  return iVar4;
}
#endif
