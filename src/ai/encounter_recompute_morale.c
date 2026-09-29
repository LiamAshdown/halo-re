// encounter_recompute_morale  (Ghidra: encounter_recompute_morale; named for this rewrite)
// address 0x437940, size 1234 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: phase-4 summary ("recomputes a squad's aggregate morale/combat statistics from
//   its members and decides whether the squad should begin retreating or disperse its
//   actor-group links"). It is the function types/ai.h already credits with clearing
//   encounter.dirty ("0x435f00 re-runs morale for dirty encounters") and it is the only
//   writer of the three running vitality sums this rewrite named average_vitality on
//   encounter (+0x34), encounter_squad_state (+0x1c) and encounter_platoon_state (+0x0c):
//   each one is summed over the live members and then divided by the matching member count.
// register convention: plain __cdecl, one stack argument.
//   // blam-cc: stack -> encounter_index
//
// UNSURE:
//  - The per-member vitality sample is object+0xe0 for a member that has a unit, and
//    cluster_count / unknown_20 for a member that does not (a swarm); the "sample weight"
//    sVar10 is cluster_count in the first case and 1 in the second. Neither field is named
//    in types/objects.h.
//  - The hostility test uses team_pair_globals.secondary_bits (+0x94), NOT enemy_bits
//    (+0xa4) the way encounters_note_hostile_object does, with the index actor.team * 10 +
//    prop.object_type. prop.object_type is documented in types/ai.h as "object+0xb8 of the
//    tracked object", which this module elsewhere treats as a team. Left as-is.
//  - encounter+0x42 / 0x44 / 0x45 / 0x46 / 0x47 / 0x48 / 0x4a / 0x4c / 0x50 / 0x54 / 0x58
//    form the retreat state machine; only the transitions are reproduced here, not named.
//  - The three averages subtract 0.001 and clamp at zero, which is a deliberate decay, not a
//    rounding artifact.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "ai.h"

extern data_array *encounter_data;                        // 0x008802c8
extern encounter_squad_state *encounter_squad_states;     // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4
extern ai_globals *ai_globals_ptr;                        // 0x00880354
extern data_array *actor_data;                            // 0x00880360
extern data_array *object_data;                           // 0x008603b0
extern data_array *prop_data;                             // 0x008802c0
extern team_pair_globals *team_pair_data;                 // 0x006b0b84
extern game_time_globals *game_time;                      // 0x006f1d6c

extern void encounter_release_stale_props(datum_index encounter_index);  // 0x4382b0
extern void encounter_choose_vocalizations(datum_index encounter_index); // 0x438580

