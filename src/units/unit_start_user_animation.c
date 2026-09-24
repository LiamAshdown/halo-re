// unit_start_user_animation  (Ghidra: already named unit_start_user_animation)
// address 0x5702a0, size 343 bytes
// name confidence: 0.6 (this is the real function of that name: it carries the exact
//   "doesn't exist in the graph" string the cea-pdb hint cites; units_types_notes.md notes the
//   pre-existing name was previously mis-assigned to 0x5739a0, a hover-vehicle physics
//   function, and belongs here instead)
// rewrite confidence: 0.3 -- the animation-name string and the graph tag id arrive in registers
//   Ghidra could not source at all (not even as "unaff_" locals for the string), so
//   animation_graph_find_animation_by_name and console_print_va are called with no arguments modeled,
//   matching Ghidra's own decompile, rather than invented ones.
// evidence: types/objects.h object.animation_index (0x0d0), .animation_frame (0x0d2);
//   types/units.h unit_data.animation_state (0x2a3), .animation_state_flags (0x298, bit 0 =
//   _unit_animation_flag_action_active); callee unit_set_custom_animation (0x56ebd0, this
//   batch), object_recalculate_bounding_radius_recursive.
// register convention: object index in EAX (param_1), a graph tag id in EDI (unaff_EDI), a
//   "warn if missing" flag in a second register (param_2). The animation name string is a third
//   register argument this decompile never names.
//   // blam-cc: EAX -> object_index, EDI -> graph_tag_id, second register -> warn_if_missing
// UNSURE: the "continuation" branch (matching animation type at graph offset 0x42 relative to
//   the two records, and stepping the frame back by one when within 2 frames of the end) is
//   reproduced with raw offsets into the graph's 0xb4-stride animation block, the same
//   unnamed block unit_get_custom_animation_time_remaining reads.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t animation_graph_find_animation_by_name(void); // 0x4d6ab0, UNSURE: real args not visible
extern void console_print_va(const char *format, ...); // 0x4c6920
extern int16_t animation_choose_random_permutation(uint32_t flag); // 0x4d6280, UNSURE: allocates some kind of instance/token
extern void object_copy_default_node_transforms(uint32_t unit_index); // 0x4f6b70  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 1 of 2 args at this call site
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0

// Starts (or validates continuation of) a named custom animation on the unit, switching it into
// custom-animation control state. Returns 1 if the animation was (re)started, 0 if the named
// animation does not exist in the graph, if it is already playing and not near its end (a
// continuation), or if either input index is -1.
uint8_t unit_start_user_animation(uint32_t object_index, datum_index graph_tag_id, uint8_t warn_if_missing)
{
    object *obj;
    uint8_t *graph_tag;
    int16_t animation_index;
    int16_t new_animation_index;

    if (object_index == 0xffffffff || graph_tag_id == 0xffffffff) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    graph_tag = tag_instances[graph_tag_id & 0xffff].data;

    animation_index = animation_graph_find_animation_by_name(); // UNSURE args, see file header
    if (animation_index == -1) {
        console_print_va("the animation '%s' doesn't exist in the graph '%s'");
        return 0;
    }

    new_animation_index = animation_choose_random_permutation(1);
    {
        uint8_t *anim_block = *(uint8_t **)(graph_tag + 0x78);
        int16_t kind = *(int16_t *)(anim_block + new_animation_index * 0xb4 + 0x20);

        if (kind == 1 || kind != 0) {
            return 0;
        }

        if (obj->animation_index != -1 &&
            ((unit_data *)((uint8_t *)obj + k_unit_data_offset))->animation_state == _unit_animation_state_custom_animation) {
            int32_t old_record = obj->animation_index * 0xb4;
            if (*(int16_t *)(anim_block + old_record + 0x42) == *(int16_t *)(anim_block + new_animation_index * 0xb4 + 0x42)) {
                int16_t old_frame_count = *(int16_t *)(anim_block + old_record + 0x34);
                int16_t current_frame = obj->animation_frame;
                if (current_frame + 2 == old_frame_count) {
                    obj->animation_frame = current_frame - 1;
                    return 0;
                }
                if (current_frame < old_frame_count) {
                    return 0;
                }
            }
        }

        if (warn_if_missing != 0) {
            object_copy_default_node_transforms(object_index);
        }
        ((unit_data *)((uint8_t *)obj + k_unit_data_offset))->animation_state = _unit_animation_state_custom_animation;
        unit_set_custom_animation(object_index, graph_tag_id, new_animation_index);
        ((unit_data *)((uint8_t *)obj + k_unit_data_offset))->animation_state_flags |= _unit_animation_flag_action_active;
        object_recalculate_bounding_radius_recursive(object_index);
        return 1;
    }
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
