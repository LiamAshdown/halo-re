// player_release_unit_and_reset  (Ghidra: FUN_004760b0; named per this rewrite)
// address 0x4760b0, size 408 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Synchronizes light-attachment state on a player
//   unit's current and former parent objects after a parenting change and resets the player's
//   local-player struct"); types/game.h player_globals::local_player_units (+0x08),
//   player::previous_unit (+0x38), player::local_player_index (+0x02); types/units.h unit_data
//   (controlling_player +0x218, current_weapon_index +0x2f2, weapons[] +0x2f8);
//   game_engine_attribute_player_death.c / player_reset_after_unit_change.c (this module) for
//   the two forwarded calls' signatures. objdump -d -M intel --start-address=0x4760b0
//   --stop-address=0x476248 bin/halo.exe confirms player_index arrives in EAX and
//   previous_unit_override is a genuine second, stack, parameter -- both dropped by Ghidra's own
//   "void FUN_004760b0(int param_1)" rendering, which shows only the stack one.
// register convention: EAX -> player_index, stack -> previous_unit_override.
//   // blam-cc: EAX -> player_index, stack -> previous_unit_override
//
// CORRECTED: this function's callers in this batch (0x475270, 0x4757b0, 0x475c60) all show
// Ghidra calling it with a single visible argument; the real call also needs EAX, which by that
// point in each caller still holds the player_index parameter unchanged. Those three files were
// written/reviewed alongside this one so their calls already pass both arguments.
// UNSURE: the weapon-tag-data "+0x34 != -1" test mirrors the same raw offset already left
// UNSURE in the sibling attach functions in this batch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0
extern tag_instance *tag_instances;          // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value;    // 0x0087aa10

extern void game_engine_attribute_player_death(datum_index victim_unit, datum_index killer,
    datum_index death_object, int32_t killer_team, char credit_kills); // 0x46ff00
extern void player_reset_after_unit_change(uint32_t player_index); // this batch, 0x474e10
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
                                              int32_t invoke_callback); // 0x4f9a20

