// actor_score_blast_area_clear  (Ghidra: actor_score_blast_area_clear, renamed)
// address 0x410da0, size 979 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump 0x410da0..0x411172)
// evidence: phase-4 summary "scores hostiles within a blast radius of a point and checks
// no friendlies are within a safety radius, returning whether the spot is clear to throw
// at"; confirmed callers actor_can_throw_grenade_at_target (0x40d9c0),
// actor_target_is_visible_or_object_count_ok (0x40f700) and
// actor_validate_grenade_impact_point (0x410710) all pass (blast_radius, safety_radius,
// point, out_count) in that order, which is the full signature Ghidra recovered here.
// register convention: none unresolved -- the decompiled C already carries a full
// parameter list (actor_index arrives through in_EAX, which Ghidra did not turn into a
// formal parameter here either).
// blam-cc: EAX -> actor_index, stack -> blast_radius, safety_radius, point, out_count
// UNSURE: prop.is_unit (0x60) gates whether a candidate prop is scored as a hostile or
// checked as a friendly-proximity hazard; types/ai.h does not resolve what it truly means.
// ai_reference_actor_iterator_init_cursor is called with zero visible arguments and its result is read back through a
// local Ghidra never explicitly assigns (the same hidden-return idiom as
// actor_find_nearest_grenade_ally, 0x40e540); reconstructed the same way here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern ai_globals *ai_globals_ptr; // 0x00880354
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, EAX encounter, ECX cursor[3]