// blam-cc: stack -> encounter_index
// Rebuilds every aggregate the encounter, its squads and its platoons keep about their live
// members (head count, swarm count, engaged count, average vitality), decides from those
// whether the encounter should start or stop retreating, and clears the dirty flag.
void encounter_recompute_morale(datum_index encounter_index)
{
    encounter *enc;
    encounter_squad_state *squad_state;
    encounter_platoon_state *platoon_state;
    actor *a;
    object *obj;
    prop *p;
    datum_index actor_index;
    datum_index current;
    float sample;
    int16_t weight;
    int16_t i;
    int16_t pair;
    uint8_t counts;
    uint8_t engaged;
    uint8_t any_unfriendly_target;
    uint8_t any_vocalizing;
    uint8_t any_flag_8c;
    uint8_t any_flag_8d;
    int32_t retreat_timer;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];

    any_unfriendly_target = 0;
    any_vocalizing = 0;
    any_flag_8c = 0;
    any_flag_8d = 0;

    enc->unknown_44 = 0;
    enc->unknown_45 = 0;
    enc->unknown_30 = 0;
    enc->unknown_2e = 0;
    enc->unknown_2c = 0;
    enc->unknown_2a = 0;
    enc->average_vitality = 0.0f;

    i = 0;
    if (0 < enc->squad_count) {
        do {
            squad_state = &encounter_squad_states[(int16_t)(enc->first_squad + i)];
            i = i + 1;
            squad_state->unknown_1a = 0;
            squad_state->unknown_18 = 0;
            squad_state->average_vitality = 0.0f;
        } while (i < enc->squad_count);
    }
    i = 0;
    if (0 < enc->platoon_count) {
        do {
            platoon_state = &encounter_platoon_states[(int16_t)(enc->first_platoon + i)];
            i = i + 1;
            platoon_state->unknown_08 = 0;
            platoon_state->unknown_06 = 0;
            platoon_state->average_vitality = 0.0f;
        } while (i < enc->platoon_count);
    }

    actor_index = (datum_index)k_datum_index_none;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = ai_globals_ptr->unknown_08;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        actor_index = a->next_in_encounter;
        squad_state = &encounter_squad_states[(int16_t)(a->squad_index + enc->first_squad)];

        if (a->unit_index == (datum_index)k_datum_index_none) {
            weight = a->cluster_count;
            sample = (float)(int32_t)weight / (float)(int32_t)a->unknown_20;
        } else {
            obj = ((object_header *)object_data->data)[a->unit_index & 0xffff].data;
            sample = ((object *)obj)->body_vitality;
            weight = 1;
        }

        if (a->platoon_index != -1) {
            platoon_state =
                &encounter_platoon_states[(int16_t)(enc->first_platoon + a->platoon_index)];
            platoon_state->unknown_06 = platoon_state->unknown_06 + weight;
            platoon_state->average_vitality = sample + platoon_state->average_vitality;
            platoon_state->unknown_08 =
                platoon_state->unknown_08 + (int16_t)((uint16_t)a->swarm * weight);
        }

        squad_state->unknown_18 = squad_state->unknown_18 + weight;
        squad_state->average_vitality = sample + squad_state->average_vitality;
        squad_state->unknown_1a = squad_state->unknown_1a + (int16_t)((uint16_t)a->swarm * weight);

        enc->unknown_2a = enc->unknown_2a + weight;
        enc->unknown_2c = enc->unknown_2c + (int16_t)((uint16_t)a->swarm * weight);

        counts = (uint8_t)(a->awareness_level == 3 && a->alert_floor < a->alert_level);
        enc->unknown_2e = enc->unknown_2e + (int16_t)((uint16_t)counts * weight);

        engaged = (uint8_t)(6 < a->alert_level);
        if (engaged != 0 && a->mode == 4 && 0 < *(int16_t *)(a->mode_data.raw + 0x0c)) { // actor + 0xa8 lies inside actor.mode_data.raw
            engaged = 0;
        }
        enc->average_vitality = sample + enc->average_vitality;
        enc->unknown_30 = enc->unknown_30 + (int16_t)((uint16_t)engaged * weight);

        if (a->target_unit_index != (datum_index)k_datum_index_none) {
            p = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
            enc->unknown_43 = 1;
            if (a->team < 0 || 9 < a->team || p->object_type < 0 || 9 < p->object_type ||
                (pair = (int16_t)((int32_t)p->object_type + a->team * 10),
                 (team_pair_data->secondary_bits[pair >> 5] & (1 << (pair & 0x1f))) == 0)) {
                any_unfriendly_target = 1;
            }
            if (a->unknown_8c != 0) {
                any_flag_8c = 1;
            }
            if (a->unknown_8d != 0) {
                any_flag_8d = 1;
            }
            if (a->alert_level < 7) {
                if (p->kind < 2 || 3 < p->kind) {
                    obj = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
                    counts = *((uint8_t *)obj + 0x106) & 4;
                } else {
                    counts = p->is_vault;
                }
                if (counts != 0) {
                    goto tally_vocalization;
                }
            } else {
                enc->unknown_45 = 1;
            }
            enc->unknown_44 = 1;
        }
tally_vocalization:
        if (0 < a->unknown_1e4) {
            any_vocalizing = 1;
        }
    }

    if (any_unfriendly_target != 0) {
        enc->unknown_46 = 0;
    }

    retreat_timer = enc->unknown_50;
    if (enc->unknown_45 == 0 &&
        (retreat_timer == -1 || 0x3b < retreat_timer) &&
        ((enc->unknown_44 == 0 && ((int32_t)enc->unknown_54 == -1 || 0x3b < (int32_t)enc->unknown_54)) ||
         retreat_timer == -1 || 0x1c1 < retreat_timer)) {
        if (enc->unknown_42 == 0) {
            if (enc->unknown_47 == 0) {
                if (any_flag_8c != 0 && any_flag_8d != 0) {
                    encounter_choose_vocalizations(encounter_index);
                    goto normalize;
                }
            } else {
                enc->unknown_48 = (uint8_t)(any_vocalizing == 0);
                if (enc->unknown_4a != 0) {
                    goto normalize;
                }
            }
            encounter_release_stale_props(encounter_index);
        } else {
            enc->unknown_47 = 0;
            enc->unknown_1a = enc->unknown_2a;
            enc->unknown_58 = game_time->game_time;
            enc->unknown_4c = 0;
            if (enc->unknown_43 == 0) {
                enc->unknown_50 = (datum_index)k_datum_index_none;
                enc->unknown_54 = (datum_index)k_datum_index_none;
            }
        }
    } else {
        enc->unknown_42 = 0;
        enc->unknown_47 = 0;
    }

