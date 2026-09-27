// player_swap_to_weapon  (Ghidra: FUN_00479240; renamed -- the secondary-mode handler
// game_engine_apply_player_interaction_message.c (this batch) dispatches to)
// address 0x479240, size 351 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: types/game.h player::unit/interaction_type/interaction_object (0x34/0x28/0x24);
//   types/units.h unit_data::current_weapon_index/desired_weapon_index/weapons[4]
//   (0x2f2/0x2f4/0x2f8); unit_drop_current_weapon / unit_ready_desired_weapon already
//   established (src/game/ctf_engine_flag_tick.c and src/units/unit_pick_and_ready_next_weapon.c).
// register convention: a player index in EAX (in_EAX); `target_weapon` is this function's own
//   stack parameter.
//   // blam-cc: EAX -> player_index, stack -> target_weapon
// UNSURE: unit_pickup_weapon/hud_add_item_message/unit_invalidate_local_player_zoom_level's exact effects (all called elsewhere in this
//   module with the same one-visible-argument shape).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0

extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force); // 0x56d6e0, stack (unit, force)
extern uint8_t unit_pickup_weapon(uint8_t is_primary); // 0x56d400, not in this batch
extern void hud_add_item_message(uint32_t a); // 0x4ae400, not in this batch
extern void unit_invalidate_local_player_zoom_level(void); // 0x4726f0, not in this batch

// blam-cc: EAX -> player_index, stack -> target_weapon
// If `player_index`'s pending interaction is 6 (swap weapon), makes `target_weapon` the desired
// weapon (finding it in the unit's own inventory and calling unit_ready_desired_weapon) unless
// it is already the current weapon, then drops the previous current weapon. If the interaction
// is 7 instead, just re-readies the current weapon. Any other interaction type is a no-op.
// Returns 1 (handled) for interaction 6, 0 otherwise.
uint8_t player_swap_to_weapon(uint32_t player_index, datum_index target_weapon)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit_data *unit = (unit_data *)((object_header *)object_headers->data)[p->unit & 0xffff].data;

    if (p->interaction_type != 6) {
        if (p->interaction_type == 7 && unit_pickup_weapon(1) != 0) {
            hud_add_item_message(0);
        }
        return 0;
    }

    {
        int16_t current_weapon_index = unit->current_weapon_index;
        datum_index current_weapon = (datum_index)0xffffffff;
        if (current_weapon_index != -1) {
            current_weapon = unit->weapons[current_weapon_index];
        }

        if (current_weapon != target_weapon) {
            int32_t i;
            for (i = 0; i < 4; i++) {
                if (unit->weapons[i] == target_weapon) {
                    unit->desired_weapon_index = (int16_t)i;
                    unit_ready_desired_weapon((uint32_t)p->unit, 1);
                    break;
                }
            }
        }
    }

    if (unit_drop_current_weapon((uint32_t)p->unit, 1) != 0 && unit_pickup_weapon(1) != 0) {
        hud_add_item_message(0);
        unit_invalidate_local_player_zoom_level();
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x479240), from tools/pack.py 0x479240:

undefined4 FUN_00479240(int param_1)

{
  short sVar1;
  int iVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;

  iVar4 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar4 + 0x34) & 0xffff) * 0xc);
  if (*(short *)(iVar4 + 0x28) != 6) {
    if ((*(short *)(iVar4 + 0x28) == 7) && (cVar3 = FUN_0056d400(1), cVar3 != '\0')) {
      FUN_004ae400(0);
      return 0;
    }
    return 0;
  }
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar4 + 0x34) & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar5 + 0x2f2);
  iVar7 = -1;
  if (sVar1 != -1) {
    iVar7 = *(int *)(iVar5 + 0x2f8 + sVar1 * 4);
  }
  if (iVar7 != param_1) {
    iVar5 = 0;
    piVar6 = (int *)(iVar2 + 0x2f8);
    do {
      if (*piVar6 == param_1) {
        *(short *)(iVar2 + 0x2f4) = (short)iVar5;
        unit_ready_desired_weapon(*(undefined4 *)(iVar4 + 0x34),1);
        break;
      }
      iVar5 = iVar5 + 1;
      piVar6 = piVar6 + 1;
    } while (iVar5 < 4);
  }
  cVar3 = unit_drop_current_weapon(*(undefined4 *)(iVar4 + 0x34),1);
  if ((cVar3 != '\0') && (cVar3 = FUN_0056d400(1), cVar3 != '\0')) {
    FUN_004ae400(0);
    FUN_004726f0();
  }
  return 1;
}
#endif
