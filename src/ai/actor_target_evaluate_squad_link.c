// actor_target_evaluate_squad_link  (Ghidra: actor_target_evaluate_squad_link; named from out/phase2/results/ai_02.json)
// address 0x41e320, size 1849 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// REWRITTEN from objdump 0x41e320..0x41ea58 (the draft was a goto transliteration with every helper called without
//   arguments). Walks an object list (next +0x114, recursing into first children +0x118), visiting each object once
//   per scan (object +0x14 against object_cluster_stamp 0x8603cc), and sorts what the actor perceives:
//   - a biped (type 0): the target is the object, or for a swarm unit (+0x1f8) the member nearest the actor's
//     firing block (0x41c2c0); the actor itself is skipped. With the unit tag's danger radius (+0x284) and the unit
//     firing (+0x106 bit 2 without +0x420) or meleeing (state 0x1e) it registers a danger point (0x41ec90). A unit
//     with no controlling player (+0x218) whose actor is asleep/inactive, or further than 40 (squared 1600), is
//     skipped; a firing unit counts only when it fired after the encounter's/actor's reference time (+0x58/+0x3a0)
//     and, with an encounter not in combat (+0x42/+0x44/+0x45), within 15; otherwise units need a danger radius, or
//     (enemies only) to have fired within 150 ticks, the actor's combat grade at most 1, and to be within 4 (8 for a
//     calm, +0x6a < 3, friend). Friends further than 15 are dropped; enemies further than 6 and friends that are
//     far off (+0x6e >= 4, or unalerted +0x1cc beyond 4) go to the list's far entries, the rest become props
//     (actor_find_or_allocate_prop + actor_target_data_refresh), counted unless firing;
//   - a vehicle (type 1) with no driver (+0x324) is a stationary danger (0x41ea60);
//   - a projectile (type 5) whose tag danger radius (+0x1a8) plus 10 reaches the actor becomes the actor's danger
//     (+0x280 kind 2) unless one closer is already held, with the source object (+0xc4) as its owner (+0x282: 2 its
//     own unit, 1 a friend).
//   Lists (enemies: candidates_a, friends: candidates_b): +0 prop count, +2 far count, +4 far entries of 12 bytes
//   {object, -1, distance squared}, at most 0x80.
// blam-cc: stack -> actor_index, object_index, candidates_a, candidates_b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>

extern data_array *actor_data;       // 0x00880360
extern data_array *object_data;      // 0x008603b0
extern data_array *encounter_data;   // 0x008802c8
extern int32_t object_cluster_stamp; // 0x008603cc
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern double sqrt(double x);

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, EAX, ECX, EDX
extern datum_index object_find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index,
    char stamp_group); // 0x41c2c0, EAX, stack
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX
extern uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index, float radius,
    float distance, char accept_flag, uint8_t unknown_byte); // 0x41ec90, EAX, EDX, stack
extern uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index,
    datum_index object_index, uint8_t unknown_byte); // 0x41ea60, EAX, stack
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, EAX
extern datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind); // 0x43e270
extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force,
    char allow_reassign); // 0x41c4b0

#define OBJ(i) ((uint8_t *)((object_header *)object_data->data)[(i) & 0xffff].data)

static void squad_link_add_far(uint8_t *list, datum_index object_index, float distance_squared)
{
    int16_t count = *(int16_t *)(list + 2);
    uint8_t *entry;

    if (count >= 0x80) {
        return;
    }
    entry = list + 4 + count * 0xc;
    *(datum_index *)(entry + 0) = object_index;
    *(int32_t *)(entry + 4) = -1;
    *(float *)(entry + 8) = distance_squared;
    *(int16_t *)(list + 2) = (int16_t)(count + 1);
}