normalize:
    if (0 < enc->member_count) {
        sample = enc->average_vitality / (float)(int32_t)enc->member_count - 0.001f;
        if (sample < 0.0f) {
            sample = 0.0f;
        }
        enc->average_vitality = sample;
    }
    i = 0;
    if (0 < enc->squad_count) {
        do {
            squad_state = &encounter_squad_states[(int16_t)(enc->first_squad + i)];
            sample = squad_state->average_vitality /
                     (float)(int32_t)squad_state->member_count - 0.001f;
            if (sample < 0.0f) {
                sample = 0.0f;
            }
            i = i + 1;
            squad_state->average_vitality = sample;
        } while (i < enc->squad_count);
    }
    i = 0;
    if (0 < enc->platoon_count) {
        do {
            platoon_state = &encounter_platoon_states[(int16_t)(enc->first_platoon + i)];
            sample = platoon_state->average_vitality /
                     (float)(int32_t)platoon_state->member_count - 0.001f;
            if (sample < 0.0f) {
                sample = 0.0f;
            }
            i = i + 1;
            platoon_state->average_vitality = sample;
        } while (i < enc->platoon_count);
    }
    enc->dirty = 0;
}

#if 0
Original Ghidra decompilation (0x437940):

void FUN_00437940(uint param_1)

