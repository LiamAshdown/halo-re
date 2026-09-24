// actor_select_move_position  (Ghidra: actor_select_move_position, renamed)
// address 0x4014c0, size 744 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/ai.h actor.encounter_index/squad_index/order_committed/first_prop/
//   unknown_68, prop.kind/next_in_actor/last_known_position; types/tags.h Scenario.encounters,
//   ScenarioEncounter.squads, ScenarioSquad.move_positions (ScenarioMovePosition, stride
//   0x50); types/game.h game_time_globals.game_time (tick parity for select_mode 4).
// register convention: actor index in EAX; selection mode, current/target index and an
//   optional in/out "last direction" flag are ordinary stack parameters.
//   // blam-cc: EAX -> actor_index, stack -> select_mode, position_index, direction_flag
// UNSURE: actor+0x12c/0x130/0x134 fall inside actor.mode_data (a per-mode union, see
//   types/ai.h); read here as a cached float position but not independently confirmed.
//   UNSURE: actor.unknown_68 compared against ScenarioMovePosition.sequence_id -- likely a
//   per-formation-slot "sequence" filter, name not recovered.
//   UNSURE: ai_weighted_random_index's exact register mapping (Ghidra shows an unaccounted-for in_DX read
//   alongside its stack parameters); it is outside this session's address range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern Scenario *global_scenario;    // 0x00746f8c
extern game_time_globals *game_time; // 0x006f1d6c

// 0x432100, not yet rewritten (outside this session's range): weighted-random pick over an
// array of move positions, skipping indices flagged in the exclude bitmask.
extern int32_t ai_weighted_random_index(ScenarioMovePosition *positions, int32_t stride, uint16_t count, uint32_t *exclude_mask);

