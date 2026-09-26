// unit_start_seat_overlay_animation_a  (Ghidra: unit_start_seat_overlay_animation_a)
// address 0x565e00, size 349 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.35
// evidence: types/units.h unit_data.overlays[0] (0x2aa), .unknown_2a4 (0x2a4),
//   .animation_definition_index/.animation_weapon_index/.animation_weapon_type_index (0x2a0/
//   0x2a1/0x2a2); types/objects.h object.definition_tag (0x000), Object.animation_graph
//   (tag+0x44); types/tags.h ModelAnimationsAnimationGraphUnitSeat (weapons at 0x58),
//   ModelAnimationsAnimationGraphWeapon (animations TagReflexive at 0x98, weapon_types
//   TagReflexive at 0xb0), ModelAnimationsAnimationGraphWeaponType (0x3c, animations
//   TagReflexive at 0x30) -- same lookup chain as unit_try_set_animation_state, one level
//   deeper for the command values that resolve through the weapon-type table instead of the
//   weapon table.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_copy_default_node_transforms(void);              // 0x4f6b70  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 0 of 2 args at this call site
extern int16_t animation_choose_random_permutation(uint32_t flag);  // 0x4d6280

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX; unit_index arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> unit_index, command
void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (command == 0) {
        unit->unknown_2a4 = 0;
        unit->overlays[0].animation_index = -1;
        return;
    }

    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph + 0x10);
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);
    ModelAnimationsAnimationGraphWeaponType *weapon_type =
        (ModelAnimationsAnimationGraphWeaponType *)((uint8_t *)weapon_anim->weapon_types.pointer +
                                                      unit->animation_weapon_type_index * 0x3c);

    int16_t raw_index = -1;
    uint8_t via_weapon_type = 0;

    switch (command) {
    case 1: raw_index = 0x15; break;
    case 2: raw_index = 0x16; break;
    case 3: raw_index = 0x17; break;
    case 4: raw_index = 0x18; break;
    case 5: raw_index = 0; via_weapon_type = 1; break;
    case 6: raw_index = 1; via_weapon_type = 1; break;
    case 7: raw_index = 8; via_weapon_type = 1; break;
    case 8: raw_index = 0x14; break;
    case 9: raw_index = 9; via_weapon_type = 1; break;
    default: raw_index = -2; break; // sentinel: skip the whole lookup below
    }

    int16_t animation_index = -1;
    if (raw_index != -2) {
        if (via_weapon_type) {
            if (raw_index < (int32_t)weapon_type->animations.count) {
                animation_index = *(int16_t *)((uint8_t *)weapon_type->animations.pointer + raw_index * 2);
            }
        } else if (raw_index < (int32_t)weapon_anim->animations.count) {
            animation_index = *(int16_t *)((uint8_t *)weapon_anim->animations.pointer + raw_index * 2);
        }
    }

    if (animation_index != -1) {
        if (command != 7) {
            object_copy_default_node_transforms();
        }
        unit->overlays[0].animation_index = animation_choose_random_permutation(1);
        unit->overlays[0].frame = 0;
        unit->unknown_2a4 = (int8_t)command;
    }
}

#if 0
Original Ghidra decompilation (0x565e00):

void FUN_00565e00(uint param_1,short param_2)

{
  uint *puVar1;
  undefined2 uVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  int iVar6;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (param_2 == 0) {
    *(undefined1 *)(puVar1 + 0xa9) = 0;
    *(undefined2 *)((int)puVar1 + 0x2aa) = 0xffff;
    return;
  }
  iVar6 = *(int *)(*(int *)(*(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 0x44) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 0x10) + 0x5c + (char)puVar1[0xa8] * 100)
  ;
  iVar3 = *(char *)((int)puVar1 + 0x2a1) * 0xbc;
  iVar4 = iVar3 + iVar6;
  iVar6 = *(char *)((int)puVar1 + 0x2a2) * 0x3c + *(int *)(iVar3 + 0xb4 + iVar6);
  sVar5 = -1;
  switch((int)param_2) {
  case 1:
    sVar5 = 0x15;
    break;
  case 2:
    sVar5 = 0x16;
    break;
  case 3:
    sVar5 = 0x17;
    break;
  case 4:
    sVar5 = 0x18;
    break;
  case 5:
    sVar5 = 0;
    goto LAB_00565ef7;
  case 6:
    sVar5 = 1;
    goto LAB_00565ef7;
  case 7:
    sVar5 = 8;
    goto LAB_00565ef7;
  case 8:
    sVar5 = 0x14;
    break;
  case 9:
    sVar5 = 9;
LAB_00565ef7:
    if ((int)sVar5 < *(int *)(iVar6 + 0x30)) {
      sVar5 = *(short *)(*(int *)(iVar6 + 0x34) + sVar5 * 2);
      goto switchD_00565e9f_default;
    }
    goto LAB_00565f0a;
  default:
    goto switchD_00565e9f_default;
  }
  if ((int)sVar5 < *(int *)(iVar4 + 0x98)) {
    sVar5 = *(short *)(*(int *)(iVar4 + 0x9c) + sVar5 * 2);
  }
  else {
LAB_00565f0a:
    sVar5 = -1;
  }
switchD_00565e9f_default:
  if (sVar5 != -1) {
    if (((param_2 == 7) - 1U & 6) != 0) {
      FUN_004f6b70();
    }
    uVar2 = FUN_004d6280(1);
    *(undefined2 *)((int)puVar1 + 0x2aa) = uVar2;
    *(undefined2 *)(puVar1 + 0xab) = 0;
    *(undefined1 *)(puVar1 + 0xa9) = (undefined1)param_2;
  }
  return;
}
#endif
