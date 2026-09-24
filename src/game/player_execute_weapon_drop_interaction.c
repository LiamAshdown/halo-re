// player_execute_weapon_drop_interaction  (Ghidra: FUN_004790d0; renamed -- handles pending
// interaction types 6 (drop current weapon) and 7 (re-ready it) for a player, and notifies
// observers)
// address 0x4790d0, size 353 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: types/game.h player::unit/interaction_type/interaction_object/interaction_seat
//   (0x34/0x28/0x24/0x2a); types/units.h unit_data::current_weapon_index/weapons[4]
//   (0x2f2/0x2f8); unit_drop_current_weapon already established; the trailing
//   game_engine_notify_player_interaction(1, interaction_type, interaction_seat, held_weapon) notification matches
//   game_engine_notify_player_interaction's own field shape (this batch).
// register convention: none -- `player_index` is this function's own single stack parameter
//   (Ghidra's own `uint param_1`).
// UNSURE: object+0x04 (network_role, gating the final notification) is read through a second,
//   independently resolved object pointer in the original -- modeled here as the same `obj`
//   already in scope, since both point at the same unit; unit_pickup_weapon/hud_add_item_message/
//   unit_invalidate_local_player_zoom_level's exact effects.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0

extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern uint8_t unit_pickup_weapon(uint8_t is_primary); // 0x56d400, not in this batch
extern void hud_add_item_message(uint32_t a); // 0x4ae400, not in this batch
extern void unit_invalidate_local_player_zoom_level(void); // 0x4726f0, not in this batch
extern void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t mode, uint32_t interaction_type_and_seat,
    int32_t secondary_key); // this batch, 0x478ff0 (game_engine_notify_player_interaction's
    // own Ghidra name)

// blam-cc: stack -> player_index
// For pending interaction 6: remembers the unit's currently-held weapon, drops it (forcing),
// and (if unit_pickup_weapon approves) notifies via hud_add_item_message/unit_invalidate_local_player_zoom_level. For interaction 7:
// just asks unit_pickup_weapon to re-ready and notifies via hud_add_item_message. Either way, if the unit's
// network_role is 0, broadcasts the interaction via game_engine_notify_player_interaction. Returns 1 if interaction 6
// was handled (even if the drop failed), 0 for interaction 7 or anything else.
uint8_t player_execute_weapon_drop_interaction(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    object *obj = (object *)((object_header *)object_headers->data)[p->unit & 0xffff].data;
    uint8_t result = 0;
    datum_index held_weapon = (datum_index)0xffffffff;

    if (p->interaction_type == 6) {
        unit_data *unit = (unit_data *)((object_header *)object_headers->data)[p->unit & 0xffff].data;
        int16_t current_weapon_index = unit->current_weapon_index;
        uint8_t handled;

        if (current_weapon_index != -1) {
            held_weapon = unit->weapons[current_weapon_index];
        }

        if (unit_drop_current_weapon((uint32_t)p->unit, 1) != 0 && unit_pickup_weapon(1) != 0) {
            hud_add_item_message(0);
            unit_invalidate_local_player_zoom_level();
            handled = 1;
        } else {
            handled = 0;
        }

        result = 1;
        if (!handled) {
            return 1;
        }
    } else {
        if (p->interaction_type != 7) {
            return 0;
        }
        if (unit_pickup_weapon(1) == 0) {
            return 0;
        }
        hud_add_item_message(0);
    }

    if (obj->network_role == 0) {
        game_engine_notify_player_interaction(1, p->interaction_type, p->interaction_seat, (int32_t)held_weapon);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4790d0), from tools/pack.py 0x4790d0:

undefined1 FUN_004790d0(uint param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  bool bVar6;
  undefined1 uVar7;
  int iVar8;
  undefined4 uVar9;

  iVar8 = (param_1 & 0xffff) * 0x200;
  uVar2 = *(uint *)(iVar8 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  iVar8 = iVar8 + *(int *)(DAT_0087a480 + 0x34);
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
  uVar7 = 0;
  uVar9 = 0xffffffff;
  if (*(short *)(iVar8 + 0x28) == 6) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar8 + 0x34) & 0xffff) * 0xc);
    sVar1 = *(short *)(iVar4 + 0x2f2);
    uVar9 = 0xffffffff;
    if (sVar1 != -1) {
      uVar9 = *(undefined4 *)(iVar4 + 0x2f8 + sVar1 * 4);
    }
    cVar5 = unit_drop_current_weapon(uVar2,1);
    if ((cVar5 == '\0') || (cVar5 = FUN_0056d400(1), cVar5 == '\0')) {
      bVar6 = false;
    }
    else {
      FUN_004ae400(0);
      FUN_004726f0();
      bVar6 = true;
    }
    uVar7 = 1;
    if (!bVar6) {
      return 1;
    }
  }
  else {
    if (*(short *)(iVar8 + 0x28) != 7) {
      return 0;
    }
    cVar5 = FUN_0056d400(1);
    if (cVar5 == '\0') {
      return 0;
    }
    FUN_004ae400(0);
  }
  if (*(int *)(iVar3 + 4) == 0) {
    FUN_00478ff0(1,*(undefined2 *)(iVar8 + 0x28),*(undefined2 *)(iVar8 + 0x2a),uVar9);
  }
  return uVar7;
}
#endif
