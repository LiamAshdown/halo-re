// weapon_set_state  (Ghidra: weapon_set_state, already named)
// address 0x4c5670, size 381 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (objdump 0x4c5670: permutation EAX graph / DX animation, overlay command EAX
//   parent unit / CX state)
// evidence: types/items.h weapon_state (the exact 11-entry animation-index mapping table is
//   quoted there verbatim); types/objects.h object.animation_graph/animation_index/
//   animation_frame/parent_object; types/tags.h ModelAnimations.weapons (TagReflexive 0x18) ->
//   ModelAnimationsAnimationGraphWeaponAnimations.animations (TagReflexive 0x10) ->
//   ModelAnimationsWeaponAnimation.animation (uint16, one per state). Confirmed against the
//   binary with an offsetof probe against types/tags.h.
// register convention: item index in EAX; new state and the "force" flag are Ghidra-recognized
//   stack parameters.
// blam-cc: EAX -> item_index, stack -> (new_state, force)
// UNSURE: the first `object_try_and_get(parent_object, _object_mask_unit)` call's result is
// discarded entirely; preserved as decompiled rather than dropped as dead code, since its
// purpose (a reference touch/refresh?) is not established. animation_choose_random_permutation and unit_dispatch_seat_overlay_command are
// outside this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX graph, DX animation, stack
                                             // animation index, UNSURE signature
extern void unit_dispatch_seat_overlay_command(uint32_t unit_index, int16_t command); // 0x567400, EAX unit, CX command
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// Transitions a weapon into a new state/animation mode. Refuses the transition (returns 0)
// when not forced, the weapon is mid-fire (state 1 or 2), and the requested state would move it
// backwards; otherwise selects the matching first-person animation permutation from the
// weapon's animation graph (when the tag data for it exists) and notifies the holding unit.
int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force)
{
    object *item_obj;
    weapon_data *wd;
    int16_t current_state;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    current_state = (int8_t)wd->state;
    if (force == 0 && current_state != 0) {
        if (current_state < 1) return 0;
        if (current_state > 2) return 0;
        if (new_state < current_state) return 0;
    }

    {
        Weapon *weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
        datum_index graph_tag_id = *(datum_index *)&weapon_tag->base.base.animation_graph.tag_id;

        if (graph_tag_id != (datum_index)0xffffffff) {
            ModelAnimations *graph = (ModelAnimations *)tag_instances[(uint16_t)graph_tag_id].data;
            if (graph->weapons.count != 0) {
                ModelAnimationsAnimationGraphWeaponAnimations *weapon_anims =
                    (ModelAnimationsAnimationGraphWeaponAnimations *)graph->weapons.pointer;
                if (weapon_anims != 0) {
                    int16_t animation_index;
                    switch (new_state) {
                    case 0: animation_index = 0; break;
                    case 1: animation_index = 9; break;
                    case 2: animation_index = 10; break;
                    case 3: animation_index = 5; break;
                    case 4: animation_index = 6; break;
                    case 5: case 6: animation_index = 3; break;
                    case 7: case 8: animation_index = 8; break;
                    case 9: animation_index = 1; break;
                    case 10: animation_index = 2; break;
                    default: goto skip_animation;
                    }

                    int16_t animation = (animation_index < weapon_anims->animations.count)
                        ? (int16_t)((ModelAnimationsWeaponAnimation *)weapon_anims->animations.pointer)[animation_index].animation
                        : -1;

                    if (animation != -1 || new_state == 0) {
                        // 0x4c5776: EAX = the graph tag, DX = the animation (or -1)
                        item_obj->animation_index = animation_choose_random_permutation(graph_tag_id, animation, 1);
                        item_obj->animation_frame = 0;
                        wd->state = (int8_t)new_state;
                    }
                }
            }
        }
    }
skip_animation:
    {
        // 0x4c57a1: the parent when it is a unit, else none; EAX = that unit, CX = the new state
        datum_index parent = ((object_header *)object_data->data)[(uint16_t)item_index].data->parent_object;
        datum_index unit_index = (datum_index)0xffffffff;

        if (parent != (datum_index)0xffffffff && object_try_and_get(parent, _object_mask_unit) != 0) {
            unit_index = parent;
        }
        if (object_try_and_get(unit_index, _object_mask_unit) != 0) {
            unit_dispatch_seat_overlay_command(unit_index, new_state);
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4c5670):

undefined4 weapon_set_state(short param_1,char param_2)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  undefined2 uVar4;
  uint in_EAX;
  short sVar5;
  int iVar6;

  iVar6 = (in_EAX & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  if ((param_2 == '\0') && (sVar5 = (short)(char)puVar1[0x8e], sVar5 != 0)) {
    if (sVar5 < 1) {
      return 0;
    }
    if (2 < sVar5) {
      return 0;
    }
    if (param_1 < sVar5) {
      return 0;
    }
  }
  uVar2 = *(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x44);
  if (((uVar2 != 0xffffffff) &&
      (iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), *(int *)(iVar3 + 0x18) != 0)
      ) && (iVar3 = *(int *)(iVar3 + 0x1c), iVar3 != 0)) {
    switch(param_1) {
    case 0:
      sVar5 = 0;
      break;
    case 1:
      sVar5 = 9;
      break;
    case 2:
      sVar5 = 10;
      break;
    case 3:
      sVar5 = 5;
      break;
    case 4:
      sVar5 = 6;
      break;
    case 5:
    case 6:
      sVar5 = 3;
      break;
    case 7:
    case 8:
      sVar5 = 8;
      break;
    case 9:
      sVar5 = 1;
      break;
    case 10:
      sVar5 = 2;
      break;
    default:
      goto switchD_004c5719_default;
    }
    if ((((int)sVar5 < *(int *)(iVar3 + 0x10)) &&
        (*(short *)(*(int *)(iVar3 + 0x14) + sVar5 * 2) != -1)) || (param_1 == 0)) {
      uVar4 = FUN_004d6280(1);
      *(undefined2 *)(puVar1 + 0x34) = uVar4;
      *(undefined2 *)((int)puVar1 + 0xd2) = 0;
      *(char *)(puVar1 + 0x8e) = (char)param_1;
    }
  }
switchD_004c5719_default:
  if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6) + 0x11c) != -1) {
    object_try_and_get(3);
  }
  iVar6 = object_try_and_get(3);
  if (iVar6 != 0) {
    FUN_00567400();
  }
  return 1;
}
#endif