{
  short *psVar1;
  float fVar2;
  short sVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  byte bVar8;
  bool bVar9;
  short sVar10;
  uint uVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  uint local_4;

  iVar13 = DAT_008802c8;
  iVar14 = (param_1 & 0xffff) * 0x6c;
  iVar16 = *(int *)(DAT_008802c8 + 0x34) + iVar14;
  sVar10 = 0;
  bVar4 = false;
  bVar5 = false;
  bVar6 = false;
  bVar7 = false;
  *(undefined1 *)(iVar16 + 0x44) = 0;
  *(undefined1 *)(iVar16 + 0x45) = 0;
  *(undefined2 *)(iVar16 + 0x30) = 0;
  *(undefined2 *)(iVar16 + 0x2e) = 0;
  *(undefined2 *)(iVar16 + 0x2c) = 0;
  *(undefined2 *)(iVar16 + 0x2a) = 0;
  *(undefined4 *)(iVar16 + 0x34) = 0;
  if (0 < *(short *)(iVar16 + 6)) {
    do {
      iVar12 = (short)(*(short *)(iVar16 + 4) + sVar10) * 0x20 + DAT_008802cc;
      sVar10 = sVar10 + 1;
      *(undefined2 *)(iVar12 + 0x1a) = 0;
      *(undefined2 *)(iVar12 + 0x18) = 0;
      *(undefined4 *)(iVar12 + 0x1c) = 0;
    } while (sVar10 < *(short *)(iVar16 + 6));
  }
  sVar10 = 0;
  if (0 < *(short *)(iVar16 + 10)) {
    do {
      iVar12 = (short)(*(short *)(iVar16 + 8) + sVar10) * 0x10 + DAT_008802c4;
      sVar10 = sVar10 + 1;
      *(undefined2 *)(iVar12 + 8) = 0;
      *(undefined2 *)(iVar12 + 6) = 0;
      *(undefined4 *)(iVar12 + 0xc) = 0;
    } while (sVar10 < *(short *)(iVar16 + 10));
  }
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (param_1 == 0xffffffff) {
      local_4 = *(uint *)(DAT_00880354 + 8);
    }
    else {
      local_4 = *(uint *)(iVar14 + 0x14 + *(int *)(iVar13 + 0x34));
    }
  }
  while ((iVar13 = DAT_008802c4, *(char *)(DAT_00880354 + 1) != '\0' && (local_4 != 0xffffffff))) {
    uVar11 = local_4 & 0xffff;
    iVar12 = uVar11 * 0x724;
    local_4 = *(uint *)(iVar12 + 0x2c + *(int *)(DAT_00880360 + 0x34));
    iVar12 = iVar12 + *(int *)(DAT_00880360 + 0x34);
    iVar14 = (short)(*(short *)(iVar12 + 0x3a) + *(short *)(iVar16 + 4)) * 0x20 + DAT_008802cc;
    if (*(uint *)(iVar12 + 0x18) == 0xffffffff) {
      sVar10 = *(short *)(iVar12 + 0x1e);
      fVar2 = (float)(int)sVar10 / (float)(int)*(short *)(iVar12 + 0x20);
    }
    else {
      fVar2 = *(float *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                 (*(uint *)(iVar12 + 0x18) & 0xffff) * 0xc) + 0xe0);
      sVar10 = 1;
    }
    if (*(short *)(iVar12 + 0x3c) != -1) {
      iVar15 = (short)(*(short *)(iVar16 + 8) + *(short *)(iVar12 + 0x3c)) * 0x10;
      psVar1 = (short *)(iVar15 + 6 + DAT_008802c4);
      *psVar1 = *psVar1 + sVar10;
      iVar15 = iVar15 + iVar13;
      bVar8 = *(byte *)(iVar12 + 6);
      *(float *)(iVar15 + 0xc) = fVar2 + *(float *)(iVar15 + 0xc);
      *(short *)(iVar15 + 8) = *(short *)(iVar15 + 8) + (ushort)bVar8 * sVar10;
    }
    *(short *)(iVar14 + 0x18) = *(short *)(iVar14 + 0x18) + sVar10;
    bVar8 = *(byte *)(iVar12 + 6);
    *(float *)(iVar14 + 0x1c) = fVar2 + *(float *)(iVar14 + 0x1c);
    *(short *)(iVar14 + 0x1a) = *(short *)(iVar14 + 0x1a) + (ushort)bVar8 * sVar10;
    *(short *)(iVar16 + 0x2a) = *(short *)(iVar16 + 0x2a) + sVar10;
    *(short *)(iVar16 + 0x2c) = *(short *)(iVar16 + 0x2c) + (ushort)*(byte *)(iVar12 + 6) * sVar10;
    iVar13 = uVar11 * 0x724;
    iVar14 = *(int *)(DAT_00880360 + 0x34) + iVar13;
    if ((*(short *)(iVar14 + 0x6a) == 3) && (*(short *)(iVar14 + 0x72) < *(short *)(iVar14 + 0x6e)))
    {
      bVar8 = 1;
    }
    else {
      bVar8 = 0;
    }
    *(short *)(iVar16 + 0x2e) = *(short *)(iVar16 + 0x2e) + (ushort)bVar8 * sVar10;
    iVar13 = *(int *)(DAT_00880360 + 0x34) + iVar13;
    bVar9 = 6 < *(short *)(iVar13 + 0x6e);
    if (((bVar9) && (*(short *)(iVar13 + 0x6c) == 4)) && (0 < *(short *)(iVar13 + 0xa8))) {
      bVar9 = false;
    }
    *(float *)(iVar16 + 0x34) = fVar2 + *(float *)(iVar16 + 0x34);
    *(short *)(iVar16 + 0x30) = *(short *)(iVar16 + 0x30) + (ushort)bVar9 * sVar10;
    if (*(uint *)(iVar12 + 0x270) != 0xffffffff) {
      iVar13 = (*(uint *)(iVar12 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
      *(undefined1 *)(iVar16 + 0x43) = 1;
      sVar10 = *(short *)(iVar12 + 0x3e);
      sVar3 = *(short *)(iVar13 + 0x12);
      if (((sVar10 < 0) || (9 < sVar10)) ||
         ((sVar3 < 0 ||
          ((9 < sVar3 ||
           (iVar14 = (int)sVar3 + sVar10 * 10,
           (*(uint *)(DAT_006b0b84 + 0x94 + (iVar14 >> 5) * 4) & 1 << ((byte)iVar14 & 0x1f)) == 0)))
          ))) {
        bVar4 = true;
      }
      if (*(char *)(iVar12 + 0x8c) != '\0') {
        bVar6 = true;
      }
      if (*(char *)(iVar12 + 0x8d) != '\0') {
        bVar7 = true;
      }
      if (*(short *)(iVar12 + 0x6e) < 7) {
        if ((*(short *)(iVar13 + 0x24) < 2) || (3 < *(short *)(iVar13 + 0x24))) {
          bVar8 = *(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                    (*(uint *)(iVar13 + 0x18) & 0xffff) * 0xc) + 0x106) & 4;
        }
        else {
          bVar8 = *(byte *)(iVar13 + 0x127);
        }
        if (bVar8 != 0) goto LAB_00437c58;
      }
      else {
        *(undefined1 *)(iVar16 + 0x45) = 1;
      }
      *(undefined1 *)(iVar16 + 0x44) = 1;
    }
LAB_00437c58:
    if (0 < *(short *)(iVar12 + 0x1e4)) {
      bVar5 = true;
    }
  }
  if (bVar4) {
    *(undefined1 *)(iVar16 + 0x46) = 0;
  }
  iVar13 = DAT_006f1d6c;
  if (((*(char *)(iVar16 + 0x45) == '\0') &&
      ((iVar14 = *(int *)(iVar16 + 0x50), iVar14 == -1 || (0x3b < iVar14)))) &&
     (((*(char *)(iVar16 + 0x44) == '\0' &&
       ((*(int *)(iVar16 + 0x54) == -1 || (0x3b < *(int *)(iVar16 + 0x54))))) ||
      ((iVar14 == -1 || (0x1c1 < iVar14)))))) {
    if (*(char *)(iVar16 + 0x42) == '\0') {
      if (*(char *)(iVar16 + 0x47) == '\0') {
        if ((bVar6) && (bVar7)) {
          FUN_00438580(param_1);
          goto LAB_00437d04;
        }
      }
      else {
        *(bool *)(iVar16 + 0x48) = !bVar5;
        if (*(short *)(iVar16 + 0x4a) != 0) goto LAB_00437d04;
      }
      FUN_004382b0();
    }
    else {
      *(undefined1 *)(iVar16 + 0x47) = 0;
      *(undefined2 *)(iVar16 + 0x1a) = *(undefined2 *)(iVar16 + 0x2a);
      *(undefined4 *)(iVar16 + 0x58) = *(undefined4 *)(iVar13 + 0xc);
      *(undefined2 *)(iVar16 + 0x4c) = 0;
      if (*(char *)(iVar16 + 0x43) == '\0') {
        *(undefined4 *)(iVar16 + 0x50) = 0xffffffff;
        *(undefined4 *)(iVar16 + 0x54) = 0xffffffff;
      }
    }
  }
  else {
    *(undefined1 *)(iVar16 + 0x42) = 0;
    *(undefined1 *)(iVar16 + 0x47) = 0;
  }