// Selects a formation ("move position") slot from the actor's current squad. select_mode:
//   1 - use position_index directly if it is already a valid slot, otherwise search from 0
//   2 - the next free slot after position_index
//   3 - the slot before position_index
//   4 - forward or backward depending on the low bit of the current game tick
//   5 - a weighted-random free slot (delegates to ai_weighted_random_index)
// A slot claimed by a nearby squadmate (a prop of kind 2 or 3 within 0.5 units of the slot)
// is treated as taken. Returns the chosen index, or -1 if the actor has already committed to
// an order, has no encounter, has no move positions, or every slot is taken.
int32_t actor_select_move_position(uint32_t actor_index, int16_t select_mode, int32_t position_index, uint8_t *direction_flag)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    int32_t result;

    if (a->order_committed != 0 || select_mode == 0) {
        return -1;
    }

    result = -1;
    if (a->encounter_index != (datum_index)k_datum_index_none) {
        ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
        ScenarioEncounter *enc = &encounters[a->encounter_index & 0xffff];
        ScenarioSquad *squad = (ScenarioSquad *)enc->squads.pointer + a->squad_index;
        int16_t target = (int16_t)position_index;

        if (select_mode != 1 || (result = position_index, target == -1)) {
            uint8_t found_free = 0;
            // local_c in the original: [0]/[1] hold a 64-bit occupancy bitmask for the scan
            // below, but the array is only 3 dwords and slot [1]/[2] are reused as the move
            // positions pointer/count once the scan starts -- preserved exactly as decompiled.
            uint32_t scratch[3];
            int16_t cursor = 0;
            int32_t count;

            scratch[0] = 0;
            count = squad->move_positions.count;
            scratch[2] = (uint32_t)count;
            if (0 < count) {
                ScenarioMovePosition *positions = (ScenarioMovePosition *)squad->move_positions.pointer;
                int32_t i = 0;
                scratch[1] = (uint32_t)positions;
                do {
                    ScenarioMovePosition *p = &positions[i];
                    uint8_t candidate_ok = (cursor != target);

                    if (target != -1) {
                        float dx = p->position.x - a->body_position.x;
                        float dy = p->position.y - a->body_position.y;
                        float dz = p->position.z - a->body_position.z;
                        if (dx * dx + dy * dy + dz * dz < 0.25f) {
                            candidate_ok = 0;
                        }
                    }
                    if (p->sequence_id != 0 && p->sequence_id != (int8_t)a->unknown_68) {
                        candidate_ok = 0;
                    }

                    {
                        datum_index prop_index = a->first_prop;
                        uint8_t occupied = 0;
                        for (;;) {
                            prop *pr;
                            int16_t kind;
                            float dx, dy, dz;

                            if (prop_index == (datum_index)k_datum_index_none) {
                                if (candidate_ok) {
                                    found_free = 1;
                                    goto next_slot;
                                }
                                break;
                            }
                            pr = &((prop *)prop_data->data)[prop_index & 0xffff];
                            kind = pr->kind;
                            prop_index = pr->next_in_actor;
                            if (kind >= 2 && kind <= 3) {
                                dx = p->position.x - pr->last_known_position.x;
                                dy = p->position.y - pr->last_known_position.y;
                                dz = p->position.z - pr->last_known_position.z;
                                if (dx * dx + dy * dy + dz * dz < 0.25f) {
                                    occupied = 1;
                                    break;
                                }
                            }
                        }
                        if (occupied || !candidate_ok) {
                            scratch[i >> 5] |= 1u << (i & 0x1f);
                        }
                    }
                next_slot:
                    cursor++;
                    i = cursor;
                } while (i < count);

                if (found_free) {
                    uint8_t take;

                    if (select_mode == 5) {
                        return ai_weighted_random_index(positions, 0x50, (uint16_t)count, scratch);
                    }
                    if (target < 0 || count <= target) {
                        position_index = 0;
                    }
                    for (;;) {
                        take = 1;
                        if (select_mode == 2) {
                        forward:
                            take = 1;
                        report:
                            if (direction_flag != 0) {
                                *direction_flag = take;
                            }
                            if (take != 0) {
                                goto advance;
                            }
                            position_index--;
                            if ((int16_t)position_index < 0) {
                                position_index = count - 1;
                            }
                        } else if (select_mode == 3) {
                            if ((int16_t)position_index == 0) {
                                goto forward;
                            }
                            if ((int16_t)position_index == count - 1) {
                                take = 0;
                                goto report;
                            }
                            if (direction_flag != 0) {
                                take = *direction_flag;
                                goto report;
                            }
                            goto advance;
                        } else {
                            if (select_mode == 4) {
                                take = (uint8_t)(game_time->game_time & 1);
                            }
                            goto report;
                        }
                        goto check;
                    advance:
                        position_index++;
                        if (count <= (int16_t)position_index) {
                            position_index = 0;
                        }
                    check:
                        if ((scratch[(int16_t)position_index >> 5] & (1u << ((uint8_t)position_index & 0x1f))) == 0) {
                            return position_index;
                        }
                    }
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4014c0):

int FUN_004014c0(short param_1,int param_2,byte *param_3)

{
  short sVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  bool bVar5;
  int iVar6;
  short sVar7;
  short sVar8;
  uint in_EAX;
  int iVar9;
  int iVar10;
  int iVar11;
  byte bVar12;
  float *pfVar13;
  int iVar14;
  uint uVar15;
  bool bVar16;
  uint auStackY_100c [1013];
  uint local_c [3];

  iVar6 = DAT_006f1d6c;
  iVar9 = (in_EAX & 0xffff) * 0x724;
  iVar11 = iVar9 + *(int *)(DAT_00880360 + 0x34);
  if ((*(char *)(iVar9 + 0x160 + *(int *)(DAT_00880360 + 0x34)) != '\0') || (param_1 == 0)) {
    return -1;
  }
  iVar9 = -1;
  if (*(uint *)(iVar11 + 0x34) != 0xffffffff) {
    iVar14 = *(short *)(iVar11 + 0x3a) * 0xe8 +
             *(int *)((*(uint *)(iVar11 + 0x34) & 0xffff) * 0xb0 + 0x84 +
                     *(int *)(global_scenario + 0x430));
    sVar7 = (short)param_2;
    if ((param_1 != 1) || (iVar9 = param_2, sVar7 == -1)) {
      bVar5 = false;
      local_c[0] = 0;
      sVar8 = 0;
      local_c[2] = *(int *)(iVar14 + 0xc4);
      if (0 < *(int *)(iVar14 + 0xc4)) {
        local_c[1] = *(int *)(iVar14 + 200);
        iVar9 = 0;
        do {
          pfVar13 = (float *)(iVar9 * 0x50 + local_c[1]);
          bVar16 = sVar8 != sVar7;
          if ((sVar7 != -1) &&
             (fVar2 = *pfVar13 - *(float *)(iVar11 + 300),
             fVar4 = pfVar13[1] - *(float *)(iVar11 + 0x130),
             fVar3 = pfVar13[2] - *(float *)(iVar11 + 0x134),
             fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3 < 0.25)) {
            bVar16 = false;
          }
          if ((*(char *)((int)pfVar13 + 0x1e) != '\0') &&
             (*(char *)((int)pfVar13 + 0x1e) != *(char *)(iVar11 + 0x68))) {
            bVar16 = false;
          }
          uVar15 = *(uint *)(iVar11 + 0x50);
          do {
            if (uVar15 == 0xffffffff) {
              if (bVar16) {
                bVar5 = true;
                goto LAB_0040167e;
              }
              break;
            }
            iVar10 = (uVar15 & 0xffff) * 0x138;
            sVar1 = *(short *)(iVar10 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
            iVar10 = iVar10 + *(int *)(DAT_008802c0 + 0x34);
            uVar15 = *(uint *)(iVar10 + 8);
          } while (((sVar1 < 2) || (3 < sVar1)) ||
                  (fVar2 = *pfVar13 - *(float *)(iVar10 + 0xbc),
                  fVar4 = pfVar13[1] - *(float *)(iVar10 + 0xc0),
                  fVar3 = pfVar13[2] - *(float *)(iVar10 + 0xc4),
                  0.25 <= fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3));
          local_c[iVar9 >> 5] = local_c[iVar9 >> 5] | 1 << ((byte)iVar9 & 0x1f);
LAB_0040167e:
          sVar8 = sVar8 + 1;
          iVar9 = (int)sVar8;
        } while (iVar9 < (int)local_c[2]);
        if (bVar5) {
          if (param_1 == 5) {
            iVar11 = FUN_00432100(local_c[1],0x50,*(undefined2 *)(iVar14 + 0xc4),local_c);
            return iVar11;
          }
          if ((sVar7 < 0) || ((int)local_c[2] <= (int)sVar7)) {
            param_2 = 0;
          }
          do {
            bVar12 = 1;
            if (param_1 == 2) {
LAB_0040174a:
              bVar12 = 1;
LAB_0040174c:
              if (param_3 != (byte *)0x0) {
                *param_3 = bVar12;
              }
              if (bVar12 != 0) goto LAB_0040175a;
              param_2 = param_2 + -1;
              if ((short)param_2 < 0) {
                param_2 = CONCAT22((short)((uint)param_2 >> 0x10),*(short *)(iVar14 + 0xc4) + -1);
              }
            }
            else {
              if (param_1 != 3) {
                if (param_1 == 4) {
                  bVar12 = *(byte *)(iVar6 + 0xc) & 1;
                }
                goto LAB_0040174c;
              }
              if ((short)param_2 == 0) goto LAB_0040174a;
              if ((int)(short)param_2 == *(int *)(iVar14 + 0xc4) + -1) {
                bVar12 = 0;
                goto LAB_0040174c;
              }
              if (param_3 != (byte *)0x0) {
                bVar12 = *param_3;
                goto LAB_0040174c;
              }
LAB_0040175a:
              param_2 = param_2 + 1;
              if (*(int *)(iVar14 + 0xc4) <= (int)(short)param_2) {
                param_2 = 0;
              }
            }
            if ((local_c[(int)(short)param_2 >> 5] & 1 << ((byte)param_2 & 0x1f)) == 0) {
              return param_2;
            }
          } while( true );
        }
      }
      iVar9 = -1;
    }
  }
  return iVar9;
}
#endif
