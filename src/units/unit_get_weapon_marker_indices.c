// unit_get_weapon_marker_indices  (Ghidra: unit_get_weapon_marker_indices)
// address 0x5642c0, size 200 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.35
// evidence: types/units.h unit_data.animation_weapon_index/.animation_definition_index
//   (0x2a1/0x2a0); types/objects.h object.definition_tag (0x000), Object.animation_graph
//   (tag+0x44); types/tags.h ModelAnimationsAnimationGraphUnitSeat (0x64, weapons TagReflexive
//   at 0x58), ModelAnimationsAnimationGraphWeapon (0xbc, animations TagReflexive at 0x98),
//   ModelAnimationsAnimation (0xb4, key_frame_index at 0x34, frame_count at 0x22) -- same
//   lookup chain unit_try_set_animation_state (0x565f90) uses.
// register convention: unit index in ECX, a reload flag in AL, two pass-through values on the
//   stack, and two output pointers carried in EBX/EDI rather than pushed.
//   // blam-cc: in_ECX -> unit_index, in_AL -> use_alternate, param_1/param_2 -> forwarded to
//   //   animation_get_frame_info_distance, unaff_EBX -> out_frame_count, unaff_EDI -> out_key_frame_index
// UNSURE: animation_get_frame_info_distance's real purpose and argument types are unknown (no strings/callees);
//   out_frame_count/out_key_frame_index are modelled as explicit pointer parameters since
//   Ghidra shows them as bare unresolved registers.

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

extern void animation_get_frame_info_distance(ModelAnimationsAnimation *animation, float *dx_to_key_frame, float *dx_total); // 0x4d4850, ECX, stack

uint8_t unit_get_weapon_marker_indices(uint32_t unit_index, uint8_t use_alternate, uint32_t out_dx_to_key_frame,
                                        uint32_t out_dx_total, int16_t *out_frame_count,
                                        int16_t *out_key_frame_index) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer;
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);

    int32_t raw_index = (use_alternate ? 3 : 0) + 0x27;
    int16_t animation_index = -1;
    if (raw_index < (int32_t)weapon_anim->animations.count) {
        animation_index = *(int16_t *)((uint8_t *)weapon_anim->animations.pointer + raw_index * 2);
    }
    if (animation_index == -1) {
        return 0;
    }

    uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
    ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + animation_index * 0xb4);

    animation_get_frame_info_distance(anim, (float *)out_dx_to_key_frame, (float *)out_dx_total); // 0x56435e: ECX = the animation

    if (out_key_frame_index != 0) {
        *out_key_frame_index = anim->key_frame_index;
    }
    if (out_frame_count != 0) {
        *out_frame_count = anim->frame_count;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5642c0):

undefined4 FUN_005642c0(undefined4 param_1,undefined4 param_2)

{
  uint *puVar1;
  char in_AL;
  short sVar2;
  int iVar3;
  uint in_ECX;
  int iVar4;
  undefined2 *unaff_EBX;
  int iVar5;
  undefined2 *unaff_EDI;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  iVar5 = *(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x44) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(char *)((int)puVar1 + 0x2a1) * 0xbc +
          *(int *)((char)puVar1[0xa8] * 100 + 0x5c + *(int *)(iVar5 + 0x10));
  iVar3 = (-(uint)(in_AL != '\0') & 3) + 0x27;
  if (iVar3 < *(int *)(iVar4 + 0x98)) {
    sVar2 = *(short *)(*(int *)(iVar4 + 0x9c) + iVar3 * 2);
  }
  else {
    sVar2 = -1;
  }
  if (sVar2 == -1) {
    return 0;
  }
  iVar5 = sVar2 * 0xb4 + *(int *)(iVar5 + 0x78);
  FUN_004d4850(param_1,param_2);
  if (unaff_EDI != (undefined2 *)0x0) {
    *unaff_EDI = *(undefined2 *)(iVar5 + 0x34);
  }
  if (unaff_EBX != (undefined2 *)0x0) {
    *unaff_EBX = *(undefined2 *)(iVar5 + 0x22);
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
