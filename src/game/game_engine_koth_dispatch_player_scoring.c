// game_engine_koth_dispatch_player_scoring  (Ghidra: FUN_0046c3e0; named per its summary)
// address 0x46c3e0, size 484 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Dispatches the King-of-the-Hill per-tick scoring for
//   a player across however many hill slots they occupy, applying a configurable score
//   multiplier and periodic sound cues"); game_engine_variant::unknown_84/unknown_8c/unknown_90
//   aliased 0x006f1d0c/0x006f1d14/0x006f1d18; game_engine_variant::ctf_value_80 (0x006f1d08)
//   reused here as a generic 1/2/other score-multiplier selector; king_hill_occupant_table
//   (0x006b120c, this batch); player::unknown_70/74/78/6c (0x70/0x74/0x78/0x6c); types/units.h
//   unit_data.current_weapon_index/weapons[4].
// register convention: player index in the stack parameter (Ghidra's own param_1, also its
//   return value register).
// UNSURE: unit_reset_gauge_if_flagged's identity; weapon_object + 0x2b8 (puVar2 + 0xae as a uint*-scaled
//   offset) does not correspond to a named weapon_data field in types/items.h.

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_variant game_engine_variant; // 0x006f1c88
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value;   // 0x0087aa10
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c, this batch
extern int32_t king_alt_player_score[];       // 0x006b118c, this batch
extern int32_t king_alt_score_target;         // 0x006b1148, this batch

extern void unit_reset_gauge_if_flagged(void); // 0x4633a0, UNSURE exact identity
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_engine_koth_alt_scorer_tick(uint32_t player_index); // 0x46c230, this batch
extern void game_engine_koth_update_occupant_table(uint32_t index); // 0x46c320, this batch

uint32_t game_engine_koth_dispatch_player_scoring(uint32_t player_index)
{
    uint32_t idx = player_index & 0xffff;
    player *p = (player *)((uint8_t *)player_data->data + idx * sizeof(player));
    int32_t occupied_slots;
    int32_t i;
    uint32_t result = 0;

    p->hud_message_index = (datum_index)0xffffffff;
    p->hud_message_player = (datum_index)0xffffffff;

    game_engine_koth_update_occupant_table(player_index);

    occupied_slots = 0;
    if (game_engine_variant.engine.oddball.ball_count > 0) {
        for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
            if (king_hill_occupant_table[i] == player_index) {
                occupied_slots++;
            }
        }
    }
    result = (uint32_t)occupied_slots;

    p->speed = 1.0f; // 0x6c
    if (occupied_slots > 0) {
        if (game_engine_variant.engine.oddball.trait_with_ball != 1) {
            unit_reset_gauge_if_flagged();
        }
        if (game_engine_variant.engine.oddball.speed_with_ball == 1) {
            p->speed = 1.0f;
        } else if (game_engine_variant.engine.oddball.speed_with_ball == 2) {
            p->speed = 1.25f;
        } else {
            p->speed = 0.75f;
        }
    }

    if ((current_game_engine == 0 || game_engine_state_value == 0) &&
        game_engine_variant.engine.oddball.ball_type != 2 && occupied_slots > 0) {
        int32_t remaining = occupied_slots;
        do {
            if (game_engine_variant.engine.oddball.ball_type == 0) {
                p->hud_message_index = (datum_index)0x29;
                p->hud_message_player = (datum_index)player_index;
            }
            game_engine_koth_alt_scorer_tick(player_index);
            remaining--;
        } while (remaining != 0);
    }

    if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type < 3 && occupied_slots > 0) {
        p->hud_message_index = (datum_index)0x23;
        p->hud_message_player = (datum_index)player_index;
    }

    if (p->unit != (datum_index)0xffffffff) {
        unit_data *unit = (unit_data *)((uint8_t *)
            ((object_header *)object_data->data)[(uint32_t)p->unit & 0xffff].data +
            k_unit_data_offset);
        if (unit->current_weapon_index != -1) {
            datum_index weapon = unit->weapons[unit->current_weapon_index];
            if (weapon != (datum_index)0xffffffff) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon & 0xffff].data;
                uint32_t *tag_data = (uint32_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                if ((*(uint32_t *)((uint8_t *)tag_data + 0x308) >> 3 & 1) != 0) {
                    int32_t score = king_alt_player_score[idx];
                    if (score > 0 && score % 0x96 == 0 && score < king_alt_score_target) {
                        // UNSURE: Ghidra shows this void call's result assigned to the return
                        // value (a leaked-register artifact, like the "always -1" case in
                        // game_engine_find_player_holding_object.c); `result` is left unchanged.
                        game_engine_queue_multiplayer_sound(0x2a, 0xffffffff, 0); // 0x46c5a4..0x46c5ad (EDX, the zero remainder, is the broadcast)
                    }
                    // UNSURE: weapon_obj + 0x2b8, not a named weapon_data field
                    *(int16_t *)((uint8_t *)weapon_obj + 0x2b8) = (int16_t)(score / 30);
                }
            }
        }
    }

    return result;
}

