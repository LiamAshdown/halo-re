// object_find_nearest_biped  (Ghidra: FUN_0056bee0)
// address 0x56bee0, size 219 bytes, name confidence 0.4, rewrite confidence 0.5
// functions.md: "Finds the nearest object of type 0 (biped) to a given reference object by
// scanning all objects and comparing positions."
// evidence: types/objects.h object_iterator (type_mask 1 = biped only), object.vitality_flags
//   (0x106).
// blam-cc: unaff_EDI -> reference_object_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern double sqrt(double x); // a single x87 FSQRT instruction in the original (Ghidra's SQRT())
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern object * object_iterator_next(object_iterator *iterator);           // 0x4f6f20

int32_t object_find_nearest_biped(int32_t reference_object_index) // blam-cc: unaff_EDI
{
    int32_t best_index = -1;
    float best_distance = 3.4028235e+38f;

    object_iterator iter = { _object_mask_biped, 0, 0, 0, 0xffffffff };
    object *obj = object_iterator_next(&iter);
    while (obj != (object *)0) {
        int32_t this_handle = (int32_t)iter.handle;
        if ((this_handle != reference_object_index) && ((obj->vitality_flags & _object_health_frozen_bit) == 0)) {
            float distance;
            if (reference_object_index == -1) {
                distance = 0.0f;
            } else {
                real_point3d a, b;
                object_get_position(&a, (uint32_t)this_handle);
                object_get_position(&b, (uint32_t)reference_object_index);
                float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
                distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
            }
            if (distance < best_distance) {
                best_index = this_handle;
                best_distance = distance;
            }
        }
        obj = object_iterator_next(&iter);
    }
    return best_index;
}

#if 0
Original Ghidra decompilation (0x56bee0):

int FUN_0056bee0(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int unaff_EDI;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  int local_8;
  undefined4 local_4;

  iVar4 = -1;
  local_2c = 3.4028235e+38;
  local_4 = 0x86868686;
  local_10 = 1;
  local_c = 0;
  local_a = 0;
  local_8 = -1;
  iVar3 = object_iterator_next(&local_10);
  iVar2 = local_8;
  while (iVar3 != 0) {
    local_8 = iVar2;
    if ((iVar2 != unaff_EDI) && ((*(byte *)(iVar3 + 0x106) & 4) == 0)) {
      if (unaff_EDI == -1) {
        fVar1 = 0.0;
      }
      else {
        object_get_position();
        object_get_position();
        fVar1 = SQRT((local_28 - local_1c) * (local_28 - local_1c) +
                     (local_24 - local_18) * (local_24 - local_18) +
                     (local_20 - local_14) * (local_20 - local_14));
      }
      if (fVar1 < local_2c) {
        iVar4 = iVar2;
        local_2c = fVar1;
      }
    }
    iVar3 = object_iterator_next(&local_10);
    iVar2 = local_8;
  }
  return iVar4;
}
#endif
