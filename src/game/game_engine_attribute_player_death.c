// game_engine_attribute_player_death  (Ghidra: game_engine_attribute_player_death, already named)
// address 0x46ff00, size 1483 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Determines kill/assist credit for a player's death by
// scanning the victim's recent-damager history and team relationships before invoking
// game_engine_on_player_death with the resolved killer"); out/phase4/game_types_notes.md ("the
// whole statistics block... every surviving entry of the victim's four-slot recent-damager list
// (unit+0x43c, unit_recent_damage in types/units.h) gets assists (0xa4)"); types/units.h
// unit_recent_damage (0x430, 4 entries of 0x10: tick, damage, responsible_unit,
// responsible_player); types/game.h player statistics block; game_engine_on_player_death.c
// (0x460200, already rewritten) for its (killer, death_object, victim, is_suicide) signature.
// register convention: fully reconstructed against
//   objdump -d -M intel --start-address=0x46ff00 --stop-address=0x470500 bin/halo.exe
// because every register-passed argument here is register-passed and Ghidra shows none of them
// explicitly (`in_EAX` for the object argument is never named; the killer_team argument
// disappears completely, printed as an unused `param_3`). Confirmed at the disassembly level:
// EAX holds the dying unit's own object index (saved to ESI immediately, so it survives the
// player_index_from_unit_index call); the first stack dword is the killer candidate (Ghidra's "param_1", compared
// against the resolved victim for the suicide count and later reassigned to the chosen
// recent-damager's responsible_player); the second stack dword passes straight through as
// game_engine_on_player_death's death_object; the third stack dword (a small int stored in a full
// stack slot) is loaded into ECX right before the one visible call to teams_are_enemies, with
// victim_team already resident in EDX from three instructions earlier (`mov edx,[eax+0x20]`) --
// i.e. it is teams_are_enemies(killer_team, victim_team); and the fourth stack byte is the
// credit-assists/betrayals flag gating almost everything after the recent-damager compaction.
//   // blam-cc: EAX -> victim_unit, stack -> killer, death_object, killer_team, credit_kills
// UNSURE: player_index_from_unit_index (0x474db0, not in this batch) is called as `player_index_from_unit_index(victim_unit)` and
// its return value is used exactly like a player datum_index (bounds/salt-checked below at
// iVar13); out/phase4/game_functions.md could not pin its behavior beyond "iterates all
// players", so it is modeled here as "find the player currently controlling this unit" on
// evidence alone, not a confirmed name.
// UNSURE: DAT_006b1458 (an early-out flag gating the whole function) is not attested anywhere
// else in this module's notes; modeled as a raw byte extern.
// UNSURE: the tie-break condition selecting between a `killer`-matching recent-damager entry and
// the highest-damage one within the assist window is transcribed exactly as Ghidra's own control
// flow (verified correct against the disassembly for the compaction section), but its game-design
// intent is not independently confirmed.
// NOTE (verified against objdump, not a transcription guess): inside that same scoring loop, the
// "is this entry's owner an enemy of the victim" test compares victim_team against the raw low 16
// bits of the entry's responsible_player datum_index (a player array slot number), not against
// that player's actual ::team field -- unlike every other team comparison in this function, which
// does dereference ::team. Preserved exactly; see the comment at entry_team below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

extern uint8_t game_engine_attribute_enabled;    // 0x006b1458, UNSURE: see header
extern game_time_globals *game_time;             // 0x006f1d6c
extern data_array *player_data;                  // 0x0087a480
extern data_array *object_data;               // 0x008603b0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern team_pair_globals *team_pair_data;        // 0x006b0b84

extern datum_index player_index_from_unit_index(datum_index unit); // 0x474db0, not in this batch; UNSURE signature, see header
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50

extern void player_advance_multikill_medal(datum_index player_handle); // 0x479eb0, this batch, UNSURE args (register-passed)

