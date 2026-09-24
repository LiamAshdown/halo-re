// unit_point_in_front_and_asleep  (Ghidra: FUN_0056bc80)
// address 0x56bc80, size 215 bytes, name confidence 0.3, rewrite confidence 0.3
// functions.md: "Tests whether a given world point lies in front of the controlled unit and
// matches an additional name-based condition."
// evidence: types/objects.h object.bounding_center (0xa0), .type (0xb4); types/units.h
//   unit_data.looking_vector (0x260); types/tags.h Unit.unit_flags (0x17c, bit 0x10000 not
//   named by this module); unit_base_animation_state_names[6] (0x0069fde4, established in
//   src/units/unit_set_or_test_seat_and_weapon_label.c) -- the compare is against entry 0,
//   "asleep".
// blam-cc: in_EAX -> world_point, implicit ECX -> controlling unit handle 3 (object_try_and_get
//   mask, resolved via whatever the caller left in ECX).
// UNSURE: the exact object_try_and_get handle argument (ECX) is not visible in this
// decompilation; functions.md's "the controlled unit" phrasing is the best available reading.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern char *unit_base_animation_state_names[6]; // 0x0069fde4
extern tag_instance *tag_instances;              // 0x0087bc14

extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern char * unit_get_seat_or_state_name(uint32_t unit_index); // 0x56c2f0, UNSURE signature

uint8_t unit_point_in_front_and_asleep(real_point3d *world_point) // blam-cc: in_EAX
{
    object *unit_obj = object_try_and_get(k_datum_index_none /* UNSURE: implicit ECX handle */, _object_mask_unit);
    if ((unit_obj == (object *)0) || (unit_obj->type != _object_type_biped)) {
        return 0;
    }
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    if ((unit_tag->unit_flags & 0x10000) != 0) {
        return 0;
    }

    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    float dot = (unit_obj->bounding_center.x - world_point->x) * unit->looking_vector.i +
                (unit_obj->bounding_center.y - world_point->y) * unit->looking_vector.j +
                (unit_obj->bounding_center.z - world_point->z) * unit->looking_vector.k;

    if ((dot < 0.0f) || (dot == 0.0f)) {
        char *seat_name = unit_get_seat_or_state_name(k_datum_index_none /* UNSURE: same implicit handle */);
        char *asleep = unit_base_animation_state_names[0];
        int32_t i = 0;
        for (;;) {
            if (seat_name[i] != asleep[i]) return 0;
            if (seat_name[i] == '\0') break;
            i++;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x56bc80):

uint FUN_0056bc80(void)

{
  byte bVar1;
  float fVar2;
  float *in_EAX;
  uint *puVar3;
  undefined4 uVar4;
  byte *pbVar5;
  byte *pbVar6;
  bool bVar7;

  puVar3 = (uint *)object_try_and_get(3);
  if (((puVar3 != (uint *)0x0) && ((short)puVar3[0x2d] == 0)) &&
     ((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x10000) == 0)
     ) {
    fVar2 = ((float)puVar3[0x28] - *in_EAX) * (float)puVar3[0x98] +
            ((float)puVar3[0x29] - in_EAX[1]) * (float)puVar3[0x99] +
            ((float)puVar3[0x2a] - in_EAX[2]) * (float)puVar3[0x9a];
    uVar4 = CONCAT22((short)((uint)puVar3 >> 0x10),
                     (ushort)(fVar2 < 0.0) << 8 | (ushort)NAN(fVar2) << 10 |
                     (ushort)(fVar2 == 0.0) << 0xe);
    if (fVar2 < 0.0 != 0 || (fVar2 == 0.0) != 0) {
      pbVar5 = (byte *)FUN_0056c2f0();
      pbVar6 = PTR_DAT_0069fde4;
      do {
        bVar1 = *pbVar6;
        bVar7 = bVar1 < *pbVar5;
        if (bVar1 != *pbVar5) {
LAB_0056bd48:
          puVar3 = (uint *)((1 - (uint)bVar7) - (uint)(bVar7 != 0));
          goto LAB_0056bd4d;
        }
        if (bVar1 == 0) break;
        bVar1 = pbVar6[1];
        bVar7 = bVar1 < pbVar5[1];
        if (bVar1 != pbVar5[1]) goto LAB_0056bd48;
        pbVar6 = pbVar6 + 2;
        pbVar5 = pbVar5 + 2;
      } while (bVar1 != 0);
      puVar3 = (uint *)0x0;
LAB_0056bd4d:
      uVar4 = 0;
      if (puVar3 != (uint *)0x0) goto LAB_0056bd56;
    }
    return CONCAT31((int3)((uint)uVar4 >> 8),1);
  }
LAB_0056bd56:
  return (uint)puVar3 & 0xffffff00;
}
#endif