LAB_00437d04:
  if (0 < *(short *)(iVar16 + 0x18)) {
    fVar2 = *(float *)(iVar16 + 0x34) / (float)(int)*(short *)(iVar16 + 0x18) - 0.001;
    if (fVar2 < 0.0) {
      fVar2 = 0.0;
    }
    *(float *)(iVar16 + 0x34) = fVar2;
  }
  sVar10 = 0;
  if (0 < *(short *)(iVar16 + 6)) {
    do {
      iVar13 = (short)(*(short *)(iVar16 + 4) + sVar10) * 0x20 + DAT_008802cc;
      fVar2 = *(float *)(iVar13 + 0x1c) / (float)(int)*(short *)(iVar13 + 0x16) - 0.001;
      if (fVar2 < 0.0) {
        fVar2 = 0.0;
      }
      sVar10 = sVar10 + 1;
      *(float *)(iVar13 + 0x1c) = fVar2;
    } while (sVar10 < *(short *)(iVar16 + 6));
  }
  sVar10 = 0;
  if (0 < *(short *)(iVar16 + 10)) {
    do {
      iVar13 = (short)(*(short *)(iVar16 + 8) + sVar10) * 0x10 + DAT_008802c4;
      fVar2 = *(float *)(iVar13 + 0xc) / (float)(int)*(short *)(iVar13 + 4) - 0.001;
      if (fVar2 < 0.0) {
        fVar2 = 0.0;
      }
      sVar10 = sVar10 + 1;
      *(float *)(iVar13 + 0xc) = fVar2;
    } while (sVar10 < *(short *)(iVar16 + 10));
  }
  *(undefined1 *)(iVar16 + 0x28) = 0;
  return;
}
#endif
