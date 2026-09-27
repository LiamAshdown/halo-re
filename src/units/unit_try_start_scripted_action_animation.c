// unit_try_start_scripted_action_animation  (Ghidra: FUN_00569530)
// address 0x569530, size 319 bytes, name confidence 0.4, rewrite confidence 0.3
// functions.md: "Starts a unit's scripted action animation (e.g. melee/grenade-throw class) if
// one exists for the requested action."
// evidence: same graph traversal as unit_scripted_action_animation_exists.c (0x569470);
//   types/units.h unit_data.animation_state_flags (0x298, bit 0x1 = action_active),
//   .animation_state (0x2a3, 0x1d = scripted_action).
// blam-cc: param_1 -> unit_index, param_2 -> command (forwarded), param_3 -> direction (a real_vector2d pointer, ECX to unit_set_throw_aim_direction).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_is_seat_control_available(uint32_t unit_index, int16_t command); // 0x5693a0
extern int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority); // 0x5692b0
extern void object_copy_default_node_transforms(uint32_t unit_index);                          // 0x4f6b70, UNSURE signature  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 1 of 2 args at this call site
extern int32_t animation_choose_random_permutation(int32_t mode);                              // 0x4d6280
extern void unit_set_throw_aim_direction(uint32_t object_index, const real_vector2d *direction_xy); // 0x5704d0, EAX, ECX

uint8_t unit_try_start_scripted_action_animation(uint32_t unit_index, int16_t command, const real_vector2d *direction)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (!unit_is_seat_control_available(unit_index, command)) {
        return 0;
    }

    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[unit_tag->base.animation_graph.tag_id.index & 0xffff].data;
    uint8_t *units_block = *(uint8_t **)(graph + 0x10);
    uint8_t *weapons_array = *(uint8_t **)(units_block + 0x5c + unit->animation_definition_index * 100);
    uint8_t *weapon_record = weapons_array + unit->animation_weapon_index * 0xbc;

    int16_t state_index = (int16_t)unit_map_action_command_to_animation_state(command, (int16_t *)0);
    if ((-1 < state_index) && (state_index < *(int32_t *)(weapon_record + 0x98)) &&
        (*(int16_t *)(*(int32_t *)(weapon_record + 0x9c) + state_index * 2) != -1)) {
        object_copy_default_node_transforms(unit_index);
        int16_t animation = (int16_t)animation_choose_random_permutation(1);
        unit->animation_state_flags |= 1;
        unit->animation_state = 0x1d;
        // NOTE: the original writes the animation graph TagID and the chosen animation index
        // directly into the object at +0xcc/+0xd0/+0xd2, i.e. object.animation_graph /
        // .animation_index / .animation_frame, matching unit_set_custom_animation's own body.
        unit_obj->animation_graph = *(datum_index *)&unit_tag->base.animation_graph.tag_id;
        unit_obj->animation_index = animation;
        unit_obj->animation_frame = 0;
        if ((direction != 0) && (unit_obj->type == _object_type_biped) && (unit_obj->parent_object == k_datum_index_none)) {
            unit_set_throw_aim_direction(unit_index, direction); // 0x569623: ECX = the third argument
        }
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x569530):

undefined4 FUN_00569530(uint param_1,undefined4 param_2,int param_3)

{
  uint *puVar1;
  int iVar2;
  char cVar3;
  short sVar4;
  undefined2 uVar5;
  int iVar6;
  int iVar7;

  iVar6 = (param_1 & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  cVar3 = FUN_005693a0();
  if (cVar3 == '\0') {
    return 0;
  }
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar7 = *(char *)((int)puVar1 + 0x2a1) * 0xbc +
          *(int *)(*(int *)(*(int *)((*(uint *)(iVar2 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                    ) + 0x10) + 0x5c + (char)puVar1[0xa8] * 100);
  sVar4 = FUN_005692b0();
  if (((-1 < sVar4) && ((int)sVar4 < *(int *)(iVar7 + 0x98))) &&
     (*(short *)(*(int *)(iVar7 + 0x9c) + sVar4 * 2) != -1)) {
    FUN_004f6b70();
    uVar5 = FUN_004d6280(1);
    iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
    *(undefined4 *)(iVar6 + 0xcc) = *(undefined4 *)(iVar2 + 0x44);
    *(undefined2 *)(iVar6 + 0xd0) = uVar5;
    *(undefined2 *)(iVar6 + 0xd2) = 0;
    *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 1;
    *(undefined1 *)((int)puVar1 + 0x2a3) = 0x1d;
    if (((param_3 != 0) && ((short)puVar1[0x2d] == 0)) && (puVar1[0x47] == 0xffffffff)) {
      FUN_005704d0();
    }
    return 1;
  }
  return 0;
}
#endif
