// unit_get_animation_frames_remaining  (Ghidra: unit_get_animation_frames_remaining)
// address 0x564390, size 94 bytes
// name confidence: 0.4 (phase2 candidate)   rewrite confidence: 0.5
// evidence: types/objects.h object.animation_graph/animation_index/animation_frame (0xcc/0xd0/
//   0xd2); types/tags.h ModelAnimationsAnimation (stride 0xb4, frame_count at +0x22) -- same
//   chain as object_animation_get_frames_remaining.c and unit_apply_scale_change.c, but reached
//   via the object's own runtime animation_graph (0xcc) rather than the tag's; types/units.h
//   unit_data.animation_state (0x2a3).
// register convention: unit index in EAX, an output pointer for the current animation_state in
//   the recognized parameter.
//   // blam-cc: in_EAX -> unit_index, param_1 -> out_animation_state

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

int32_t unit_get_animation_frames_remaining(uint32_t unit_index, int16_t *out_animation_state) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    void *graph = tag_instances[obj->animation_graph & 0xffff].data;
    uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
    ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + obj->animation_index * 0xb4);

    *out_animation_state = unit->animation_state;
    return (int32_t)anim->frame_count - (int32_t)obj->animation_frame;
}

#if 0
Original Ghidra decompilation (0x564390):

int FUN_00564390(short *param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar2 + 0xd0);
  iVar3 = *(int *)(*(int *)((*(uint *)(iVar2 + 0xcc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x78)
  ;
  *param_1 = (short)*(char *)(iVar2 + 0x2a3);
  return (int)*(short *)(sVar1 * 0xb4 + iVar3 + 0x22) - (int)*(short *)(iVar2 + 0xd2);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