// 0x41e3b1..0x41e7ed
static void squad_link_evaluate_biped(uint32_t actor_index, uint8_t *self, datum_index object_index, uint8_t *object,
    uint8_t *list_enemy, uint8_t *list_friend)
{
    real_point3d position;
    uint32_t block[14];
    real_point3d *block_point = (real_point3d *)&block[3];
    datum_index target = object_index;
    datum_index target_actor_index;
    uint8_t *unit = object;
    uint8_t *unit_tag;
    uint8_t *target_actor = 0;
    uint8_t controlled;
    uint8_t enemies;
    uint8_t firing;
    uint8_t far_flag = 0;
    int16_t since_fired;
    float radius;
    float distance_squared;
    uint8_t *list;

    object_get_position(&position, object_index);
    actor_get_firing_positions(actor_index, block, &position);
    if (*(datum_index *)(object + 0x1f8) != k_datum_index_none) {
        target_actor_index = *(datum_index *)(object + 0x1f8);
        target = object_find_nearest_squad_member(target_actor_index, block, k_datum_index_none, 1);
        if (target == k_datum_index_none) {
            return;
        }
        unit = OBJ(target);
        object_get_position(&position, target);
    } else {
        target_actor_index = *(datum_index *)(object + 0x1f4);
    }
    if (target_actor_index == actor_index) {
        return;
    }

    unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    controlled = ((unit_object *)unit)->unit.controlling_player != k_datum_index_none;
    enemies = teams_are_enemies(((unit_object *)unit)->base.owner_team, ((actor *)self)->team);
    if ((unit[0x106] & 4) != 0 && ((struct unit_object *)unit)->unit.unknown_420 == 0) {
        int32_t fired = ((struct unit_object *)unit)->unit.unknown_41c;

        firing = 1;
        since_fired = fired == -1 ? 0x7fff : (int16_t)((int16_t)game_time->game_time - (int16_t)fired);
    } else {
        firing = 0;
        since_fired = 0;
    }
    radius = *(float *)(unit_tag + 0x284);
    {
        float dx = position.x - block_point->x;
        float dy = position.y - block_point->y;
        float dz = position.z - block_point->z;

        distance_squared = dz * dz + dy * dy + dx * dx;
    }
    if (radius > 0.0f && (firing || (int8_t)unit[0x2a3] == 0x1e)) {
        actor_danger_register_point(actor_index, target, radius, (float)sqrt((double)distance_squared), (char)enemies, 0);
    }
    if (target_actor_index != k_datum_index_none) {
        target_actor = (uint8_t *)actor_data->data + (target_actor_index & 0xffff) * 0x724;
    }

    if (!controlled) {
        if (target_actor != 0 && (target_actor[8] == 0 || target_actor[0x13] != 0)) {
            return;
        }
        if (distance_squared > 1600.0f) {
            return;
        }
        if (firing) {
            datum_index encounter_index = ((actor *)self)->encounter_index;

            if (encounter_index == k_datum_index_none) {
                goto check_radius;
            } else {
                uint8_t *encounter = (uint8_t *)encounter_data->data + (encounter_index & 0xffff) * 0x6c;
                uint8_t *target_unit = OBJ(target);
                int32_t reference = ((struct encounter *)encounter)->unknown_58;
                uint8_t counts = 1;
                uint8_t calm;

                if (!(reference > *(int32_t *)&((struct actor *)self)->unknown_3a0)) {
                    reference = *(int32_t *)&((struct actor *)self)->unknown_3a0;
                }
                if (reference != -1) {
                    int32_t fired = *(int32_t *)(target_unit + 0x41c);

                    if (fired == -1 || fired < reference) {
                        counts = 0;
                    }
                }
                calm = encounter[0x45] == 0 && encounter[0x44] == 0 && encounter[0x42] == 0;
                if (!counts) {
                    return;
                }
                if (!calm) {
                    goto check_radius;
                }
                if (!(distance_squared < 225.0f)) {
                    return;
                }
            }
        } else if (enemies) {
            far_flag = distance_squared > 36.0f;
        } else {
            // 0x41e756: the friend's far flag is computed in AL and used directly at 0x41e66c
            uint8_t near = distance_squared < 225.0f;

            if (((struct actor *)self)->unknown_6e >= 4) {
                far_flag = 1;
            } else {
                far_flag = self[0x1cc] == 0 && distance_squared > 16.0f;
            }
            if (!near) {
                return;
            }
            list = list_friend;
            goto add;
        }
        goto add_by_team;

check_radius:
        // 0x41e6b7
        if (!(radius > 0.0f)) {
            float limit;

            if (enemies && since_fired > 0x96) {
                return;
            }
            if (actor_get_current_mode_combat_grade(actor_index) > 1) {
                return;
            }
            limit = 16.0f;
            if (!enemies && ((actor *)self)->awareness_level < 3) {
                limit = 64.0f;
            }
            if (!(distance_squared < limit)) {
                return;
            }
        }
    }

add_by_team:
    list = enemies ? list_enemy : list_friend;
add:
    if (far_flag) {
        squad_link_add_far(list, target, distance_squared);
        return;
    }
    {
        datum_index prop_index = actor_find_or_allocate_prop(actor_index, target, (char)enemies);

        if (prop_index == k_datum_index_none) {
            return;
        }
        actor_target_data_refresh(actor_index, prop_index, block, 0, 0);
        if (!firing) {
            *(int16_t *)list = (int16_t)(*(int16_t *)list + 1);
        }
    }
}

