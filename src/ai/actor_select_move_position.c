// actor_select_move_position  (Ghidra: actor_select_move_position, renamed)
// address 0x4014c0, size 744 bytes
// name confidence: 0.4   rewrite confidence: 0.9
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

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern Scenario *global_scenario;   // 0x00746f8c
extern game_time_globals *game_time; // 0x006f1d6c

extern int32_t ai_weighted_random_index(int16_t weight_offset, void *base, int16_t stride, uint16_t count,
    uint32_t *exclude_mask); // 0x432100, EDX, stack

// REWRITTEN from objdump 0x4014c0..0x4017a7. EAX: actor; stack: (mode, current index, direction byte *). Picks one
//   of the actor's squad move positions (ScenarioSquad +0xc4 count / +0xc8 block, 0x50 each). A position is taken
//   out (mask bit) when it is the current one, within 0.5 of the actor, of another group letter (+0x1e vs actor
//   +0x68) or within 0.5 of an ally prop (kind 2..3, +0xbc). None left: -1. Mode 5 picks by weight (+0x10); the
//   others step from the current index -- forward (mode 2 / default), alternating (4: the tick's low bit) or
//   ping-pong (3: the direction byte, reversed at the ends) -- to the next unmasked one. Mode 1 keeps a valid
//   current index. The draft passed no weight offset to the weighted pick.
// blam-cc: EAX -> actor_index, stack -> select_mode, position_index, direction_flag
int32_t actor_select_move_position(uint32_t actor_index, int16_t select_mode, int32_t position_index,
    uint8_t *direction_flag)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;       // ebp
    uint8_t *squad;                                                                   // esi
    uint8_t *positions;                                                               // [esp+0x20]
    int32_t count;                                                                    // [esp+0x24]
    uint32_t mask = 0;                                                                // [esp+0x1c]
    uint8_t found = 0;                                                                // [esp+0x13]
    int16_t current = (int16_t)position_index;
    int16_t i;
    int16_t index;

    if (a[0x160] || select_mode == 0) {
        return -1;
    }
    if (((actor *)a)->encounter_index == k_datum_index_none) {
        return -1;
    }
    squad = *(uint8_t **)(*(uint8_t **)((uint8_t *)global_scenario + 0x430) +
        (((actor *)a)->encounter_index & 0xffff) * 0xb0 + 0x84) + ((actor *)a)->squad_index * 0xe8;
    if (select_mode == 1 && current != -1) {
        return position_index;
    }
    count = *(int32_t *)(squad + 0xc4);
    if (count <= 0) {
        return -1;
    }
    positions = *(uint8_t **)(squad + 0xc8);
    for (i = 0; (int32_t)i < count; i++) {
        float *pos = (float *)(positions + i * 0x50);
        uint8_t eligible = (i != current);                                            // bl
        uint8_t occupied = 0;
        datum_index prop_index;

        if (current != -1) {
            float dx = pos[0] - ((actor *)a)->body_position.x;
            float dy = pos[1] - ((actor *)a)->body_position.y;
            float dz = pos[2] - ((actor *)a)->body_position.z;

            if (!(dz * dz + dy * dy + dx * dx >= 0.25f)) {
                eligible = 0;
            }
        }
        if (((uint8_t *)pos)[0x1e] && ((uint8_t *)pos)[0x1e] != a[0x68]) {
            eligible = 0;
        }
        for (prop_index = ((actor *)a)->first_prop; prop_index != k_datum_index_none;) {
            uint8_t *pr = (uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138;
            int16_t kind = *(int16_t *)(pr + 0x24);

            prop_index = *(datum_index *)(pr + 0x8);
            if (kind >= 2 && kind <= 3) {
                float dx = pos[0] - *(float *)(pr + 0xbc);
                float dy = pos[1] - *(float *)(pr + 0xc0);
                float dz = pos[2] - *(float *)(pr + 0xc4);

                if (!(dz * dz + dy * dy + dx * dx >= 0.25f)) {
                    occupied = 1;
                    break;
                }
            }
        }
        if (occupied || !eligible) {
            (&mask)[i >> 5] |= 1u << (i & 0x1f);
        } else {
            found = 1;
        }
    }
    if (!found) {
        return -1;
    }
    if (select_mode == 5) {
        return ai_weighted_random_index(0x10, positions, 0x50, (uint16_t)*(int32_t *)(squad + 0xc4), &mask);
    }
    index = current;
    if (index < 0 || (int32_t)index >= count) {
        index = 0;
    }
    do {
        uint8_t forward = 1;                                                          // cl
        uint8_t store = 1;

        if (select_mode == 2) {
            forward = 1;
        } else if (select_mode == 3) {
            if (index == 0) {
                forward = 1;
            } else if ((int32_t)index == *(int32_t *)(squad + 0xc4) - 1) {
                forward = 0;
            } else if (direction_flag != 0) {
                forward = *direction_flag;
            } else {
                store = 0;                                                            // 0x401744: straight on
            }
        } else if (select_mode == 4) {
            forward = (uint8_t)(game_time->game_time & 1);
        }
        if (store && direction_flag != 0) {
            *direction_flag = forward;
        }
        if (forward) {
            index++;
            if ((int32_t)index >= *(int32_t *)(squad + 0xc4)) {
                index = 0;
            }
        } else {
            index--;
            if (index < 0) {
                index = (int16_t)(*(int32_t *)(squad + 0xc4) - 1);
            }
        }
    } while ((&mask)[index >> 5] & (1u << (index & 0x1f)));
    return index;
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
