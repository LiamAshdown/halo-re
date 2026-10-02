// effect_resolve_marker_transform  (Ghidra: FUN_00453220, still unnamed there)
// address 0x453220, size 100 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: types/effects.h effect.object_index (+0x3c) and effect.first_person_weapon_index
//   (+0x4c) match in_ECX+0x3c/+0x4c exactly; types/objects.h object_header (size 0x0c, data
//   pointer at +0x08) and object.nodes (object_block_reference at +0x1f0, its offset sub-field
//   at +0x1f2) match the default-path arithmetic; the caller (object_change_color_evaluate
//   0x4529d0, outside this batch) hands the result straight to matrix4x3_transform_point,
//   confirming the return value is a real_matrix4x3 *.
// register convention: marker value in EAX (in_EAX, an effect_location_marker.marker_index style
//   value: 0xffff means "no marker", bit 0x8000 selects a first-person weapon node), owning
//   effect in ECX (in_ECX).
//   // blam-cc: in_ECX -> self, in_EAX -> marker
// UNSURE: when marker is 0xffff the function still falls through to the object path with a node
//   index of -1, one real_matrix4x3 before the node array's own base -- kept exactly as decoded
//   rather than special-cased, since it is presumably how the engine reaches the object's own
//   root transform.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;              // 0x008603b0
extern uint8_t *first_person_weapon_interfaces; // 0x006b2d98, row stride 0x1ea0; UNSURE of the
                                    // element type, declared as a byte base so the byte offset
                                    // arithmetic below stays exact

real_matrix4x3 *effect_resolve_marker_transform(effect *self, int16_t marker) // blam-cc: ECX, EAX
{
    int16_t node_index = marker;

    if (node_index != (int16_t)0xffff) {
        if ((marker & 0x8000) != 0) {
            uint16_t weapon_node = (uint16_t)marker & 0x7fff;
            return (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
                                       self->first_person_weapon_index * 0x1ea0 +
                                       weapon_node * 0x34);
        }
        node_index = (int16_t)((uint16_t)marker & 0x7fff);
    }

    {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        return (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + node_index * 0x34);
    }
}

#if 0
Original Ghidra decompilation (0x453220):

int FUN_00453220(void)

{
  int iVar1;
  ushort uVar2;
  undefined4 in_EAX;
  int in_ECX;

  uVar2 = (ushort)in_EAX;
  if (uVar2 == 0xffff) {
    uVar2 = 0xffff;
  }
  else {
    if ((char)((uint)in_EAX >> 8) < '\0') {
      return (short)(uVar2 & 0x7fff) * 0x34 + 0x108c +
             *(short *)(in_ECX + 0x4c) * 0x1ea0 + DAT_006b2d98;
    }
    uVar2 = uVar2 & 0x7fff;
  }
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(in_ECX + 0x3c) & 0xffff) * 0xc);
  return (int)*(short *)(iVar1 + 0x1f2) + (short)uVar2 * 0x34 + iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
