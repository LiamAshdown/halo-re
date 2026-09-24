// unit_try_ready_weapon  (Ghidra: FUN_00569a20)
// address 0x569a20, size 238 bytes, name confidence 0.35, rewrite confidence 0.4
// functions.md: "Checks whether the unit's current weapon animation mode allows a state change
// and, if so, calls FUN_00565f90 and unit_set_throw_aim_direction and updates the weapon-mode fields."
// evidence: types/units.h unit_data.animation_state (0x2a3, _unit_animation_state_ready_weapon
//   = 0x19 set here per the enum comment), .melee_state (0x289), .melee_damage_countdown (0x28a);
//   types/tags.h UnitFlags melee_attack_is_fatal (0x100).
// blam-cc: unaff_EDI -> unit_index, param_1 -> is_melee, param_2 -> fire_trigger_event.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_set_throw_aim_direction(uint32_t unit_index); // 0x5704d0, UNSURE signature  // real signature (unit_set_throw_aim_direction.c): void unit_set_throw_aim_direction(uint32_t object_index, float direction_x, float direction_y); Ghidra recovered 1 of 3 args at this call site

uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t is_melee, int32_t fire_trigger_event) // blam-cc: unaff_EDI, param_1, param_2
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    uint8_t result = 0;

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        break;
    default:
        if ((unit_try_set_animation_state(unit_index, 0x19) != 0) || is_melee) {
            if ((unit_tag->unit_flags & 0x100) != 0) {
                unit->animation_state = 0x19;
            }
            if (fire_trigger_event != 0) {
                unit_set_throw_aim_direction(unit_index);
            }
            if (is_melee) {
                unit->melee_state = 4;
                unit->melee_damage_countdown = 0;
                return 1;
            }
            unit->melee_state = 1;
            result = 1;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x569a20):

undefined1 FUN_00569a20(char param_1,int param_2)

{
  uint *puVar1;
  int iVar2;
  char cVar3;
  undefined1 uVar4;
  uint unaff_EDI;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar4 = 0;
  switch(*(undefined1 *)((int)puVar1 + 0x2a3)) {
  case 0x17:
  case 0x18:
  case 0x19:
  case 0x1a:
  case 0x1b:
  case 0x1d:
  case 0x1e:
  case 0x1f:
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x27:
  case 0x29:
    break;
  default:
    cVar3 = unit_try_set_animation_state();
    if ((cVar3 != '\0') || (param_1 != '\0')) {
      if ((*(uint *)(iVar2 + 0x17c) & 0x100) != 0) {
        *(undefined1 *)((int)puVar1 + 0x2a3) = 0x19;
      }
      if (param_2 != 0) {
        FUN_005704d0();
      }
      if (param_1 != '\0') {
        *(undefined1 *)((int)puVar1 + 0x289) = 4;
        *(undefined1 *)((int)puVar1 + 0x28a) = 0;
        return 1;
      }
      *(undefined1 *)((int)puVar1 + 0x289) = 1;
      uVar4 = 1;
    }
  }
  return uVar4;
}
#endif
