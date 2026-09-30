// main_switch_structure_bsp  (Ghidra: main_switch_structure_bsp, already named)
// address 0x4749a0, size 902 bytes
// name confidence: 0.8   rewrite confidence: 0.85 (REWRITTEN 2026-09-28 against objdump 0x4749a0..0x474d2b (really the per-tick players update): all-dead test on +0x10 no_player_has_a_unit, fade-stage counter after the loop, byte store to 0x71973c.)
// evidence: out/phase4/game_functions.md ("Per-tick handling of structure-BSP switch triggers:
//   validates the requested BSP index and either logs an error or performs the switch"); CEA-pdb
//   string match on this function's own two format strings. types/tags.h Scenario
//   (bsp_switch_trigger_volumes reflexive at scenario+0x39c/+0x3a0, computed by counting
//   TagReflexive members forward from player_starting_profile against types/game.h's own
//   +0x378/+0x384 anchors for netgame_flags/netgame_equipment), ScenarioBSPSwitchTriggerVolume
//   (trigger_volume/source/destination, 8 bytes); types/objects.h object::flags (+0x10);
//   types/game.h player_globals::respawn_stagger (+0x0e), player::teleport_blocked (+0xd4).
//   objdump -d -M intel --start-address=0x4749a0 --stop-address=0x474d40 bin/halo.exe: CORRECTED
//   a genuine Ghidra bug (see below) and confirmed every register.
//
// CORRECTED: Ghidra renders the per-player interaction-state reset at the end of the loop as
// `*(undefined2 *)(iVar12 + 0x1fffe28) = 0; *(undefined4 *)(iVar12 + 0x1fffe24) = 0xffffffff;`
// against `iVar12 = *(int *)(DAT_0087a480 + 0x34)` (player_data->data alone) -- a wild, clearly
// wrong pointer. The disassembly shows the real computation includes the player index term
// Ghidra dropped (`(player_handle & 0xffff) << 9`), landing on the current player's own
// interaction_type (+0x28) and interaction_object (+0x24), exactly the same reset the two
// player constructors perform.
// UNSURE: player_globals+0x17 (a nibble-swap "fade stage" counter), player+0xcc (a screen-fade
// tick countdown) and 0x006b0b80+2 are all in this module's documented TYPES-GAP tails; kept as
// raw offsets. player_update_nearby_interactions_primary/player_update_nearby_interactions_secondary's own register convention (EDI -> player_handle) was
// read directly off their entry instructions since neither is in this batch.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"
#include "fn_scenario.h"
#include "fn_main.h"
#include <stdint.h>

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0
extern int16_t network_game_mode;            // 0x00719720
extern Scenario *global_scenario;            // 0x00746f8c
extern int16_t global_structure_bsp_index;  // 0x0069e8d8, UNSURE name
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, UNSURE name (low 16 bits of a
                                                //   larger record another module owns)
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t global_007102d8;              // 0x007102d8, UNSURE identity
extern uint8_t network_join_error_reason;              // 0x0071973c, UNSURE identity; BYTE stores (0x474cff)
extern uint8_t main_globals_byte_0071974f;              // 0x0071974f, UNSURE identity
extern uint8_t *main_game_globals;             // 0x006b0b80, TYPES-GAP (cached_object_render_states
                                              //   elsewhere in this module; only +0x2 is touched here)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void player_effect_apply_generic_damage_feedback(datum_index player_index, real fade_fraction); // 0x4569d0, EDX player, stack fade_fraction

extern void player_kill_streak_tick(datum_index player_handle); // this batch, 0x479d10, blam-cc: EAX -> player_handle

    // 0x53f020

extern void hud_display_loading_message(uint8_t is_begin); // 0x4aa2a0, AL
    // split_screen_quit_prompt_string
extern void player_update_nearby_interactions_primary(datum_index player_handle); // this batch, 0x478400, blam-cc: EDI -> player_handle
extern void player_update_nearby_interactions_secondary(datum_index player_handle); // this batch, 0x478500, blam-cc: EDI -> player_handle

// Per-tick BSP-switch-trigger handling. First decrements player_globals::respawn_stagger and,
// once it reaches 0, clears a render-state flag. Then for every player: advances its screen-fade
// state (a countdown that either ticks down or, once triggered, drives player_effect_apply_generic_damage_feedback with the
// fraction remaining), and once the fade completes and the player's unit does not have object
// flags bit 0x20 set, announces game-over-style kill feed lines to every player while hosting
// and sets that flag. Advances the player's kill-streak timers if it has a unit. Then, if the
// player's unit's root ancestor's object flags do not have bit 0x200000 set, scans every
// bsp_switch_trigger_volume whose source matches the currently active structure BSP: if the
// player is standing in its named trigger volume, validates the destination BSP index (logging
// an error for an out-of-range or same-as-current one) and otherwise stages it in
// split_screen_quit_prompt_string and calls hud_display_loading_message to perform the switch. Finally resets the
// player's pending interaction and dispatches the appropriate nearby-interaction scan
// (player_update_nearby_interactions_secondary while this machine is a network client, player_update_nearby_interactions_primary otherwise).
void main_switch_structure_bsp(void)
{
    data_iterator player_iter;
    player *plr;
    datum_index player_handle;
    datum_index walk, root;
    object *root_obj;

    if (local_player_globals->respawn_stagger > 0) {
        local_player_globals->respawn_stagger = local_player_globals->respawn_stagger - 1;
        if (local_player_globals->respawn_stagger == 0) {
            main_game_globals[2] = 0;
        }
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;

    plr = (player *)data_iterator_next(&player_iter);
    while (plr != (player *)0) {
        player_handle = player_iter.index;

        {
            // UNSURE: player+0xcc, a screen-fade tick countdown within the unresolved
            // unknown_ca[] tail (types/game.h); accessed as a raw int32 since no named field
            // covers it.
            int32_t *fade_ticks = (int32_t *)((uint8_t *)plr + 0xcc);

            if (plr->teleport_blocked == 0) {
                if (*fade_ticks > 0) {
                    *fade_ticks = *fade_ticks - 1;
                }
            } else if (*fade_ticks < 0x5a) {
                player_effect_apply_generic_damage_feedback(player_handle, (real)*fade_ticks * 0.011111111f); // 0x474af9: EDX = the player
            } else if (plr->unit != (datum_index)-1) {
                object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
                if ((*((uint8_t *)unit_obj + 0x106) & 0x20) == 0) {
                    if (network_game_mode == 2) {
                        // 0x474a68..0x474a7b: (player, 0x1f, -1, 1) for this player; the every-player walk at
                        // 0x474a80 only runs for a -1 handle, which the outer iterator never yields
                        chimera__kill_feed(player_handle, 0x1f, (uint32_t)0xffffffff, 1, 0);
                    }
                    *((uint8_t *)unit_obj + 0x106) = *((uint8_t *)unit_obj + 0x106) | 0x20;
                }
            }
        }
        plr->teleport_blocked = 0;

        if (plr->unit != (datum_index)-1) {
            player_kill_streak_tick(player_handle);
        }

        if (plr->unit != (datum_index)-1) {
            walk = plr->unit;
            do {
                root = walk;
                walk = ((object_header *)object_data->data)[root & 0xffff].data->parent_object;
            } while (walk != (datum_index)-1);
            root_obj = ((object_header *)object_data->data)[root & 0xffff].data;

            if ((root_obj->flags & 0x200000) == 0 && global_scenario->bsp_switch_trigger_volumes.count > 0) {
                ScenarioBSPSwitchTriggerVolume *volumes =
                    (ScenarioBSPSwitchTriggerVolume *)global_scenario->bsp_switch_trigger_volumes.pointer;
                int32_t count = global_scenario->bsp_switch_trigger_volumes.count;
                int32_t i;

                for (i = 0; i < count; i = i + 1) {
                    ScenarioBSPSwitchTriggerVolume *entry = &volumes[i];
                    if (entry->source == (uint16_t)global_structure_bsp_index && plr->unit != (datum_index)-1 &&
                        scenario_trigger_volume_contains_point((int16_t)entry->trigger_volume,
                            (real_point3d *)(*(uint8_t **)((uint8_t *)object_data->data + (plr->unit & 0xffff) * 0xc + 8) + 0xa0)) != 0) {
                        // FIXED (0x474bc8..0x474be4): ECX = the unit's centre (object +0xa0), not its handle.
                        int16_t destination = (int16_t)entry->destination;

                        // CORRECTED by review: the first pass dropped this three-step
                        // read-modify-write on player_globals + 0x17 entirely. The original is
                        //   bVar6 = *(byte *)(iVar12 + 0x17) & 0xf;
                        //   *(byte *)(iVar12 + 0x17) = bVar6;
                        //   *(byte *)(iVar12 + 0x17) = (*(byte *)(iVar8 + 2) ^ bVar6) & 0xf ^ bVar6;
                        // i.e. clear the high nibble, then stamp this player's
                        // local_player_index into the low nibble. The intermediate store is
                        // preserved because it is a separate write to a global the rest of the
                        // frame can observe.
                        {
                            uint8_t *stage_byte = (uint8_t *)local_player_globals + 0x17;
                            uint8_t low = (uint8_t)(*stage_byte & 0xf);
                            *stage_byte = low;
                            *stage_byte = (uint8_t)((((uint8_t)plr->local_player_index ^ low) & 0xf) ^ low);
                        }
                        local_player_globals->unknown_12 = (int16_t)i; // UNSURE field identity confirmed by offset only
                        if (destination < 0 || destination >= global_scenario->structure_bsps.count) {
                            console_print_va("tried to switch to invalid structure-bsp %d", (int32_t)destination);
                        } else if (destination == global_structure_bsp_index) {
                            console_print_va("tried to switch to current structure-bsp %d", (int32_t)destination);
                        } else {
                            // CORRECTED by review: the original's "goto LAB_00474c59" lands on
                            // the INNER volume loop's own increment -- it only skips the two
                            // console_print_va calls, which the if/else chain here already
                            // skips. The first pass turned it into a jump out to the player
                            // loop's advance, which wrongly skipped the interaction reset and
                            // the player_update_nearby_interactions_primary/00478500 dispatch below for that player.
                            split_screen_quit_prompt_string = (uint16_t)destination;
                            hud_display_loading_message(1); // FIXED: AL = 1 (0x474c39)
                        }
                    }
                }
            }
        }

        plr->interaction_object = (datum_index)-1;
        plr->interaction_type = 0;

        if (network_game_mode == 1) {
            player_update_nearby_interactions_secondary(player_handle);
        } else {
            player_update_nearby_interactions_primary(player_handle);
        }

        plr = (player *)data_iterator_next(&player_iter);
    }

    // 0x474cbf..0x474ce1: the fade-stage nibble counter at player_globals +0x17 advances once per tick, after
    // the player loop (FIXED 2026-09-28: the draft advanced it inside the loop, before the BSP switch stamps it)
    {
        uint8_t *stage = (uint8_t *)local_player_globals + 0x17;
        if ((*stage & 0xf) != 0xf) {
            *stage = (uint8_t)(((*stage & 0xf0) + 0x10) ^ (*stage & 0xf));
            if ((*stage & 0xf0) > 0xc0) {
                *stage = 0xf;
            }
        }
    }

    // 0x474ce5: every player without a unit (dead) -> lost_map (0x71974f), which main_loop turns into the
    // checkpoint revert. FIXED 2026-09-28: the draft tested +0x16 (never set), so dying never reverted.
    if (local_player_globals->no_player_has_a_unit == 0) {
        if (global_007102d8 != 0) {
            global_007102d8 = 0;
        }
    } else if (current_game_engine == 0 && global_007102d8 == 0) {
        network_join_error_reason = 0;
        main_globals_byte_0071974f = 1;
        global_007102d8 = 1;
    }
}

#if 0
Original Ghidra decompilation (0x4749a0), from tools/pack.py 0x4749a0:

/* WARNING: Removing unreachable block (ram,0x00474a70) */

void main_switch_structure_bsp(void)

{
  byte *pbVar1;
  int *piVar2;
  short sVar3;
  uint uVar4;
  char cVar5;
  byte bVar6;
  short sVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int *piVar11;
  int iVar12;
  char *format;

  iVar12 = DAT_0087a478;
  if ((0 < *(short *)(DAT_0087a478 + 0xe)) &&
     (sVar7 = *(short *)(DAT_0087a478 + 0xe) + -1, *(short *)(DAT_0087a478 + 0xe) = sVar7,
     sVar7 == 0)) {
    *(undefined1 *)(DAT_006b0b80 + 2) = 0;
  }
  iVar8 = data_iterator_next();
  do {
    if (iVar8 == 0) {
      bVar6 = *(byte *)(iVar12 + 0x17);
      if (((bVar6 & 0xf) != 0xf) &&
         (bVar6 = (bVar6 & 0xf0) + 0x10 ^ bVar6 & 0xf, *(byte *)(iVar12 + 0x17) = bVar6,
         0xc0 < (bVar6 & 0xf0))) {
        *(undefined1 *)(iVar12 + 0x17) = 0xf;
      }
      if (*(char *)(iVar12 + 0x10) == '\0') {
        if (DAT_007102d8 != '\0') {
          DAT_007102d8 = '\0';
        }
      }
      else if ((DAT_006f1d20 == 0) && (DAT_007102d8 == '\0')) {
        DAT_0071973c = 0;
        DAT_0071974f = 1;
        DAT_007102d8 = 1;
        return;
      }
      return;
    }
    iVar10 = *(int *)(iVar8 + 0xcc);
    if (*(char *)(iVar8 + 0xd4) == '\0') {
      if (0 < iVar10) {
        *(int *)(iVar8 + 0xcc) = iVar10 + -1;
      }
    }
    else if (iVar10 < 0x5a) {
      FUN_004569d0((float)iVar10 * 0.011111111);
    }
    else if ((*(uint *)(iVar8 + 0x34) != 0xffffffff) &&
            ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                (*(uint *)(iVar8 + 0x34) & 0xffff) * 0xc) + 0x106) & 0x20) == 0)) {
      if (DAT_00719720 == 2) {
        iVar10 = data_iterator_next();
        while (iVar10 != 0) {
          chimera__kill_feed(0xffffffff,0x1f,0xffffffff,1);
          iVar10 = data_iterator_next();
          iVar12 = DAT_0087a478;
        }
      }
      pbVar1 = (byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                (*(uint *)(iVar8 + 0x34) & 0xffff) * 0xc) + 0x106);
      *pbVar1 = *pbVar1 | 0x20;
    }
    *(undefined1 *)(iVar8 + 0xd4) = 0;
    if (*(int *)(iVar8 + 0x34) != -1) {
      FUN_00479d10();
    }
    if (*(uint *)(iVar8 + 0x34) != 0xffffffff) {
      uVar4 = *(uint *)(iVar8 + 0x34);
      do {
        uVar9 = uVar4;
        uVar4 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) +
                         0x11c);
      } while (uVar4 != 0xffffffff);
      if (((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) + 0x10) &
           0x200000) == 0) &&
         (piVar2 = (int *)(global_scenario + 0x39c), 0 < *(int *)(global_scenario + 0x39c))) {
        iVar10 = 0;
        piVar11 = (int *)(global_scenario + 0x3a0);
        sVar7 = 0;
        do {
          iVar10 = *piVar11 + iVar10 * 8;
          if (((*(short *)(iVar10 + 2) == DAT_0069e8d8) && (*(int *)(iVar8 + 0x34) != -1)) &&
             (cVar5 = scenario_trigger_volume_contains_point(), cVar5 != '\0')) {
            bVar6 = *(byte *)(iVar12 + 0x17) & 0xf;
            *(byte *)(iVar12 + 0x17) = bVar6;
            *(byte *)(iVar12 + 0x17) = (*(byte *)(iVar8 + 2) ^ bVar6) & 0xf ^ bVar6;
            *(short *)(iVar12 + 0x12) = sVar7;
            sVar3 = *(short *)(iVar10 + 4);
            if ((sVar3 < 0) || (*(int *)(global_scenario + 0x5a4) <= (int)sVar3)) {
              format = "tried to switch to invalid structure-bsp %d";
            }
            else {
              if (sVar3 != DAT_0069e8d8) {
                DAT_00719754._0_2_ = sVar3;
                FUN_004aa2a0();
                iVar12 = DAT_0087a478;
                goto LAB_00474c59;
              }
              format = "tried to switch to current structure-bsp %d";
            }
            console_print_va(format,(int)sVar3);
            iVar12 = DAT_0087a478;
          }
LAB_00474c59:
          sVar7 = sVar7 + 1;
          iVar10 = (int)sVar7;
        } while (iVar10 < *piVar2);
      }
    }
    iVar12 = *(int *)(DAT_0087a480 + 0x34);
    *(undefined2 *)(iVar12 + 0x1fffe28) = 0;
    *(undefined4 *)(iVar12 + 0x1fffe24) = 0xffffffff;
    if (DAT_00719720 == 1) {
      FUN_00478500();
    }
    else {
      FUN_00478400();
    }
    iVar8 = data_iterator_next();
    iVar12 = DAT_0087a478;
  } while( true );
}
#endif