// 0x41e829..0x41ea05
static void squad_link_evaluate_projectile(uint32_t actor_index, uint8_t *self, datum_index object_index, uint8_t *object)
{
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
    float radius = *(float *)(tag + 0x1a8);
    real_point3d position;
    uint32_t block[14];
    real_point3d *block_point = (real_point3d *)&block[3];
    float distance;
    datum_index owner;
    datum_index owner_unit = k_datum_index_none;

    if (!(radius > 0.0f)) {
        return;
    }
    if (((struct object *)object)->parent_object != k_datum_index_none && (object[0x22c] & 0x20) == 0) {
        return;
    }
    object_get_position(&position, object_index);
    actor_get_firing_positions(actor_index, block, &position);
    {
        float dx = position.x - block_point->x;
        float dy = position.y - block_point->y;
        float dz = position.z - block_point->z;

        distance = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
    }
    if (!(radius + 10.0f > distance)) {
        return;
    }
    if (((actor *)self)->danger_type >= 2) {
        if (((actor *)self)->danger_type != 2 || ((actor *)self)->danger_object_index == object_index ||
            !(distance < ((actor *)self)->danger_unknown_2d4)) {
            return;
        }
    }
    memset(self + 0x280, 0, 0x6c);
    ((actor *)self)->danger_type = 2;
    ((actor *)self)->danger_object_index = object_index;
    ((actor *)self)->danger_unknown_294 = radius;
    *(real_point3d *)&((actor *)self)->danger_unknown_298 = position;
    *(real_vector3d *)&((actor *)self)->danger_unknown_2a4 = *(real_vector3d *)&((struct object *)object)->velocity.i;
    ((actor *)self)->danger_unknown_284 = 0x1e;
    self[0x286] = 0;
    ((actor *)self)->danger_unknown_282 = 0;
    owner = ((struct object *)object)->creator_object;
    if (owner != k_datum_index_none) {
        uint8_t *owner_object = (uint8_t *)object_try_and_get(owner, 0xffffffff);

        if (owner_object != 0 && ((1u << owner_object[0xb4]) & 3) != 0) {
            owner_unit = owner;
            if (((actor *)self)->unit_index != k_datum_index_none && owner == ((actor *)self)->unit_index) {
                ((actor *)self)->danger_unknown_282 = 2;
            } else if (!teams_are_enemies(((struct object *)object)->owner_team, ((actor *)self)->team)) {
                ((actor *)self)->danger_unknown_282 = 1;
            }
        }
    }
    *(datum_index *)&((actor *)self)->danger_unknown_290 = owner_unit;
}

void actor_target_evaluate_squad_link(uint32_t actor_index, datum_index object_index, int16_t *candidates_a,
    int16_t *candidates_b)
{
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    while (object_index != k_datum_index_none) {
        uint8_t *object = OBJ(object_index);

        if (((struct object *)object)->cluster_stamp != object_cluster_stamp) {
            ((struct object *)object)->cluster_stamp = object_cluster_stamp;
            switch (((struct object *)object)->type) {
            case 0:
                squad_link_evaluate_biped(actor_index, self, object_index, object, (uint8_t *)candidates_a,
                    (uint8_t *)candidates_b);
                break;
            case 1:
                if (*(datum_index *)(object + 0x324) == k_datum_index_none) {
                    actor_danger_register_stationary_object(0, actor_index, object_index, 0);
                }
                break;
            case 5:
                squad_link_evaluate_projectile(actor_index, self, object_index, object);
                break;
            default:
                break;
            }
        }
        if (((struct object *)object)->first_child_object != k_datum_index_none) {
            actor_target_evaluate_squad_link(actor_index, ((struct object *)object)->first_child_object, candidates_a, candidates_b);
        }
        object_index = ((struct object *)object)->next_object;
    }
}

#if 0
Original Ghidra decompilation (0x41e320):