// blam-cc: EAX -> actor_index, stack -> blast_radius, safety_radius, point, out_count
uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius,
                                      real_point3d *point, int16_t *out_count)
{
    actor *self;
    datum_index prop_cursor;
    datum_index counted[32];
    int16_t counted_count;
    uint8_t clear;
    int16_t score;
    prop *p;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    prop_cursor = self->first_prop;
    counted_count = 0;
    clear = 1;
    score = 0;
    p = (prop *)0;

    for (;;) {
        int done_with_hostiles = 0;

        for (;;) {
            for (;;) {
                if (prop_cursor == (datum_index)k_datum_index_none) {
                    goto after_prop_walk;
                }
                p = (prop *)((uint8_t *)prop_data->data + (prop_cursor & 0xffff) * sizeof(prop));
                prop_cursor = p->next_in_actor;
                if (2 <= p->state && p->state <= 3 && p->dead == 0) break;
            }
            if (p->enemy == 0) {
                done_with_hostiles = 1;
                break;
            }
            {
                float dx = point->x - p->last_known_position.x;
                float dy = point->y - p->last_known_position.y;
                float dz = point->z - p->last_known_position.z;
                if (dx * dx + dy * dy + dz * dz < blast_radius * blast_radius) {
                    if (p->is_parented == 0) {
                        if (p->relationship_object_index == -1) {
                            datum_index owner = p->owner_actor_index;
                            if (owner != (datum_index)k_datum_index_none) {
                                if (counted_count < 0x20) {
                                    counted[counted_count] = owner;
                                    counted_count++;
                                }
                                if (p->swarm_owned == 0) {
                                    score++;
                                } else {
                                    actor *owner_actor = (actor *)((uint8_t *)actor_data->data +
                                                                    (owner & 0xffff) * sizeof(actor));
                                    score += owner_actor->cluster_count;
                                }
                            }
                        } else {
                            score += 5;
                        }
                    } else {
                        score += 10;
                    }
                }
            }
        }

        if (!done_with_hostiles) {
            break; // unreachable: for-loop above only exits via goto or done_with_hostiles
        }

        if (safety_radius <= 0.0f) {
            continue;
        }
        {
            float dx = point->x - p->last_known_position.x;
            float dy = point->y - p->last_known_position.y;
            float dz = point->z - p->last_known_position.z;
            if (safety_radius * safety_radius <= dx * dx + dy * dy + dz * dz) {
                continue;
            }
        }
        clear = 0;
        break;
    }

after_prop_walk:
    if (0.0f < blast_radius && self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *target_prop = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
        datum_index owner = target_prop->owner_actor_index;
        if (owner != (datum_index)k_datum_index_none) {
            actor *owner_actor = (actor *)((uint8_t *)actor_data->data + (owner & 0xffff) * sizeof(actor));
            if (owner_actor->encounter_index != (datum_index)k_datum_index_none) {
                datum_index iterator[3]; // 0x410fbe: a 12-byte cursor; the walk starts from its third slot
                datum_index cursor;
                iterator[2] = k_datum_index_none;
                ai_reference_actor_iterator_init_cursor((int32_t)owner_actor->encounter_index, iterator);
                cursor = iterator[2];
                while (ai_globals_ptr->actors_valid != 0 && cursor != (datum_index)k_datum_index_none) {
                    actor *candidate = (actor *)((uint8_t *)actor_data->data + (cursor & 0xffff) * sizeof(actor));
                    datum_index next = candidate->next_in_encounter;
                    int16_t i;
                    uint8_t already_counted = 0;

                    for (i = 0; i < counted_count; i++) {
                        if (counted[i] == cursor) { already_counted = 1; break; }
                    }
                    if (!already_counted) {
                        float dx = point->x - candidate->body_position.x;
                        float dy = point->y - candidate->body_position.y;
                        float dz = point->z - candidate->body_position.z;
                        if (dx * dx + dy * dy + dz * dz < blast_radius * blast_radius) {
                            if (candidate->swarm == 0) {
                                score++;
                            } else {
                                score += candidate->cluster_count;
                            }
                        }
                    }
                    cursor = next;
                }
            }
        }
    }

    if (clear != 0 && self->encounter_index != (datum_index)k_datum_index_none && 0.0f < safety_radius) {
        datum_index iterator[3]; // 0x4110c9
        datum_index cursor;
        iterator[2] = k_datum_index_none;
        ai_reference_actor_iterator_init_cursor((int32_t)self->encounter_index, iterator);
        cursor = iterator[2];
        while (ai_globals_ptr->actors_valid != 0 && cursor != (datum_index)k_datum_index_none) {
            actor *candidate = (actor *)((uint8_t *)actor_data->data + (cursor & 0xffff) * sizeof(actor));
            float dx = point->x - candidate->body_position.x;
            float dy = point->y - candidate->body_position.y;
            float dz = point->z - candidate->body_position.z;
            cursor = candidate->next_in_encounter;
            if (dx * dx + dy * dy + dz * dz < safety_radius * safety_radius) {
                clear = 0;
                break;
            }
        }
    }

    if (out_count != (int16_t *)0) {
        *out_count = score;
    }
    return clear;
}

#if 0
Original Ghidra decompilation (0x410da0):

char FUN_00410da0(float param_1,float param_2,float *param_3,short *param_4)

