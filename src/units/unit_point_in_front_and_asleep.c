// unit_point_in_front_and_asleep  (Ghidra: FUN_0056bc80)
// address 0x56bc80, size 215 bytes, name confidence 0.3, rewrite confidence 0.9
// REWRITTEN from objdump 0x56bc80..0x56bd59 (the draft lost the unit handle, which arrives in EDI and is moved to
//   ECX for object_try_and_get). EAX: world point, EDI: unit. True when the unit is a biped whose tag lacks unit
//   flag 0x10000 and either the point lies behind its look vector (dot(centre - point, look) > 0, summed z, y, x
//   like the binary) or its seat/state name (0x56c2f0) is "asleep" (unit_base_animation_state_names[0]).
// blam-cc: EAX -> world_point, EDI -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern char *unit_base_animation_state_names[6]; // 0x0069fde4
extern tag_instance *tag_instances;              // 0x0087bc14

extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern char * unit_get_seat_or_state_name(uint32_t unit_index); // 0x56c2f0, EAX

uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)object_try_and_get(unit_index, 3);
    float dot;

    if (obj == 0 || ((unit_object *)obj)->base.type != 0) {
        return 0;
    }
    if (*(uint32_t *)((uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data + 0x17c) & 0x10000) {
        return 0;
    }
    dot = (((unit_object *)obj)->base.bounding_center.z - world_point->z) * ((unit_object *)obj)->unit.looking_vector.k +
          (((unit_object *)obj)->base.bounding_center.y - world_point->y) * ((unit_object *)obj)->unit.looking_vector.j +
          (((unit_object *)obj)->base.bounding_center.x - world_point->x) * ((unit_object *)obj)->unit.looking_vector.i;
    if (!(dot > 0.0f)) {
        const char *name = unit_get_seat_or_state_name(unit_index);
        const char *asleep = unit_base_animation_state_names[0];

        for (;;) {
            if (*asleep != *name) return 0;
            if (*asleep == 0) break;
            asleep++;
            name++;
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
