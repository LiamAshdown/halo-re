// object_destroy_region
// address 0x4f02d0, size 268 bytes
// name confidence: 0.75 (Ghidra-recovered name, corroborated by the "~damaged" permutation-name
// string and out/phase4/objects_functions.md's summary)
// rewrite confidence: 0.7 (raised from 0.55 by the phase-4 review pass: object_set_permutation_by_name's missing EAX object-index argument was resolved from the disassembly)
// evidence: types/objects.h object.destroyed_region_flags (0x174), object.vitality_flags (0x106/
// 0x107, _object_region_response_80/100/200/400_bit); types/tags.h ModelCollisionGeometry.regions
// (TagReflexive 0x240/0x244), ModelCollisionGeometryRegion.flags (0x20, bits 0x02, 0x20, 0x40,
// 0x80, 0x100 tested here).
// register convention: uint32_t object_index in EAX (in_EAX); int32_t region_index on the stack
// (param_1).
// blam-cc: EAX=object_index, stack=region_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six
extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
                                           char use_matched_index); // 0x4f6c60
    // PHASE-4 REVIEW: this was declared with only the three stack arguments Ghidra shows.
    // objdump 0x4f0362 is `mov eax,ebx` immediately before the call, and 0x4f6c60 opens by
    // masking EAX into the object_data stride, so the object index is a fourth (register)
    // argument. Matches src/objects/object_set_permutation_by_name.c's own definition.
extern void object_set_health_frozen_flag(uint32_t object_index); // UNSURE: zero visible args at this call site; this module, 0x4eda20
extern void object_type_definitions_notify_region_damage(uint32_t object_index, uint32_t argument_1,
    uint32_t argument_2); // 0x4f4160, EBX object, stack (region index, region flags): its +0x40 hooks get all three
    // (0x4f417f / 0x4f41a9 read both stack arguments).

void object_destroy_region(uint32_t object_index, int32_t region_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if (definition->collision_model.tag_id.index != 0xffff) {
        if ((obj->destroyed_region_flags & (1 << (region_index & 0x1f))) == 0) {
            ModelCollisionGeometry *geometry =
                (ModelCollisionGeometry *)tag_instances[definition->collision_model.tag_id.index].data;
            ModelCollisionGeometryRegion *region = &((ModelCollisionGeometryRegion *)geometry->regions.pointer)[region_index];

            // 0x4f032a..0x4f0351: EAX = the object, ECX = the region's +0x44 effect, stack: object, -1, 0..
            effect_new_on_object(object_index, *(datum_index *)&((struct ModelCollisionGeometryRegion *)region)->destroyed_effect.tag_id, object_index, -1,
                0.0f, 0.0f, 0, 0);
            object_set_permutation_by_name(object_index, "~damaged", (int16_t)region_index, 1);

            if ((region->flags & 0x20) != 0) {
                obj->vitality_flags |= _object_region_response_80_bit;
            }
            if ((region->flags & 0x40) != 0) {
                obj->vitality_flags |= _object_region_response_100_bit;
            }
            if ((region->flags & 0x80) != 0) {
                *((uint8_t *)obj + 0x107) |= 2; // _object_region_response_200_bit
            }
            if ((region->flags & 0x100) != 0) {
                *((uint8_t *)obj + 0x107) |= 4; // _object_region_response_400_bit
            }
            if ((region->flags & 2) != 0) {
                object_set_health_frozen_flag(object_index); // 0x4f03af mov eax,ebx // UNSURE: Ghidra shows no visible arguments here either
            }

            obj->destroyed_region_flags |= (uint16_t)(1 << (region_index & 0x1f));
            object_type_definitions_notify_region_damage(object_index, (uint32_t)region_index, region->flags);
                // 0x4f03bf: EBX object, stack (the region index dword, the region flags +0x20)
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f02d0):

void object_destroy_region(undefined4 param_1)

{
  uint *puVar1;
  uint uVar2;
  uint in_EAX;
  int iVar3;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = *(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
  if (uVar2 != 0xffffffff) {
    if (((uint)(ushort)puVar1[0x5d] & 1 << ((byte)param_1 & 0x1f)) == 0) {
      iVar3 = (short)param_1 * 0x54 +
              *(int *)(*(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x244);
      FUN_004507a0();
      object_set_permutation_by_name("~damaged",param_1,1);
      if ((*(byte *)(iVar3 + 0x20) & 0x20) != 0) {
        *(ushort *)((int)puVar1 + 0x106) = *(ushort *)((int)puVar1 + 0x106) | 0x80;
      }
      if ((*(byte *)(iVar3 + 0x20) & 0x40) != 0) {
        *(ushort *)((int)puVar1 + 0x106) = *(ushort *)((int)puVar1 + 0x106) | 0x100;
      }
      if ((*(byte *)(iVar3 + 0x20) & 0x80) != 0) {
        *(byte *)((int)puVar1 + 0x107) = *(byte *)((int)puVar1 + 0x107) | 2;
      }
      if ((*(uint *)(iVar3 + 0x20) & 0x100) != 0) {
        *(byte *)((int)puVar1 + 0x107) = *(byte *)((int)puVar1 + 0x107) | 4;
      }
      if ((*(byte *)(iVar3 + 0x20) & 2) != 0) {
        FUN_004eda20();
      }
      *(ushort *)(puVar1 + 0x5d) = (ushort)puVar1[0x5d] | (ushort)(1 << ((byte)param_1 & 0x1f));
      FUN_004f4160(param_1,*(undefined4 *)(iVar3 + 0x20));
    }
  }
  return;
}
#endif
