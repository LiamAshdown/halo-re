// object_get_center_of_mass_and_scale
// address 0x4088e0, size 59 bytes
// name confidence: 0.6 (out/phase4/objects_types_notes.md cites this function by this name as
// the proof for object.bounding_center at 0xa0 and object.bounding_radius at 0xac)
// rewrite confidence: 0.85
// evidence: types/objects.h object_header (data_array element, stride 0x0c, object* at +0x08)
// and object.bounding_center/bounding_radius (0xa0..0xac); types/memory.h data_array.data at
// +0x34.
// register convention: real_point3d *out_center in EAX (in_EAX), object index in ECX
// (in_ECX, low 16 bits used), float *out_radius on the stack (param_1).
// blam-cc: EAX=out_center, ECX=object_index, stack=out_radius

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

void object_get_center_of_mass_and_scale(real_point3d *out_center, uint32_t object_index, float *out_radius)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    *out_center = obj->bounding_center;
    *out_radius = obj->bounding_radius;
}

#if 0
Original Ghidra decompilation (0x4088e0):

void FUN_004088e0(undefined4 *param_1)

{
  int iVar1;
  undefined4 *in_EAX;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  *in_EAX = *(undefined4 *)(iVar1 + 0xa0);
  in_EAX[1] = *(undefined4 *)(iVar1 + 0xa4);
  in_EAX[2] = *(undefined4 *)(iVar1 + 0xa8);
  *param_1 = *(undefined4 *)(iVar1 + 0xac);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