{
  short sVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  uint in_EAX;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  ushort uVar11;
  uint auStack_20080 [32757];
  char local_99;
  uint local_84;
  uint auStack_80 [32];

  iVar6 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar9 = *(uint *)(iVar6 + 0x50);
  uVar11 = 0;
  local_99 = '\x01';
  sVar1 = 0;
  do {
    while( true ) {
      do {
        if (uVar9 == 0xffffffff) goto LAB_00410f51;
        iVar7 = (uVar9 & 0xffff) * 0x138;
        uVar9 = *(uint *)(iVar7 + 8 + *(int *)(DAT_008802c0 + 0x34));
        iVar7 = iVar7 + *(int *)(DAT_008802c0 + 0x34);
      } while (((*(short *)(iVar7 + 0x24) < 2) || (3 < *(short *)(iVar7 + 0x24))) ||
              (*(char *)(iVar7 + 0x127) != '\0'));
      if (*(char *)(iVar7 + 0x60) == '\0') break;
      fVar2 = *param_3 - *(float *)(iVar7 + 0xbc);
      fVar4 = param_3[1] - *(float *)(iVar7 + 0xc0);
      fVar3 = param_3[2] - *(float *)(iVar7 + 0xc4);
      if (fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3 < param_1 * param_1) {
        if (*(char *)(iVar7 + 0x12e) == '\0') {
          if (*(int *)(iVar7 + 0x110) == -1) {
            uVar10 = *(uint *)(iVar7 + 0x1c);
            if (uVar10 != 0xffffffff) {
              if (uVar11 < 0x20) {
                auStack_80[(short)uVar11] = uVar10;
                uVar11 = uVar11 + 1;
              }
              if (*(char *)(iVar7 + 0x14) == '\0') {
                sVar1 = sVar1 + 1;
              }
              else {
                sVar1 = sVar1 + *(short *)((uVar10 & 0xffff) * 0x724 + 0x1e +
                                          *(int *)(DAT_00880360 + 0x34));
              }
            }
          }
          else {
            sVar1 = sVar1 + 5;
          }
        }
        else {
          sVar1 = sVar1 + 10;
        }
      }
    }
  } while ((param_2 <= 0.0) ||
          (fVar2 = *param_3 - *(float *)(iVar7 + 0xbc),
          fVar4 = param_3[1] - *(float *)(iVar7 + 0xc0),
          fVar3 = param_3[2] - *(float *)(iVar7 + 0xc4),
          param_2 * param_2 <= fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3));
  local_99 = '\0';
LAB_00410f51:
  if (((0.0 < param_1) && (*(uint *)(iVar6 + 0x270) != 0xffffffff)) &&
     ((uVar9 = *(uint *)((*(uint *)(iVar6 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34)
                        + 0x1c), uVar9 != 0xffffffff &&
      (iVar7 = *(int *)(DAT_00880360 + 0x34),
      *(int *)((uVar9 & 0xffff) * 0x724 + iVar7 + 0x34) != -1)))) {
    FUN_004369f0();
    uVar9 = local_84;
LAB_00410fe0:
    do {
      uVar10 = uVar9;
      if ((*(char *)(DAT_00880354 + 1) == '\0') || (uVar10 == 0xffffffff)) break;
      iVar8 = (uVar10 & 0xffff) * 0x724;
      uVar9 = *(uint *)(iVar8 + 0x2c + iVar7);
      iVar8 = iVar8 + iVar7;
      sVar5 = 0;
      if (0 < (short)uVar11) {
        do {
          if (auStack_80[sVar5] == uVar10) goto LAB_00410fe0;
          sVar5 = sVar5 + 1;
        } while (sVar5 < (short)uVar11);
      }
      fVar2 = *param_3 - *(float *)(iVar8 + 300);
      fVar4 = param_3[1] - *(float *)(iVar8 + 0x130);
      fVar3 = param_3[2] - *(float *)(iVar8 + 0x134);
      if (fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3 < param_1 * param_1) {
        if (*(char *)(iVar8 + 6) == '\0') {
          sVar1 = sVar1 + 1;
        }
        else {
          sVar1 = sVar1 + *(short *)(iVar8 + 0x1e);
        }
      }
    } while( true );
  }
  if (((local_99 != '\0') && (*(int *)(iVar6 + 0x34) != -1)) && (0.0 < param_2)) {
    FUN_004369f0();
    do {
      if ((*(char *)(DAT_00880354 + 1) == '\0') || (local_84 == 0xffffffff)) goto LAB_00411152;
      iVar6 = *(int *)(DAT_00880360 + 0x34);
      iVar7 = (local_84 & 0xffff) * 0x724;
      fVar2 = *param_3 - *(float *)(iVar7 + 300 + iVar6);
      fVar4 = param_3[1] - *(float *)(iVar7 + 0x130 + iVar6);
      fVar3 = param_3[2] - *(float *)(iVar7 + 0x134 + iVar6);
      local_84 = *(uint *)(iVar7 + iVar6 + 0x2c);
    } while (param_2 * param_2 <= fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3);
    local_99 = '\0';
  }
LAB_00411152:
  if (param_4 != (short *)0x0) {
    *param_4 = sVar1;
  }
  return local_99;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