void FUN_0041e320(uint param_1,uint param_2,short *param_3,short *param_4)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  char cVar8;
  short sVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  uint *puVar15;
  short *psVar16;
  uint uVar17;
  int iVar18;
  undefined4 *puVar19;
  uint uVar20;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  undefined1 local_70 [12];
  float local_64;
  float local_60;
  float local_5c;
  float local_2c;
  float local_28;
  float local_24;

  iVar10 = (param_1 & 0xffff) * 0x724;
  iVar13 = *(int *)(DAT_00880360 + 0x34) + iVar10;
  do {
    if (param_2 == 0xffffffff) {
      return;
    }
    iVar11 = (param_2 & 0xffff) * 0xc;
    puVar1 = *(uint **)(iVar11 + 8 + *(int *)(DAT_008603b0 + 0x34));
    if (puVar1[5] != DAT_008603cc) {
      *(uint *)(*(int *)(iVar11 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x14) = DAT_008603cc;
      sVar9 = (short)puVar1[0x2d];
      if (sVar9 == 0) {
        object_get_position();
        actor_get_firing_positions();
        uVar17 = puVar1[0x7e];
        if (uVar17 == 0xffffffff) {
          uVar17 = puVar1[0x7d];
          puVar15 = puVar1;
        }
        else {
          param_2 = FUN_0041c2c0(local_70,0xffffffff,1);
          if (param_2 == 0xffffffff) goto LAB_0041ea0b;
          puVar15 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
          object_get_position();
        }
        if ((param_2 != 0xffffffff) && (uVar17 != param_1)) {
          uVar20 = puVar15[0x86];
          iVar11 = *(int *)((*puVar15 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          cVar8 = FUN_0045bd50();
          if (((*(byte *)((int)puVar15 + 0x106) & 4) == 0) || ((short)puVar15[0x108] != 0)) {
            bVar4 = false;
            bVar6 = false;
            sVar9 = 0;
          }
          else {
            bVar4 = true;
            bVar6 = true;
            if (puVar15[0x107] == 0xffffffff) {
              sVar9 = 0x7fff;
            }
            else {
              sVar9 = *(short *)(DAT_006f1d6c + 0xc) - (short)puVar15[0x107];
            }
          }
          fVar2 = *(float *)(iVar11 + 0x284);
          fVar3 = (local_7c - local_64) * (local_7c - local_64) +
                  (local_78 - local_60) * (local_78 - local_60) +
                  (local_74 - local_5c) * (local_74 - local_5c);
          if ((0.0 < fVar2) && ((bVar4 || (*(char *)((int)puVar15 + 0x2a3) == '\x1e')))) {
            FUN_0041ec90(fVar2,SQRT(fVar3),cVar8,0);
            bVar4 = bVar6;
          }
          iVar11 = *(int *)(DAT_00880360 + 0x34);
          if (uVar17 == 0xffffffff) {
            iVar18 = 0;
          }
          else {
            iVar18 = (uVar17 & 0xffff) * 0x724 + iVar11;
          }
          bVar7 = false;
          if (uVar20 != 0xffffffff) goto LAB_0041e652;
          if (((iVar18 == 0) ||
              ((*(char *)(iVar18 + 8) != '\0' && (*(char *)(iVar18 + 0x13) == '\0')))) &&
             (fVar3 <= 1600.0)) {
            if (bVar4) {
              uVar17 = *(uint *)(iVar10 + 0x34 + iVar11);
              bVar4 = true;
              if (uVar17 == 0xffffffff) {
LAB_0041e6b7:
                if (0.0 < fVar2) {
LAB_0041e652:
                  psVar16 = param_3;
                  if (cVar8 == '\0') goto LAB_0041e665;
                  goto LAB_0041e66c;
                }
                if (((cVar8 == '\0') || (sVar9 < 0x97)) && (sVar9 = FUN_0040e760(), sVar9 < 2)) {
                  fVar2 = 16.0;
                  if ((cVar8 == '\0') && (*(short *)(iVar10 + 0x6a + iVar11) < 3)) {
                    fVar2 = 64.0;
                  }
                  if (fVar3 < fVar2) goto LAB_0041e652;
                }
              }
              else {
                iVar12 = (uVar17 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
                iVar18 = *(int *)(iVar10 + 0x3a0 + iVar11);
                iVar14 = *(int *)(iVar12 + 0x58);
                if (*(int *)(iVar12 + 0x58) <= iVar18) {
                  iVar14 = iVar18;
                }
                if ((iVar14 != -1) &&
                   ((iVar18 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (param_2 & 0xffff) * 0xc) + 0x41c), iVar18 == -1 ||
                    (iVar18 < iVar14)))) {
                  bVar4 = false;
                }
                if (((*(char *)(iVar12 + 0x45) == '\0') && (*(char *)(iVar12 + 0x44) == '\0')) &&
                   (*(char *)(iVar12 + 0x42) == '\0')) {
                  bVar5 = true;
                }
                else {
                  bVar5 = false;
                }
                if (bVar4) {
                  if (!bVar5) goto LAB_0041e6b7;
                  if (fVar3 < 225.0) goto LAB_0041e652;
                }
              }
            }
            else {
              if (cVar8 != '\0') {
                if (fVar3 <= 36.0) {
                  bVar7 = false;
                }
                else {
                  bVar7 = true;
                }
                goto LAB_0041e652;
              }
              if (*(short *)(iVar10 + 0x6e + iVar11) < 4) {
                if ((*(char *)(iVar10 + 0x1cc + iVar11) != '\0') || (fVar3 <= 16.0)) {
                  bVar7 = false;
                }
                else {
                  bVar7 = true;
                }
              }
              else {
                bVar7 = true;
              }
              if (225.0 <= fVar3) goto LAB_0041ea0b;
LAB_0041e665:
              psVar16 = param_4;
LAB_0041e66c:
              if (bVar7) {
                sVar9 = psVar16[1];
                if (sVar9 < 0x80) {
                  (psVar16 + sVar9 * 6 + 4)[0] = -1;
                  (psVar16 + sVar9 * 6 + 4)[1] = -1;
                  *(uint *)(psVar16 + psVar16[1] * 6 + 2) = param_2;
                  *(float *)(psVar16 + (psVar16[1] + 1) * 6) = fVar3;
                  psVar16[1] = psVar16[1] + 1;
                }
              }
              else {
                iVar11 = FUN_0043e270(param_1,param_2,cVar8);
                if ((iVar11 != -1) && (FUN_0041c4b0(param_1,iVar11,local_70,0,0), !bVar6)) {
                  *psVar16 = *psVar16 + 1;
                }
              }
            }
          }
        }
      }
      else if (sVar9 == 1) {
        if (puVar1[0xc9] == 0xffffffff) {
          FUN_0041ea60(param_1,param_2,0);
        }
      }
      else if (((sVar9 == 5) &&
               (iVar11 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
               0.0 < *(float *)(iVar11 + 0x1a8))) &&
              ((puVar1[0x47] == 0xffffffff || ((puVar1[0x8b] & 0x20) != 0)))) {
        object_get_position();
        actor_get_firing_positions();
        fVar2 = SQRT((local_88 - local_2c) * (local_88 - local_2c) +
                     (local_84 - local_28) * (local_84 - local_28) +
                     (local_80 - local_24) * (local_80 - local_24));
        if (fVar2 < *(float *)(iVar11 + 0x1a8) + 10.0) {
          if ((*(short *)(iVar13 + 0x280) < 2) ||
             (((*(short *)(iVar13 + 0x280) == 2 && (*(uint *)(iVar13 + 0x28c) != param_2)) &&
              (fVar2 < *(float *)(iVar13 + 0x2d4))))) {
            puVar19 = (undefined4 *)(iVar13 + 0x280);
            for (iVar18 = 0x1b; iVar18 != 0; iVar18 = iVar18 + -1) {
              *puVar19 = 0;
              puVar19 = puVar19 + 1;
            }
            *(undefined2 *)(iVar13 + 0x280) = 2;
            *(uint *)(iVar13 + 0x28c) = param_2;
            *(undefined4 *)(iVar13 + 0x294) = *(undefined4 *)(iVar11 + 0x1a8);
            *(float *)(iVar13 + 0x298) = local_88;
            *(float *)(iVar13 + 0x29c) = local_84;
            *(float *)(iVar13 + 0x2a0) = local_80;
            *(uint *)(iVar13 + 0x2a4) = puVar1[0x1a];
            *(uint *)(iVar13 + 0x2a8) = puVar1[0x1b];
            *(uint *)(iVar13 + 0x2ac) = puVar1[0x1c];
            *(undefined2 *)(iVar13 + 0x284) = 0x1e;
            *(undefined1 *)(iVar13 + 0x286) = 0;
            *(undefined2 *)(iVar13 + 0x282) = 0;
            uVar17 = puVar1[0x31];
            uVar20 = 0xffffffff;
            if (((uVar17 != 0xffffffff) && (iVar11 = object_try_and_get(0xffffffff), iVar11 != 0))
               && ((1 << (*(byte *)(iVar11 + 0xb4) & 0x1f) & 3U) != 0)) {
              uVar20 = uVar17;
              if ((*(uint *)(iVar13 + 0x18) == 0xffffffff) || (uVar17 != *(uint *)(iVar13 + 0x18)))
              {
                cVar8 = FUN_0045bd50();
                if (cVar8 == '\0') {
                  *(undefined2 *)(iVar13 + 0x282) = 1;
                }
              }
              else {
                *(undefined2 *)(iVar13 + 0x282) = 2;
              }
            }
            *(uint *)(iVar13 + 0x290) = uVar20;
          }
        }
      }
    }
LAB_0041ea0b:
    if (puVar1[0x46] != 0xffffffff) {
      FUN_0041e320(param_1,puVar1[0x46],param_3,param_4);
    }
    param_2 = puVar1[0x45];
  } while( true );
}
#endif
