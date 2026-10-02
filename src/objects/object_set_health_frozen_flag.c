// object_set_health_frozen_flag
// address 0x4eda20, size 231 bytes
// name confidence: 0.35 (still FUN_004eda20 in Ghidra; named from the bit it tests/sets,
// object.vitality_flags bit 0x0004, which out/phase4/objects_types_notes.md documents as
// "the dead column of the memory dump; guards the health restore and stun clear paths")
// rewrite confidence: 0.45
// evidence: types/objects.h object.vitality_flags (0x106, _object_health_frozen_bit), object.type
// (0xb4), object.first_child_object (0x118)/next_object (0x114); types/tags.h Object.collision_model
// (0x7c) gates the object_datum_consume_pending_flag-style notify at effect_new_on_object.
// UNSURE: the seated-child gate reads object+0x218 and object+0x2f0, which are unit-extension
// fields (not part of the common object struct documented in objects.h), and the bit it sets on
// each eligible child (0x0020) is not in object_vitality_flags; it is preserved literally.
// UNSURE: the byte global at 0x0087abc0 is read but not owned by this module; its meaning is
// unknown, so it is declared as a bare extern.
// register convention: uint32_t object_index in EAX (in_EAX).
// blam-cc: EAX=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t g_0087abc0;          // 0x0087abc0, UNSURE: not owned by this module

extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six
extern void object_set_shield_depleted_flag(uint32_t object_index); // 0x4edb10, blam-cc: EDI=object_index

void object_set_health_frozen_flag(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return;
    }

    obj->vitality_flags |= _object_health_frozen_bit;

    if (((Object *)tag_instances[obj->definition_tag & 0xffff].data)->collision_model.tag_id.index != 0xffff) {
        // 0x4eda73..0x4eda91: EAX = the object, ECX = the collision model's +0xb4 effect, stack: object, -1, 0..
        effect_new_on_object(object_index,
            *(datum_index *)((uint8_t *)tag_instances[((Object *)tag_instances[obj->definition_tag & 0xffff].data)
                ->collision_model.tag_id.index].data + 0xb4),
            object_index, -1, 0.0f, 0.0f, 0, 0);
    }

    if (obj->type == _object_type_vehicle) {
        datum_index child_index = obj->first_child_object;
        while (child_index != (datum_index)0xffffffff) {
            object *child = headers[child_index & 0xffff].data;
            uint8_t *child_bytes = (uint8_t *)child;

            if (child->type == _object_type_biped &&
                (*(int32_t *)(child_bytes + 0x218) == -1 || g_0087abc0 == 0) &&
                *(int16_t *)(child_bytes + 0x2f0) != -1) {
                child->vitality_flags |= 0x0020; // UNSURE: undocumented vitality bit
            }
            child_index = child->next_object;
        }
    }

    object_set_shield_depleted_flag(object_index);
}

#if 0
Original Ghidra decompilation (0x4eda20):

void FUN_004eda20(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;

  iVar3 = DAT_0087bc14;
  iVar4 = DAT_008603b0;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((*(ushort *)((int)puVar1 + 0x106) & 4) == 0) {
    *(ushort *)((int)puVar1 + 0x106) = *(ushort *)((int)puVar1 + 0x106) | 4;
    if (*(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + iVar3) + 0x7c) != -1) {
      FUN_004507a0();
      iVar4 = DAT_008603b0;
    }
    if ((short)puVar1[0x2d] == 1) {
      uVar2 = puVar1[0x46];
      while (uVar2 != 0xffffffff) {
        iVar3 = *(int *)(*(int *)(iVar4 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        if ((*(short *)(iVar3 + 0xb4) == 0) &&
           (((*(int *)(iVar3 + 0x218) == -1 || (DAT_0087abc0 == '\0')) &&
            (*(short *)(iVar3 + 0x2f0) != -1)))) {
          *(ushort *)(iVar3 + 0x106) = *(ushort *)(iVar3 + 0x106) | 0x20;
        }
        uVar2 = *(uint *)(iVar3 + 0x114);
      }
    }
    FUN_004edb10();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
