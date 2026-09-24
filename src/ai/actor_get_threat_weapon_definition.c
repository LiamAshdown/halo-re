// actor_get_threat_weapon_definition  (Ghidra: actor_get_threat_weapon_definition, renamed)
// address 0x40f970, size 61 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: phase-4 summary "resolves the actor's current possessed unit to its associated
// actor-type tag definition block" -- actually resolves the THREAT's weapon object (from
// actor_get_threat_weapon_object_index, 0x4282c0) to its owning tag data block.
// register convention: actor_index implicit through actor_get_threat_weapon_object_index
// (no parameters visible in this function's own decompiled C).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index actor_get_threat_weapon_object_index(void); // 0x4282c0, UNSURE: no visible args

void *actor_get_threat_weapon_definition(void)
{
    datum_index weapon_object;

    weapon_object = actor_get_threat_weapon_object_index();
    if (weapon_object != (datum_index)k_datum_index_none) {
        object_header *hdr = (object_header *)object_data->data + (weapon_object & 0xffff);
        object *obj = hdr->data;
        return tag_instances[obj->definition_tag & 0xffff].data;
    }
    return (void *)0;
}

#if 0
Original Ghidra decompilation (0x40f970):

undefined4 FUN_0040f970(void)

{
  uint uVar1;

  uVar1 = actor_get_threat_weapon_object_index();
  if (uVar1 != 0xffffffff) {
    return *(undefined4 *)
            ((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) & 0xffff) *
             0x20 + 0x14 + DAT_0087bc14);
  }
  return 0;
}
#endif
