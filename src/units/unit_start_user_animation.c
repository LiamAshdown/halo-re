// unit_start_user_animation  (Ghidra: already named unit_start_user_animation)
// address 0x5702a0, size 343 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// REWRITTEN (objdump 0x5702a0..0x5703f6; the draft had the unit in EAX and no animation name). EAX = the
//   animation name, EDI = the animation graph tag, [esp+4] = the unit, [esp+8] = interpolate. With both indices
//   set: animation_graph_find_animation_by_name(EAX graph, EBX name) -- missing prints "the animation '%s'
//   doesn't exist in the graph '%s'" (0x0066e960, the name and the graph's tag name) and fails -- then
//   animation_choose_random_permutation(EAX graph, DX index, stack 1). An animation whose type word (+0x20 of the
//   0xb4-byte record at graph +0x78) is not 0 fails. A unit already in a custom animation (+0x2a3 == 0x1c,
//   animation +0xd0 not -1) of the same +0x42 group is left alone -- two frames (+0xd2) from its end
//   (+0x34 frame count) it steps back one frame; before the end it fails. Otherwise: interpolate copies the
//   default node transforms (EAX unit, DX 6), the state becomes 0x1c, unit_set_custom_animation(EAX unit,
//   stack graph, animation), the action-active bit (+0x298 bit 0) is set, the bounding radius is recomputed.
// blam-cc: stack -> unit_index, EDI -> graph_tag, EAX -> animation_name, stack -> interpolate

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

extern int16_t animation_graph_find_animation_by_name(datum_index animation_graph_tag, const char *name);
    // 0x4d6ab0, blam-cc: EAX, EBX
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, blam-cc: EAX, DX, stack
extern void console_print_va(const char *format, ...); // 0x4c6920
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, EAX, DX
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0

uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name,
    uint8_t interpolate)
{
    uint8_t *unit;
    uint8_t *animations;
    uint8_t *record;
    int16_t animation;

    if (unit_index == k_datum_index_none || graph_tag == k_datum_index_none) {
        return 0;
    }
    unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);
    animation = animation_graph_find_animation_by_name(graph_tag, animation_name);
    if (animation == -1) {
        console_print_va("the animation '%s' doesn't exist in the graph '%s'", animation_name,
            *(char **)((uint8_t *)tag_instances + (int16_t)graph_tag * 0x20 + 0x10));
        return 0;
    }
    animation = animation_choose_random_permutation(graph_tag, animation, 1);
    animations = *(uint8_t **)((uint8_t *)tag_instances[graph_tag & 0xffff].data + 0x78);
    record = animations + animation * 0xb4;
    if (*(int16_t *)(record + 0x20) != 0) {
        return 0;
    }
    if (unit[0x2a3] == 0x1c && ((unit_object *)unit)->base.animation_index != -1) {
        uint8_t *current = animations + ((unit_object *)unit)->base.animation_index * 0xb4;

        if (*(int16_t *)(current + 0x42) == *(int16_t *)(record + 0x42)) {
            int16_t frame_count = *(int16_t *)(current + 0x34);
            uint16_t frame = *(uint16_t *)&((unit_object *)unit)->base.animation_frame;

            if ((int32_t)(int16_t)frame + 2 == (int32_t)frame_count) {
                *(uint16_t *)&((unit_object *)unit)->base.animation_frame = (uint16_t)(frame - 1);
                return 0;
            }
            if ((int16_t)frame < frame_count) {
                return 0;
            }
        }
    }
    if (interpolate) {
        object_copy_default_node_transforms(unit_index, 6);
    }
    unit[0x2a3] = 0x1c;
    unit_set_custom_animation(unit_index, graph_tag, animation);
    unit[0x298] |= 1;
    object_recalculate_bounding_radius_recursive(unit_index);
    return 1;
}

#if 0
Original Ghidra decompilation (0x5702a0):

undefined1 unit_start_user_animation(uint param_1,char param_2)

{
  int iVar1;
  int iVar2;
  short sVar3;
  short sVar4;
  int iVar5;
  uint unaff_EDI;

  if ((param_1 != 0xffffffff) && (unaff_EDI != 0xffffffff)) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    iVar2 = *(int *)((unaff_EDI & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar3 = model_get_region_index_by_name();
    if (sVar3 == -1) {
      console_print_va("the animation \'%s\' doesn\'t exist in the graph \'%s\'");
    }
    else {
      sVar4 = FUN_004d6280(1);
      iVar2 = *(int *)(iVar2 + 0x78);
      sVar3 = *(short *)(sVar4 * 0xb4 + 0x20 + iVar2);
      if ((sVar3 != 1) && (sVar3 == 0)) {
        if ((*(char *)(iVar1 + 0x2a3) == '\x1c') &&
           ((*(short *)(iVar1 + 0xd0) != -1 &&
            (iVar5 = *(short *)(iVar1 + 0xd0) * 0xb4,
            *(short *)(iVar5 + 0x42 + iVar2) == *(short *)(sVar4 * 0xb4 + iVar2 + 0x42))))) {
          sVar3 = *(short *)(iVar5 + iVar2 + 0x34);
          sVar4 = *(short *)(iVar1 + 0xd2);
          if (sVar4 + 2 == (int)sVar3) {
            *(short *)(iVar1 + 0xd2) = sVar4 + -1;
            return 0;
          }
          if (sVar4 < sVar3) {
            return 0;
          }
        }
        if (param_2 != '\0') {
          FUN_004f6b70();
        }
        *(undefined1 *)(iVar1 + 0x2a3) = 0x1c;
        unit_set_custom_animation();
        *(byte *)(iVar1 + 0x298) = *(byte *)(iVar1 + 0x298) | 1;
        object_recalculate_bounding_radius_recursive(param_1);
        return 1;
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