// If player_index's player has a unit: attributes a death for it (crediting kills, unless a
// multiplayer game is already over), stashes the unit handle into
// player_globals::local_player_units[local_player_index], resets the player's per-tick state
// (player_reset_after_unit_change, which clears player::unit), then re-reads the stashed handle
// to clear its controlling_player, clear object_header flag bit 0 and set object.flags bit 0
// (propagating light attachments first if needed) on both the unit and its current weapon (if
// any). Finally, if previous_unit_override is not the wildcard, overwrites
// player::previous_unit with it, and clears player_globals::no_player_has_a_unit.
void player_release_unit_and_reset(uint32_t player_index, int32_t previous_unit_override)
    // blam-cc: EAX -> player_index, stack -> previous_unit_override
{
    player *plr;
    datum_index saved_unit;

    plr = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    if (plr->unit == (datum_index)-1) {
        return;
    }

    if (current_game_engine == 0 || game_engine_state_value == _game_engine_state_not_started) {
        game_engine_attribute_player_death(plr->unit, (datum_index)-1, (datum_index)-1, -1, 1);
    }

    local_player_globals->local_player_units[plr->local_player_index] = plr->unit;
    player_reset_after_unit_change(player_index); // clears plr->unit

    saved_unit = local_player_globals->local_player_units[plr->local_player_index];
    {
        object_header *unit_header = &((object_header *)object_data->data)[saved_unit & 0xffff];
        object *unit_obj = unit_header->data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        datum_index weapon_handle = (datum_index)-1;

        if (unit->current_weapon_index != -1) {
            weapon_handle = unit->weapons[unit->current_weapon_index];
        }
        unit->controlling_player = (datum_index)-1;

        if ((unit_header->flags & 1) != 0) {
            unit_header->flags = unit_header->flags & ~1;
        }
        {
            uint8_t *unit_tag_data = (uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data;
            if (*(int32_t *)(unit_tag_data + 0x34) != -1 && (unit_obj->flags & 1) == 0) {
                object_for_each_light_attachment(1, 0, 0); // UNSURE arg shapes
            }
        }
        unit_obj->flags = unit_obj->flags | 1;
        unit_header->flags = unit_header->flags & ~2;

        if (weapon_handle != (datum_index)-1) {
            object_header *weapon_header = &((object_header *)object_data->data)[weapon_handle & 0xffff];
            object *weapon_obj = weapon_header->data;
            uint8_t *weapon_tag_data = (uint8_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if (*(int32_t *)(weapon_tag_data + 0x34) != -1 && (weapon_obj->flags & 1) == 0) {
                object_for_each_light_attachment(1, 0, 0); // UNSURE arg shapes
            }
            weapon_obj->flags = weapon_obj->flags | 1;
            weapon_header->flags = weapon_header->flags & ~2;
        }
    }

    if (previous_unit_override != -1) {
        plr->previous_unit = (datum_index)previous_unit_override;
    }
    local_player_globals->no_player_has_a_unit = 0;
}

#if 0
Original Ghidra decompilation (0x4760b0), from tools/pack.py 0x4760b0:

void FUN_004760b0(int param_1)

{
  byte *pbVar1;
  byte bVar2;
  uint *puVar3;
  uint in_EAX;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;

  iVar7 = (in_EAX & 0xffff) * 0x200;
  iVar8 = iVar7 + *(int *)(DAT_0087a480 + 0x34);
  if (*(int *)(iVar7 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != -1) {
    if ((DAT_006f1d20 == 0) || (DAT_0087aa10 == 0)) {
      game_engine_attribute_player_death(0xffffffff,0xffffffff,0xffffffff,1);
    }
    *(undefined4 *)(DAT_0087a478 + 8 + *(short *)(iVar8 + 2) * 4) = *(undefined4 *)(iVar8 + 0x34);
    FUN_00474e10();
    iVar4 = DAT_008603b0;
    iVar6 = (*(uint *)(DAT_0087a478 + 8 + *(short *)(iVar8 + 2) * 4) & 0xffff) * 0xc;
    iVar7 = *(int *)(iVar6 + 8 + *(int *)(DAT_008603b0 + 0x34));
    uVar5 = 0xffffffff;
    if (*(short *)(iVar7 + 0x2f2) != -1) {
      uVar5 = *(uint *)(iVar7 + 0x2f8 + *(short *)(iVar7 + 0x2f2) * 4);
    }
    *(undefined4 *)(iVar7 + 0x218) = 0xffffffff;
    bVar2 = *(byte *)(*(int *)(iVar4 + 0x34) + 2 + iVar6);
    if ((bVar2 & 1) != 0) {
      *(byte *)(*(int *)(iVar4 + 0x34) + iVar6 + 2) = bVar2 & 0xfe;
    }
    puVar3 = *(uint **)(iVar6 + 8 + *(int *)(iVar4 + 0x34));
    if ((*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) != -1) &&
       ((puVar3[4] & 1) == 0)) {
      object_for_each_light_attachment(1,0);
      iVar4 = DAT_008603b0;
    }
    iVar6 = *(int *)(iVar4 + 0x34) + iVar6;
    puVar3[4] = puVar3[4] | 1;
    *(byte *)(iVar6 + 2) = *(byte *)(iVar6 + 2) & 0xfd;
    if (uVar5 != 0xffffffff) {
      iVar7 = (uVar5 & 0xffff) * 0xc;
      puVar3 = *(uint **)(iVar7 + 8 + *(int *)(iVar4 + 0x34));
      if ((*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) != -1) &&
         ((puVar3[4] & 1) == 0)) {
        object_for_each_light_attachment(1,0);
        iVar4 = DAT_008603b0;
      }
      iVar4 = *(int *)(iVar4 + 0x34);
      puVar3[4] = puVar3[4] | 1;
      pbVar1 = (byte *)(iVar4 + iVar7 + 2);
      *pbVar1 = *pbVar1 & 0xfd;
    }
    if (param_1 != -1) {
      *(int *)(iVar8 + 0x38) = param_1;
    }
    *(undefined1 *)(DAT_0087a478 + 0x10) = 0;
  }
  return;
}
#endif
