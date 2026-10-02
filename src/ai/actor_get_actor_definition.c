// actor_get_actor_definition  (Ghidra: actor_get_actor_definition, already named)
// address 0x40fa70, size 127 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x40fa70..0x40faee)
// evidence: phase-4 summary "returns the actor definition (tag data block) to use,
// preferring a possessed unit's actor-type override when present"; reads
// actor.actor_definition_tag as the default, then follows the threat weapon object's
// definition tag to an override actor-type index at +0x3c8.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index
// UNSURE: the override index at (weapon definition tag data)+0x3c8 is not named in
// types/tags.h for whatever tag that resolves to (likely a Weapon or Projectile tag);
// kept as a raw offset.
// UNSURE: the default path reads actor.actor_variant_tag (0x5c), not actor_definition_tag
// (0x58), and returns that tag's data pointer directly -- i.e. the default return value is
// an ActorVariant tag block, not an Actor tag block, despite this function's established
// name. Preserved exactly as Ghidra has it rather than "fixed" to match the name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index); // 0x4282c0, EAX actor

// blam-cc: EAX -> actor_index
void *actor_get_actor_definition(datum_index actor_index)
{
    actor *self;
    void *default_definition;
    datum_index weapon_object;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    default_definition = tag_instances[self->actor_variant_tag & 0xffff].data; // see UNSURE above

    weapon_object = actor_get_threat_weapon_object_index(actor_index); // 0x40faa0: EAX still = the actor index
    if (weapon_object != (datum_index)k_datum_index_none) {
        object_header *hdr = (object_header *)object_data->data + (weapon_object & 0xffff);
        object *obj = hdr->data;
        void *weapon_definition = tag_instances[obj->definition_tag & 0xffff].data;
        if (weapon_definition != 0) {
            uint32_t override_index = *(uint32_t *)((uint8_t *)weapon_definition + 0x3c8);
            if (override_index != (uint32_t)-1) {
                return tag_instances[override_index & 0xffff].data;
            }
        }
    }
    return default_definition;
}

#if 0
Original Ghidra decompilation (0x40fa70):

undefined4 actor_get_actor_definition(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;

  iVar3 = DAT_0087bc14;
  uVar1 = *(undefined4 *)
           ((*(uint *)((in_EAX & 0xffff) * 0x724 + 0x5c + *(int *)(DAT_00880360 + 0x34)) & 0xffff) *
            0x20 + 0x14 + DAT_0087bc14);
  uVar4 = actor_get_threat_weapon_object_index();
  if (((uVar4 != 0xffffffff) &&
      (iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) &
                        0xffff) * 0x20 + 0x14 + iVar3), iVar2 != 0)) &&
     (uVar4 = *(uint *)(iVar2 + 0x3c8), uVar4 != 0xffffffff)) {
    return *(undefined4 *)((uVar4 & 0xffff) * 0x20 + 0x14 + iVar3);
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
