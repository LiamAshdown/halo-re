// actor_apply_queued_look_to_unit  (Ghidra: actor_apply_queued_look_to_unit, renamed)
// address 0x42a640, size 384 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x42a640..0x42a7bf. The draft passed unit_apply_control_block only a -1: the original
//   builds a unit_control_data on the stack from the actor's queued control fields and hands it over:
//   - animation_state = the byte table 0x6558b8[actor +0x6dc * 2], aiming_speed = +0x6f8, control_flags = +0x6d0,
//     weapon/grenade/zoom = -1, throttle = +0x6e0, primary_trigger = +0x720, facing = +0x6fc, aiming = +0x708,
//     looking = +0x714 (+0x0a is left unset, as in the original);
//   - applied only when the unit has no controlling player (object +0x218) or local_player_globals +0x11 is set;
//     a pending weapon refresh (+0x07) runs first (unit_refresh_targeting_flag_and_weapons(unit, CL = 1));
//   - a queued scripted action (+0x6ec, with +0x6f0) starts; a positive +0x6d4 is written to object +0x210 with
//     +0x6d8 to +0x214.
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *actor_data;          // 0x00880360
extern data_array *object_data;         // 0x008603b0
extern player_globals *local_player_globals; // 0x0087a478
extern const uint8_t actor_control_animation_state_table[]; // 0x006558b8, 2 bytes per entry, the first used

extern void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL
extern void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id); // 0x5639f0, EAX, EDX, stack
extern uint8_t unit_try_start_scripted_action_animation(uint32_t unit_index, int16_t command, const real_vector2d *direction); // 0x569530

void actor_apply_queued_look_to_unit(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint32_t unit_index = *(uint32_t *)&((struct actor *)actor)->unit_index;
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_control_data control;

    control.animation_state = (int8_t)actor_control_animation_state_table[((struct actor *)actor)->control_animation_mode * 2];
    control.aiming_speed = (int8_t)actor[0x6f8];
    control.control_flags = *(uint16_t *)&((struct actor *)actor)->control_flags;
    control.weapon_index = -1;
    control.grenade_index = -1;
    control.zoom_level = -1;
    control.unknown_0a = 0;
    control.throttle = *(real_vector3d *)&((struct actor *)actor)->throttle.i;
    control.primary_trigger = *(float *)&((struct actor *)actor)->override_target;
    control.facing_vector = *(real_vector3d *)&((struct actor *)actor)->snapshot_facing.i;
    control.aiming_vector = *(real_vector3d *)&((struct actor *)actor)->snapshot_unknown_708.i;
    control.looking_vector = *(real_vector3d *)&((struct actor *)actor)->snapshot_unknown_714.i;

    if (*(uint32_t *)&((unit_object *)unit)->unit.controlling_player != 0xffffffff && local_player_globals->input_disabled == 0) {
        return;
    }
    if (actor[0x07] != 0) {
        unit_refresh_targeting_flag_and_weapons(unit_index, 1);
        actor[0x07] = 0;
    }
    unit_apply_control_block(*(uint32_t *)&((struct actor *)actor)->unit_index, &control, -1);
    if (((struct actor *)actor)->control_animation_impulse != -1) {
        unit_try_start_scripted_action_animation(*(uint32_t *)&((struct actor *)actor)->unit_index, ((struct actor *)actor)->control_animation_impulse,
            (const real_vector2d *)(actor + 0x6f0));
    }
    if (((struct actor *)actor)->persistent_control_ticks > 0) {
        uint8_t *object = (uint8_t *)((object_header *)object_data->data)[*(uint32_t *)&((struct actor *)actor)->unit_index & 0xffff].data;

        *(int32_t *)(object + 0x210) = ((struct actor *)actor)->persistent_control_ticks;
        *(uint32_t *)(object + 0x214) = ((struct actor *)actor)->persistent_control_flags;
    }
}

#if 0
Original Ghidra decompilation (0x42a640):

void FUN_0042a640(void)

{
  undefined4 uVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar2 = DAT_008603b0;
  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                        (*(uint *)(iVar3 + 0x18) & 0xffff) * 0xc) + 0x218) == -1) ||
     (*(char *)(DAT_0087a478 + 0x11) != '\0')) {
    if (*(char *)(iVar3 + 7) != '\0') {
      FUN_00569bf0(*(uint *)(iVar3 + 0x18));
      *(undefined1 *)(iVar3 + 7) = 0;
    }
    FUN_005639f0(0xffffffff);
    if (*(short *)(iVar3 + 0x6ec) != -1) {
      FUN_00569530(*(undefined4 *)(iVar3 + 0x18),*(short *)(iVar3 + 0x6ec),iVar3 + 0x6f0);
    }
    if (0 < *(short *)(iVar3 + 0x6d4)) {
      uVar1 = *(undefined4 *)(iVar3 + 0x6d8);
      iVar2 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + (*(uint *)(iVar3 + 0x18) & 0xffff) * 0xc);
      *(int *)(iVar2 + 0x210) = (int)*(short *)(iVar3 + 0x6d4);
      *(undefined4 *)(iVar2 + 0x214) = uVar1;
    }
  }
  return;
}
#endif
