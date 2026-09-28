// unit_try_start_scripted_action_animation  (Ghidra: FUN_00569530)
// address 0x569530, size 319 bytes, name confidence 0.4, rewrite confidence 0.9 (REWRITTEN from objdump)
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

extern uint8_t unit_is_seat_control_available(uint32_t unit_index, int16_t command); // 0x5693a0, EAX, DI
extern int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority); // 0x5692b0, CX, EDX
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, EAX, DX
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack
extern void unit_set_throw_aim_direction(uint32_t object_index, const real_vector2d *direction_xy); // 0x5704d0, EAX, ECX

// REWRITTEN from objdump 0x569530..0x56966e. Stack: (unit, command, direction). When the unit may act and its current
//   animation set (graph +0x10 units, +0x5c weapons, 0xbc each; +0x98 / +0x9c states) has the command's state
//   (0x5692b0, which also yields a priority), the node transforms are reset with that priority (0x4f6b70), a random
//   permutation of the state's animation is picked (0x4d6280: graph, animation, stream 1) and played as the object's
//   animation (+0xcc / +0xd0 / +0xd2), the unit enters state 0x1d (+0x2a3, flag +0x298 bit 0) and a free biped turns
//   toward the direction. The draft lost the priority and called the permutation chooser with only the stream, so
//   the object got a garbage animation index.
uint8_t unit_try_start_scripted_action_animation(uint32_t unit_index, int16_t command, const real_vector2d *direction)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;     // esi
    uint8_t *unit_tag;                                                                              // edi
    uint8_t *weapon_record;                                                                         // ebp
    int16_t priority;                                                                               // [esp+0x10]
    int16_t state_index;
    int16_t first_animation;
    int16_t animation;
    uint8_t *object;

    if (!unit_is_seat_control_available(unit_index, command)) {
        return 0;
    }
    unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    {
        uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)(unit_tag + 0x44) & 0xffff].data;
        uint8_t *units_block = *(uint8_t **)(graph + 0x10);
        uint8_t *weapons = *(uint8_t **)(units_block + (int8_t)unit[0x2a0] * 0x64 + 0x5c);

        weapon_record = weapons + (int8_t)unit[0x2a1] * 0xbc;
    }
    state_index = (int16_t)unit_map_action_command_to_animation_state(command, &priority);
    if (state_index < 0 || (int32_t)state_index >= *(int32_t *)(weapon_record + 0x98)) {
        return 0;
    }
    first_animation = (*(int16_t **)(weapon_record + 0x9c))[state_index];
    if (first_animation == -1) {
        return 0;
    }
    object_copy_default_node_transforms(unit_index, priority);
    animation = animation_choose_random_permutation(*(datum_index *)(unit_tag + 0x44), first_animation, 1);
    object = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    *(datum_index *)(object + 0xcc) = *(datum_index *)(unit_tag + 0x44);
    *(int16_t *)(object + 0xd0) = animation;
    *(int16_t *)(object + 0xd2) = 0;
    unit[0x298] |= 1;
    unit[0x2a3] = 0x1d;
    if (direction != 0 && ((unit_object *)unit)->base.type == 0 && ((unit_object *)unit)->base.parent_object == k_datum_index_none) {
        unit_set_throw_aim_direction(unit_index, direction);
    }
    return 1;
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