// blam-cc: EAX -> victim_unit, stack -> killer, death_object, killer_team, credit_kills
// Resolves kill/assist/betrayal credit for the death of the player currently controlling
// `victim_unit`. Bumps the victim's own deaths/suicides and resets its streak fields, compacts
// `victim_unit`'s four-slot recent-damager list down to entries that still name a live player
// (writing the compacted list back in place), then -- when `credit_kills` is set -- picks either
// the entry matching `killer` or, failing that within the ~6-second assist window, the
// highest-damage entry as the resolved killer, credits that killer's kills/betrayals, and (when
// there was a genuine, non-suicide, non-friendly kill within the window) credits an assist to
// every other still-live, sufficiently-damaging, enemy-team entry. Finally forwards to
// game_engine_on_player_death with whatever killer ended up resolved (or `killer` unchanged, if
// the early-out path was taken).
void game_engine_attribute_player_death(datum_index victim_unit, datum_index killer,
    datum_index death_object, int32_t killer_team, char credit_kills)
{
    datum_index victim;
    player *victim_player;
    int32_t victim_team;
    uint8_t friendly_or_no_killer;
    int32_t current_tick;
    int32_t assist_window_start;
    object *victim_object;
    unit_recent_damage *recent_damage;
    unit_recent_damage compacted[4];
    int32_t compacted_count;
    int32_t i;
    int16_t best_damage_index;
    int16_t killer_match_index;
    int16_t index;
    float best_damage;
    float assist_damage_threshold;
    uint32_t death_flags;

    if (!game_engine_attribute_enabled) {
        return;
    }
    victim = player_index_from_unit_index(victim_unit);
    if (victim == k_datum_index_none) {
        return;
    }
    victim_player = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)victim * sizeof(player));
    victim_team = victim_player->team;
    if (killer == victim) {
        victim_player->suicides = victim_player->suicides + 1;
    }
    victim_player->deaths = victim_player->deaths + 1;
    victim_player->killing_spree_count = 0;
    victim_player->last_kill_tick = -1;
    victim_player->multikill_count = 0;

    if (killer == k_datum_index_none) {
        friendly_or_no_killer = 1;
    } else {
        friendly_or_no_killer = !teams_are_enemies((int16_t)killer_team, (int16_t)victim_team);
    }

    current_tick = game_time->game_time;
    assist_window_start = current_tick - 0xb4;
    victim_object = *(object **)((uint8_t *)object_data->data +
        (uint32_t)(uint16_t)victim_unit * object_data->size + 8);
    recent_damage = (unit_recent_damage *)((uint8_t *)victim_object + 0x430);

    compacted_count = 0;
    for (i = 0; i < 4; i++) {
        compacted[i].tick = 0;
        compacted[i].damage = 0.0f;
        compacted[i].responsible_unit = k_datum_index_none;
        compacted[i].responsible_player = k_datum_index_none;
    }
    for (i = 0; i < 4; i++) {
        datum_index responsible = recent_damage[i].responsible_player;
        int16_t responsible_index = (int16_t)responsible;
        int16_t responsible_salt = (int16_t)((uint32_t)responsible >> 16);

        if (responsible != k_datum_index_none && responsible_index >= 0 &&
            responsible_index < player_data->maximum_count) {
            int16_t slot_identifier = *(int16_t *)((uint8_t *)player_data->data +
                (int32_t)player_data->size * (int32_t)responsible_index);

            if (slot_identifier != 0 && (responsible_salt == 0 || slot_identifier == responsible_salt)) {
                compacted[compacted_count] = recent_damage[i];
                compacted_count = compacted_count + 1;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        recent_damage[i] = compacted[i];
    }

    best_damage = -3.4028235e+38f;
    best_damage_index = -1;
    killer_match_index = -1;
    for (index = 0; index < 4; index++) {
        int16_t chosen = index; // UNSURE: mirrors Ghidra's `sVar9 = sVar20` / tie-break exactly
        uint8_t take_as_primary = 0;

        if (recent_damage[index].responsible_player == killer) {
            take_as_primary = 1;
        } else if (killer_match_index == index) {
            chosen = killer_match_index;
            take_as_primary = 1;
        }

        if (take_as_primary) {
            killer_match_index = chosen;
            if (assist_window_start < recent_damage[index].tick && best_damage < recent_damage[index].damage) {
                best_damage = recent_damage[index].damage;
                best_damage_index = index;
            }
        } else if (friendly_or_no_killer) {
            int16_t entry_team = (int16_t)recent_damage[index].responsible_player; // NOTE: this is
                // the raw low 16 bits of the responsible_player datum_index (i.e. a player array
                // slot number), not a dereferenced player::team -- verified against objdump
                // 0x4701dc/0x470085 (`mov cx,[esi+0xc]`), and transcribed literally because the
                // kill-credit and assist code paths below this loop *do* dereference the real
                // team field the same way, so this is very likely a genuine retail quirk in this
                // one scoring loop rather than a decompiler error.
            uint8_t is_enemy_team;

            if (current_game_engine == 0) {
                if (victim_team < 0 || victim_team > 9 || entry_team < 0 || entry_team > 9) {
                    killer_match_index = chosen;
                    if (assist_window_start < recent_damage[index].tick &&
                        best_damage < recent_damage[index].damage) {
                        best_damage = recent_damage[index].damage;
                        best_damage_index = index;
                    }
                    continue;
                }
                {
                    int32_t bit_index = entry_team + victim_team * 10;

                    is_enemy_team = (uint8_t)(1 - ((team_pair_data->enemy_bits[bit_index >> 5] &
                        (1u << (bit_index & 0x1f))) != 0));
                }
            } else {
                is_enemy_team = (int16_t)victim_team != entry_team;
            }
            if (!is_enemy_team) {
                killer_match_index = chosen;
                if (assist_window_start < recent_damage[index].tick &&
                    best_damage < recent_damage[index].damage) {
                    best_damage = recent_damage[index].damage;
                    best_damage_index = index;
                }
            }
        }
    }

    death_flags = (uint32_t)assist_window_start & 0xffffff00u;
    if ((best_damage_index == -1 && killer_match_index == -1) || credit_kills != 1) {
        assist_damage_threshold = 0.0f;
    } else {
        killer = recent_damage[best_damage_index].responsible_player;
        assist_damage_threshold = recent_damage[best_damage_index].damage * 0.4f;
    }

    if (killer != k_datum_index_none && credit_kills == 1) {
        player *killer_player = (player *)((uint8_t *)player_data->data +
            (uint32_t)(uint16_t)killer * sizeof(player));
        int16_t killer_player_team = killer_player->team;
        uint8_t is_friendly;

        if (current_game_engine == 0 && victim_team >= 0 && victim_team < 10 &&
            killer_player_team >= 0 && killer_player_team < 10) {
            int32_t bit_index = killer_player_team + victim_team * 10;

            is_friendly = (uint8_t)((team_pair_data->enemy_bits[bit_index >> 5] &
                (1u << (bit_index & 0x1f))) != 0);
        } else if (current_game_engine != 0) {
            is_friendly = (int16_t)victim_team == killer_player_team;
        } else {
            is_friendly = 0; // UNSURE: falls through Ghidra's `goto LAB_004703c6` when out of range
        }

        if (is_friendly) {
            killer_player->betrayals = killer_player->betrayals + 1;
            death_flags = ((uint32_t)assist_window_start & 0xffffff00u) | 1u;
            if (killer != victim) {
                killer_player->betrayal_penalty_count = killer_player->betrayal_penalty_count + 1;
                player_advance_multikill_medal(killer);
            }
        } else {
            killer_player->kills = killer_player->kills + 1;
            killer_player->killing_spree_count = killer_player->killing_spree_count + 1;
            if (killer_player->last_kill_tick < current_tick - 0x78) {
                killer_player->multikill_count = 1;
            } else {
                killer_player->multikill_count = killer_player->multikill_count + 1;
            }
            killer_player->last_kill_tick = (int16_t)game_time->game_time;
        }
    }

    if (assist_damage_threshold > 0.0f && credit_kills == 1) {
        for (index = 0; index < 4; index++) {
            datum_index assist_candidate = recent_damage[index].responsible_player;

            if ((index == killer_match_index || recent_damage[index].damage < assist_damage_threshold) &&
                assist_candidate != k_datum_index_none && assist_candidate != killer) {
                player *candidate_player = (player *)((uint8_t *)player_data->data +
                    (uint32_t)(uint16_t)assist_candidate * sizeof(player));
                int16_t candidate_team = candidate_player->team;
                uint8_t is_friendly;

                if (current_game_engine == 0 && victim_team >= 0 && victim_team < 10 &&
                    candidate_team >= 0 && candidate_team < 10) {
                    int32_t bit_index = candidate_team + victim_team * 10;

                    is_friendly = (uint8_t)((team_pair_data->enemy_bits[bit_index >> 5] &
                        (1u << (bit_index & 0x1f))) != 0);
                } else if (current_game_engine != 0) {
                    is_friendly = (int16_t)victim_team == candidate_team;
                } else {
                    is_friendly = 1; // UNSURE: out-of-range teams count as "friendly" here, i.e. no assist
                }

                if (!is_friendly) {
                    candidate_player->assists = candidate_player->assists + 1;
                }
            }
        }
    }

    game_engine_on_player_death(killer, death_object, victim, (char)(death_flags & 0xff));
}

#if 0
Original Ghidra decompilation (0x46ff00), from tools/pack.py 0x46ff00:

void game_engine_attribute_player_death
               (uint param_1,undefined4 param_2,undefined4 param_3,char param_4)

{
  short *psVar1;
  float fVar2;
  short sVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  bool bVar8;
  short sVar9;
  char cVar10;
  short sVar11;
  uint in_EAX;
  uint uVar12;
  int iVar13;
  uint uVar14;
  int iVar15;
  short sVar16;
  uint *puVar17;
  int iVar18;
  uint *puVar19;
  short sVar20;
  uint *puVar21;
  short local_60;
  float local_54;
  uint local_50;
  uint local_40 [16];

  if (DAT_006b1458 == '\0') {
    return;
  }
  uVar12 = FUN_00474db0();
  iVar15 = DAT_0087a480;
  if (uVar12 == 0xffffffff) {
    return;
  }
  iVar13 = (uVar12 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  uVar4 = *(undefined4 *)(iVar13 + 0x20);
  if (param_1 == uVar12) {
    *(short *)(iVar13 + 0xb0) = *(short *)(iVar13 + 0xb0) + 1;
  }
  *(short *)(iVar13 + 0xae) = *(short *)(iVar13 + 0xae) + 1;
  *(undefined2 *)(iVar13 + 0x96) = 0;
  *(undefined2 *)(iVar13 + 0x9a) = 0xffff;
  *(undefined2 *)(iVar13 + 0x98) = 0;
  if (param_1 == 0xffffffff) {
LAB_0046ff94:
    bVar8 = true;
  }
  else {
    cVar10 = FUN_0045bd50();
    bVar8 = false;
    if (cVar10 == '\0') goto LAB_0046ff94;
  }
  iVar13 = *(int *)(DAT_006f1d6c + 0xc);
  uVar14 = iVar13 - 0xb4;
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar6 = *(int *)(iVar5 + 0x43c);
  iVar18 = 0;
  local_40[1] = 0;
  local_40[0] = 0;
  local_40[2] = 0xffffffff;
  local_40[3] = 0xffffffff;
  local_40[5] = 0;
  local_40[4] = 0;
  local_40[6] = 0xffffffff;
  local_40[7] = 0xffffffff;
  local_40[9] = 0;
  local_40[8] = 0;
  local_40[10] = 0xffffffff;
  local_40[0xb] = 0xffffffff;
  local_40[0xd] = 0;
  local_40[0xc] = 0;
  local_40[0xe] = 0xffffffff;
  local_40[0xf] = 0xffffffff;
  if ((((iVar6 != -1) && (sVar11 = (short)iVar6, -1 < sVar11)) &&
      (sVar11 < *(short *)(iVar15 + 0x20))) &&
     ((sVar11 = *(short *)((int)*(short *)(iVar15 + 0x22) * (int)sVar11 + *(int *)(iVar15 + 0x34)),
      sVar11 != 0 && ((sVar16 = (short)((uint)iVar6 >> 0x10), sVar16 == 0 || (sVar11 == sVar16))))))
  {
    local_40[0] = *(uint *)(iVar5 + 0x430);
    local_40[1] = *(undefined4 *)(iVar5 + 0x434);
    local_40[2] = *(undefined4 *)(iVar5 + 0x438);
    local_40[3] = *(undefined4 *)(iVar5 + 0x43c);
    iVar18 = 1;
  }
  iVar6 = *(int *)(iVar5 + 0x44c);
  if ((((iVar6 != -1) && (sVar11 = (short)iVar6, -1 < sVar11)) &&
      ((sVar11 < *(short *)(iVar15 + 0x20) &&
       (sVar11 = *(short *)((int)*(short *)(iVar15 + 0x22) * (int)sVar11 + *(int *)(iVar15 + 0x34)),
       sVar11 != 0)))) &&
     ((sVar16 = (short)((uint)iVar6 >> 0x10), sVar16 == 0 || (sVar11 == sVar16)))) {
    local_40[iVar18 * 4] = *(uint *)(iVar5 + 0x440);
    local_40[iVar18 * 4 + 1] = *(uint *)(iVar5 + 0x444);
    uVar7 = *(uint *)(iVar5 + 0x44c);
    local_40[iVar18 * 4 + 2] = *(uint *)(iVar5 + 0x448);
    local_40[iVar18 * 4 + 3] = uVar7;
    iVar18 = iVar18 + 1;
  }
  iVar6 = *(int *)(iVar5 + 0x45c);
  if ((((iVar6 != -1) && (sVar11 = (short)iVar6, -1 < sVar11)) &&
      (sVar11 < *(short *)(iVar15 + 0x20))) &&
     ((sVar11 = *(short *)((int)*(short *)(iVar15 + 0x22) * (int)sVar11 + *(int *)(iVar15 + 0x34)),
      sVar11 != 0 && ((sVar16 = (short)((uint)iVar6 >> 0x10), sVar16 == 0 || (sVar11 == sVar16))))))
  {
    local_40[iVar18 * 4] = *(uint *)(iVar5 + 0x450);
    local_40[iVar18 * 4 + 1] = *(uint *)(iVar5 + 0x454);
    uVar7 = *(uint *)(iVar5 + 0x45c);
    local_40[iVar18 * 4 + 2] = *(uint *)(iVar5 + 0x458);
    local_40[iVar18 * 4 + 3] = uVar7;
    iVar18 = iVar18 + 1;
  }
  iVar6 = *(int *)(iVar5 + 0x46c);
  if ((((iVar6 != -1) && (sVar11 = (short)iVar6, -1 < sVar11)) &&
      ((sVar11 < *(short *)(iVar15 + 0x20) &&
       (sVar11 = *(short *)((int)*(short *)(iVar15 + 0x22) * (int)sVar11 + *(int *)(iVar15 + 0x34)),
       sVar11 != 0)))) &&
     ((sVar16 = (short)((uint)iVar6 >> 0x10), sVar16 == 0 || (sVar11 == sVar16)))) {
    local_40[iVar18 * 4] = *(uint *)(iVar5 + 0x460);
    local_40[iVar18 * 4 + 1] = *(uint *)(iVar5 + 0x464);
    uVar7 = *(uint *)(iVar5 + 0x46c);
    local_40[iVar18 * 4 + 3] = uVar7;
  }
  fVar2 = -3.4028235e+38;
  puVar17 = (uint *)(iVar5 + 0x430);
  puVar19 = local_40;
  puVar21 = puVar17;
  for (iVar15 = 0x10; iVar15 != 0; iVar15 = iVar15 + -1) {
    *puVar21 = *puVar19;
    puVar19 = puVar19 + 1;
    puVar21 = puVar21 + 1;
  }
  sVar16 = -1;
  sVar11 = -1;
  sVar20 = 0;
  do {
    local_60 = (short)uVar4;
    sVar9 = sVar20;
    if ((puVar17[3] == param_1) || (sVar9 = sVar11, sVar11 == sVar20)) {
LAB_0047023b:
      sVar11 = sVar9;
      if ((uVar14 < *puVar17) && (fVar2 < (float)puVar17[1])) {
        fVar2 = (float)puVar17[1];
        sVar16 = sVar20;
      }
    }
    else if (bVar8) {
      sVar3 = (short)puVar17[3];
      if (DAT_006f1d20 == 0) {
        if ((((local_60 < 0) || (9 < local_60)) || (sVar3 < 0)) || (9 < sVar3)) goto LAB_0047023b;
        iVar15 = (int)sVar3 + local_60 * 10;
        cVar10 = '\x01' - ((1 << ((byte)iVar15 & 0x1f) &
                           *(uint *)(DAT_006b0b84 + 0xa4 + (iVar15 >> 5) * 4)) != 0);
      }
      else {
        cVar10 = local_60 != sVar3;
      }
      if (cVar10 != '\0') goto LAB_0047023b;
    }
    sVar20 = sVar20 + 1;
    puVar17 = puVar17 + 4;
  } while (sVar20 < 4);
  local_50 = uVar14 & 0xffffff00;
  if (((sVar16 == -1) && (sVar16 = sVar11, sVar11 == -1)) || (param_4 != '\x01')) {
    local_54 = 0.0;
  }
  else {
    param_1 = *(uint *)(sVar16 * 0x10 + 0x43c + iVar5);
    local_54 = *(float *)(sVar16 * 0x10 + iVar5 + 0x434) * 0.4;
  }
  if ((param_1 == 0xffffffff) || (param_4 != '\x01')) goto LAB_004703c6;
  iVar15 = (param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  sVar16 = *(short *)(iVar15 + 0x20);
  if (DAT_006f1d20 == 0) {
    if ((((-1 < local_60) && (local_60 < 10)) && (-1 < sVar16)) && (sVar16 < 10)) {
      iVar6 = (int)sVar16 + local_60 * 10;
      cVar10 = '\x01' - ((1 << ((byte)iVar6 & 0x1f) &
                         *(uint *)(DAT_006b0b84 + 0xa4 + (iVar6 >> 5) * 4)) != 0);
      goto LAB_00470354;
    }
  }
  else {
    cVar10 = local_60 != sVar16;
LAB_00470354:
    if (cVar10 == '\0') {
      *(short *)(iVar15 + 0xac) = *(short *)(iVar15 + 0xac) + 1;
      local_50 = CONCAT31((int3)(uVar14 >> 8),1);
      if (param_1 != uVar12) {
        *(short *)(iVar15 + 0xc0) = *(short *)(iVar15 + 0xc0) + 1;
        FUN_00479eb0();
      }
      goto LAB_004703c6;
    }
  }
  *(short *)(iVar15 + 0x9c) = *(short *)(iVar15 + 0x9c) + 1;
  *(short *)(iVar15 + 0x96) = *(short *)(iVar15 + 0x96) + 1;
  if ((int)*(short *)(iVar15 + 0x9a) < iVar13 + -0x78) {
    *(undefined2 *)(iVar15 + 0x98) = 1;
  }
  else {
    *(short *)(iVar15 + 0x98) = *(short *)(iVar15 + 0x98) + 1;
  }
  *(undefined2 *)(iVar15 + 0x9a) = *(undefined2 *)(DAT_006f1d6c + 0xc);
LAB_004703c6:
  if ((0.0 < local_54) && (param_4 == '\x01')) {
    sVar16 = 0;
    puVar17 = (uint *)(iVar5 + 0x43c);
    do {
      if (((sVar16 == sVar11) || (local_54 < (float)puVar17[-2])) &&
         ((uVar14 = *puVar17, uVar14 != 0xffffffff && (uVar14 != param_1)))) {
        iVar15 = (uVar14 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
        sVar20 = *(short *)(iVar15 + 0x20);
        if (DAT_006f1d20 == 0) {
          if ((((-1 < local_60) && (local_60 < 10)) && (-1 < sVar20)) && (sVar20 < 10)) {
            iVar13 = (int)sVar20 + local_60 * 10;
            cVar10 = '\x01' - ((1 << ((byte)iVar13 & 0x1f) &
                               *(uint *)(DAT_006b0b84 + 0xa4 + (iVar13 >> 5) * 4)) != 0);
            goto LAB_0047048d;
          }
        }
        else {
          cVar10 = local_60 != sVar20;
LAB_0047048d:
          if (cVar10 == '\0') goto LAB_0047049c;
        }
        psVar1 = (short *)(iVar15 + 0xa4);
        *psVar1 = *psVar1 + 1;
      }
LAB_0047049c:
      sVar16 = sVar16 + 1;
      puVar17 = puVar17 + 4;
    } while (sVar16 < 4);
  }
  game_engine_on_player_death(param_2,uVar12,local_50);
  return;
}
#endif