#if 0
Original Ghidra decompilation (0x46c3e0), from tools/pack.py 0x46c3e0:

uint FUN_0046c3e0(uint param_1)

{
  int iVar1;
  uint *puVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int local_8;

  uVar3 = param_1;
  uVar6 = param_1 & 0xffff;
  iVar7 = uVar6 * 0x200;
  iVar1 = *(int *)(DAT_0087a480 + 0x34) + iVar7;
  *(undefined4 *)(iVar1 + 0x74) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x78) = 0xffffffff;
  FUN_0046c320();
  iVar5 = 0;
  iVar4 = 0;
  param_1 = 0;
  if (0 < DAT_006f1d18) {
    do {
      if ((&DAT_006b120c)[iVar4] == uVar3) {
        iVar5 = iVar5 + 1;
      }
      iVar4 = iVar4 + 1;
      param_1 = iVar5;
    } while (iVar4 < DAT_006f1d18);
  }
  *(undefined4 *)(iVar1 + 0x6c) = 0x3f800000;
  if (0 < (int)param_1) {
    if (DAT_006f1d0c != 1) {
      FUN_004633a0();
    }
    if (DAT_006f1d08 == 1) {
      *(undefined4 *)(iVar1 + 0x6c) = 0x3f800000;
    }
    else if (DAT_006f1d08 == 2) {
      *(undefined4 *)(iVar1 + 0x6c) = 0x3fa00000;
    }
    else {
      *(undefined4 *)(iVar1 + 0x6c) = 0x3f400000;
    }
  }
  if ((((DAT_006f1d20 == 0) || (DAT_0087aa10 == 0)) && (DAT_006f1d14 != 2)) &&
     (local_8 = param_1, 0 < (int)param_1)) {
    do {
      if (DAT_006f1d14 == 0) {
        iVar4 = *(int *)(DAT_0087a480 + 0x34) + iVar7;
        *(undefined4 *)(iVar4 + 0x74) = 0x29;
        *(uint *)(iVar4 + 0x78) = uVar3;
      }
      FUN_0046c230();
      local_8 = local_8 + -1;
    } while (local_8 != 0);
  }
  if (((0 < DAT_006f1d14) && (DAT_006f1d14 < 3)) && (0 < (int)param_1)) {
    iVar7 = *(int *)(DAT_0087a480 + 0x34) + iVar7;
    *(undefined4 *)(iVar7 + 0x74) = 0x23;
    *(uint *)(iVar7 + 0x78) = uVar3;
  }
  uVar3 = *(uint *)(iVar1 + 0x34);
  if (uVar3 != 0xffffffff) {
    uVar3 = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
    if ((*(short *)(uVar3 + 0x2f2) != -1) &&
       (uVar3 = *(uint *)(uVar3 + 0x2f8 + *(short *)(uVar3 + 0x2f2) * 4), uVar3 != 0xffffffff)) {
      puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
      uVar3 = *(uint *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((*(uint *)(uVar3 + 0x308) >> 3 & 1) != 0) {
        iVar1 = *(int *)(&DAT_006b118c + uVar6 * 4);
        uVar3 = (uint)((longlong)iVar1 * -0x77777777);
        if (((0 < iVar1) && (uVar3 = iVar1 / 0x96, iVar1 % 0x96 == 0)) && (iVar1 < DAT_006b1148)) {
          uVar3 = game_engine_queue_multiplayer_sound(0);
        }
        *(short *)(puVar2 + 0xae) =
             ((short)(iVar1 / 0x1e) + (short)(iVar1 >> 0x1f)) -
             (short)((longlong)iVar1 * 0x88888889 >> 0x3f);
      }
    }
  }
  return uVar3;
}
#endif
