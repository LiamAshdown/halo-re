// item_compute_rotation  (Ghidra: item_compute_ground_alignment_rotation; renamed per
// types/items.h: "item_compute_rotation owns bit 0x04 -- sets it when the object angular
// velocity is nonzero, clears it and forces sine/cosine to 0/1 otherwise")
// address 0x4bd500, size 207 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/items.h item_flags._item_rotation_valid_bit (0x04), item_data.rotation_axis
//   / rotation_sine / rotation_cosine (0x218/0x224/0x228); types/objects.h object.flags (0x10),
//   object.angular_velocity (0x08c); global 0x008603b0 object_data.
// register convention: object index in EAX (in_EAX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern double sqrt(double x); // a single x87 FSQRT instruction, see src/math/quaternion_normalize.c
extern double fsin(double x);
extern double fcos(double x);

// Recomputes an item's rotation_axis / rotation_sine / rotation_cosine from its current
// angular velocity, unless the object is already at rest (object flags bit 0x20), in which
// case the axis is left alone (item_update owns it in that state) but the trig pair is still
// refreshed to match whichever branch was taken.
void item_compute_rotation(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    real magnitude = (real)sqrt((double)obj->angular_velocity.k * (double)obj->angular_velocity.k +
                                 (double)obj->angular_velocity.j * (double)obj->angular_velocity.j +
                                 (double)obj->angular_velocity.i * (double)obj->angular_velocity.i);

    if (magnitude != 0.0f) {
        item->flags |= _item_rotation_valid_bit;
        if ((obj->flags & 0x20) == 0) {
            real inverse = 1.0f / magnitude;
            item->rotation_axis.i = inverse * obj->angular_velocity.i;
            item->rotation_axis.j = inverse * obj->angular_velocity.j;
            item->rotation_axis.k = inverse * obj->angular_velocity.k;
        }
        item->rotation_sine = (real)fsin((double)magnitude);
        item->rotation_cosine = (real)fcos((double)magnitude);
        return;
    }
    item->flags &= ~_item_rotation_valid_bit;
    item->rotation_sine = 0.0f;
    item->rotation_cosine = 1.0f;
}

#if 0
Original Ghidra decompilation (0x4bd500):

void item_compute_ground_alignment_rotation(void)

{
  int iVar1;
  uint in_EAX;
  float10 fVar2;
  float10 fVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar2 = SQRT((float10)*(float *)(iVar1 + 0x94) * (float10)*(float *)(iVar1 + 0x94) +
               (float10)*(float *)(iVar1 + 0x90) * (float10)*(float *)(iVar1 + 0x90) +
               (float10)*(float *)(iVar1 + 0x8c) * (float10)*(float *)(iVar1 + 0x8c));
  if (fVar2 != (float10)0.0) {
    *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) | 4;
    if ((*(byte *)(iVar1 + 0x10) & 0x20) == 0) {
      fVar3 = (float10)1.0 / fVar2;
      *(float *)(iVar1 + 0x218) = (float)(fVar3 * (float10)*(float *)(iVar1 + 0x8c));
      *(float *)(iVar1 + 0x21c) = (float)(fVar3 * (float10)*(float *)(iVar1 + 0x90));
      *(float *)(iVar1 + 0x220) = (float)(fVar3 * (float10)*(float *)(iVar1 + 0x94));
    }
    fVar3 = (float10)fsin(fVar2);
    *(float *)(iVar1 + 0x224) = (float)fVar3;
    fVar2 = (float10)fcos(fVar2);
    *(float *)(iVar1 + 0x228) = (float)fVar2;
    return;
  }
  *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) & 0xfffffffb;
  *(undefined4 *)(iVar1 + 0x224) = 0;
  *(undefined4 *)(iVar1 + 0x228) = 0x3f800000;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
