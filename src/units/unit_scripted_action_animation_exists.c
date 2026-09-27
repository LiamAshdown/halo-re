// unit_scripted_action_animation_exists  (Ghidra: FUN_00569470)
// address 0x569470, size 184 bytes, name confidence 0.35, rewrite confidence: 0.85
// VERIFIED 2026-09-27 (static loop) against objdump 0x569470..0x569527: unit_data fields are absolute object offsets
// (0x2a0 / 0x2a1); EDX to 0x5692b0 is only a scratch priority out (NULL allowed).
// functions.md: "Checks whether the animation corresponding to a given scripted action currently
// exists for the unit's type."
// evidence: types/units.h unit_data.animation_weapon_index (0x2a1), .animation_definition_index
//   (0x2a0); types/tags.h Object.animation_graph (TagID at 0x44); the graph-internal offsets
//   (ModelAnimations.units pointer at +0x10, the per-seat weapons array pointer at +0x5c stride
//   100, ModelAnimationsAnimationGraphWeapon stride 0xbc with an [count,pointer] pair at +0x98)
//   match src/units/unit_set_or_test_seat_and_weapon_label.c's documented graph layout closely
//   enough to reuse the same offsets, but no header struct names them, so they stay raw.
// blam-cc: EAX -> unit_index, ECX -> command. command is forwarded to
//   unit_is_seat_control_available and unit_map_action_command_to_animation_state -- not visible
//   as a parameter in this decompilation, but required by both callees, so it is modelled as an
//   explicit parameter.
// UNSURE: the exact meaning of the "count"/"array" pair at weapon+0x98/0x9c.
// FIXED (register inputs, objdump): ECX (read at 0x56947d, "mov edi,ecx") carries command; the
// note described it in prose ("unaff (command) forwarded") instead of a parseable "ECX ->
// command" mapping, so it looked unclaimed. Body already used command correctly.

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

uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command) // blam-cc: EAX -> unit_index, ECX -> command
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (!unit_is_seat_control_available(unit_index, command)) {
        return 0;
    }

    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[unit_tag->base.animation_graph.tag_id.index & 0xffff].data;
    uint8_t *units_block = *(uint8_t **)(graph + 0x10);
    int8_t seat_block_index = ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->animation_definition_index; // animation_definition_index
    uint8_t *weapons_array = *(uint8_t **)(units_block + 0x5c + seat_block_index * 100);
    int8_t weapon_index = ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->animation_weapon_index; // animation_weapon_index
    uint8_t *weapon_record = weapons_array + weapon_index * 0xbc;

    int16_t state_index = (int16_t)unit_map_action_command_to_animation_state(command, (int16_t *)0);
    if ((-1 < state_index) && (state_index < *(int32_t *)(weapon_record + 0x98))) {
        int16_t animation = *(int16_t *)(*(int32_t *)(weapon_record + 0x9c) + state_index * 2);
        return animation != -1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x569470):

uint FUN_00569470(void)

{
  short sVar1;
  uint *puVar2;
  short sVar3;
  uint in_EAX;
  uint uVar4;
  int iVar5;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar4 = FUN_005693a0();
  if ((char)uVar4 == '\0') {
    return uVar4 & 0xffffff00;
  }
  iVar5 = *(char *)((int)puVar2 + 0x2a1) * 0xbc +
          *(int *)(*(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 0x44) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 0x10) + 0x5c + (char)puVar2[0xa8] * 100)
  ;
  sVar3 = FUN_005692b0();
  if ((-1 < sVar3) && ((int)sVar3 < *(int *)(iVar5 + 0x98))) {
    sVar1 = *(short *)(*(int *)(iVar5 + 0x9c) + sVar3 * 2);
    return CONCAT31((int3)(CONCAT22(sVar3 >> 0xf,sVar1) >> 8),sVar1 != -1);
  }
  return 0xffffff00;
}
#endif
