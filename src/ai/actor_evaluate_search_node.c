// actor_evaluate_search_node  (Ghidra: actor_evaluate_search_node, renamed)
// address 0x4091d0, size 1003 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: types/ai.h actor.actor_variant_tag (0x5c)/position-cache (0x12c/0x130); actor
//   mode (0x6c) compared against _actor_mode_vocalize (9); prop.is_unit (0x60)/
//   next_in_actor (0x08)/unknown_1c (0x2c); data_array.maximum_count/size (already-named
//   fields, reused here to resolve a raw datum_index into an actor pointer with a salt
//   check -- the standard pattern, just inlined rather than going through a helper); phase-4
//   summary "evaluates one candidate search/junction node against a from-to object pair,
//   returning its direction, orientation, and a visibility-based score".
//
// Given the size and depth of unresolved offsets (a from/to node table this function reads
// via param_2/param_3 that is not identified anywhere in types/ai.h or types/tags.h, plus
// six out-of-range callees), this is kept close to the Ghidra decompilation rather than
// fully re-derived. Confidence is low; treat this file as a starting point for a future
// pass, not a settled rewrite.
// UNSURE, broadly: the "candidate actor" struct walked near the end (psVar14) is resolved as
// an actor pointer via the datum_index pattern above, but its offsets beyond mode (0x6c) and
// mode_data (0x9c) are read as a distinct 0x9c-based sub-record (psVar14+0x9c..0xd8-ish) that
// does not obviously correspond to any named actor field at those absolute offsets; kept as
// raw offsets from psVar14 rather than reusing the `actor` struct for that part.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14

extern char unit_is_seat_occupied(void); // 0x56cc10, not yet rewritten
extern char unit_seat_flag_bit10(void); // 0x56cdd0, not yet rewritten
extern char unit_find_weapon_marker_transform(int32_t node_table, uint32_t node_index, float *out_a, float *out_b, uint32_t *out_c); // 0x5640a0, not yet rewritten
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, objects module
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern char unit_seat_flag_bit3(void); // 0x56cd70, not yet rewritten (result discriminated by which branch calls it)
extern double sqrt(double x); // FSQRT, Ghidra's SQRT() pseudo-function; see src/math for the convention

int32_t actor_evaluate_search_node(uint32_t actor_index, uint32_t from_object, uint32_t node_table, float *out_position, float *out_direction, uint32_t *out_extra, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_flag)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    ActorVariant *variant = (ActorVariant *)tag_instances[a->actor_variant_tag & 0xffff].data;
    char ok;

    ok = unit_is_seat_occupied();
    if (ok != 0) {
        return 0;
    }

    {
        Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
        if ((((uint8_t *)actor_def)[4] & 8) != 0 && unit_seat_flag_bit10() == 0) {
            return 0;
        }
    }

    {
        float node_x, node_y, node_z_out;
        uint32_t extra;

        if (unit_find_weapon_marker_transform((int32_t)node_table, from_object, &node_x, &node_y, &extra) == 0) {
            return 0;
        }
        {
            real_point3d self_position;
            real_vector2d dir2;
            real_vector3d dir3;
            float best_dx, best_dy;
            float score;
            datum_index prop_index;
            uint8_t is_close;
            uint8_t facing_ok;

            object_get_position(&self_position, actor_index);
            dir2.i = node_y - node_x; // UNSURE: local_1c/local_34 pairing per the original
            dir2.j = 0.0f;
            (void)dir3;

            if (vector2d_normalize_with_length(&dir2) == 0.0f) {
                dir3.i = a->facing.i;
                dir3.j = a->facing.j;
                dir3.k = a->facing.k;
            }

            {
                float d_from_x = node_x - a->body_position.x;
                float d_from_y = node_y - a->body_position.y;
                float d_to_x = node_y - a->body_position.x; // UNSURE: mirrors the original's reuse of local_1c for both ends
                float d_to_y = node_y - a->body_position.y;

                if ((float)sqrt((double)(d_from_x * d_from_x + d_from_y * d_from_y)) <= (float)sqrt((double)(d_to_x * d_to_x + d_to_y * d_to_y))) {
                    best_dx = d_from_x;
                    best_dy = node_y;
                } else {
                    best_dx = d_to_x;
                    best_dy = node_y;
                }
                best_dy -= a->body_position.y;
                score = (float)sqrt((double)(best_dx * best_dx + best_dy * best_dy));
            }

            prop_index = *(datum_index *)((uint8_t *)a + 0x50);
            for (;;) {
                if (prop_index == (datum_index)k_datum_index_none) {
                    float fx = a->body_position.x;
                    float fy = a->body_position.y;
                    float facing_dot;
                    float visibility;

                    vector2d_normalize_with_length(&dir2);
                    facing_dot = (node_y - fx) * a->facing.i + (node_y - fy) * a->facing.j;

                    facing_ok = !(score >= 1.1f || facing_dot <= 0.0f);

                    if (*(int8_t *)variant < 0) { // pcVar3: the low byte of ActorVariant.flags, i.e. bit 7 of it
                        ok = unit_seat_flag_bit3();
                        if (ok != 0) {
                            visibility = 0.0f;
                            goto have_score;
                        }
                    } else {
                        ok = unit_seat_flag_bit3();
                        if (ok == 0) {
                            visibility = 0.0f;
                            goto have_score;
                        }
                    }
                    visibility = 3.5f;
                have_score:
                    if (out_position != 0) {
                        out_position[0] = node_x;
                        out_position[1] = node_y;
                        out_position[2] = 0.0f; // UNSURE: local_2c, never assigned on this path in the original
                    }
                    if (out_direction != 0) {
                        out_direction[0] = dir2.i;
                        out_direction[1] = dir2.j;
                        out_direction[2] = 0.0f;
                    }
                    if (out_extra != 0) {
                        out_extra[0] = extra;
                        out_extra[1] = 0;
                        out_extra[2] = 0;
                    }
                    if (out_score != 0) {
                        *out_score = visibility;
                    }
                    if (out_close != 0) {
                        *out_close = score < 0.7f;
                    }
                    if (out_facing != 0) {
                        *out_facing = best_dy > 0.6f; // UNSURE: reuses fVar1 from the block above per the original
                    }
                    if (out_flag != 0) {
                        *out_flag = facing_ok;
                    }
                    return 1;
                }

                {
                    prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
                    datum_index next = p->next_in_actor;
                    int32_t candidate;

                    if (p->is_unit == 0 || (candidate = p->owner_actor_index, candidate == -1)) {
                        prop_index = next;
                        continue;
                    }
                    prop_index = next;

                    {
                        int16_t idx = (int16_t)candidate;
                        actor *cand = 0;

                        if (idx >= 0 && idx < actor_data->maximum_count) {
                            actor *maybe = &((actor *)actor_data->data)[idx];
                            int16_t salt = (int16_t)((uint32_t)candidate >> 16);
                            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                                cand = maybe;
                            }
                        }
                        if (cand == 0 || cand->mode != _actor_mode_vocalize ||
                            *(int32_t *)((uint8_t *)cand->mode_data) != (int32_t)from_object ||
                            *(int16_t *)((uint8_t *)cand->mode_data + 4) != (int16_t)node_table) {
                            continue;
                        }
                        {
                            float ex = *(float *)((uint8_t *)cand->mode_data + (0x66 * 2 - 0x9c));
                            float ey = *(float *)((uint8_t *)cand->mode_data + (0x68 * 2 - 0x9c));
                            float fx2 = cand->body_position.x;
                            float fy2 = cand->body_position.y;
                            if (score * score <= (ex - fx2) * (ex - fx2) + (ey - fy2) * (ey - fy2)) {
                                continue;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4091d0):

undefined4
FUN_004091d0(uint param_1,int param_2,undefined4 param_3,float *param_4,float *param_5,
            undefined4 *param_6,float *param_7,int param_8,int param_9,undefined1 *param_10)

{
  float fVar1;
  float fVar2;
  char *pcVar3;
  float fVar4;
  float fVar5;
  char cVar6;
  short sVar7;
  int iVar8;
  short *psVar9;
  short sVar10;
  undefined1 uVar11;
  int iVar12;
  int iVar13;
  short *psVar14;
  uint uVar15;
  float10 fVar16;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;

  iVar13 = (param_1 & 0xffff) * 0x724;
  iVar12 = *(int *)(DAT_00880360 + 0x34) + iVar13;
  pcVar3 = *(char **)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x5c + iVar13) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  cVar6 = FUN_0056cc10();
  if ((cVar6 == '\0') &&
     ((((*(byte *)(*(int *)((*(uint *)(iVar12 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) &
        8) == 0 || (cVar6 = FUN_0056cdd0(), cVar6 != '\0')) &&
      (cVar6 = FUN_005640a0(param_2,param_3,&local_34,&local_1c,&local_10), cVar6 != '\0')))) {
    object_get_position();
    local_28 = local_1c - local_34;
    local_20 = 0.0;
    local_24 = local_18 - local_30;
    fVar16 = (float10)vector2d_normalize_with_length();
    if ((float10)0.0 == fVar16) {
      local_28 = *(float *)(iVar12 + 0x174);
      local_24 = *(float *)(iVar12 + 0x178);
      local_20 = *(float *)(iVar12 + 0x17c);
    }
    fVar4 = local_34 - *(float *)(iVar12 + 300);
    fVar1 = local_30 - *(float *)(iVar12 + 0x130);
    fVar2 = local_1c - *(float *)(iVar12 + 300);
    fVar5 = local_18 - *(float *)(iVar12 + 0x130);
    if (SQRT(fVar4 * fVar4 + fVar1 * fVar1) <= SQRT(fVar2 * fVar2 + fVar5 * fVar5)) {
      fVar4 = local_34 - *(float *)(iVar12 + 300);
      fVar1 = local_30;
    }
    else {
      fVar4 = local_1c - *(float *)(iVar12 + 300);
      fVar1 = local_18;
    }
    fVar1 = fVar1 - *(float *)(iVar12 + 0x130);
    uVar15 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x50 + iVar13);
    fVar4 = SQRT(fVar4 * fVar4 + fVar1 * fVar1);
    do {
      do {
        if (uVar15 == 0xffffffff) {
          fVar1 = *(float *)(iVar12 + 300);
          fVar2 = *(float *)(iVar12 + 0x130);
          vector2d_normalize_with_length();
          fVar1 = (local_1c - fVar1) * *(float *)(iVar12 + 0x174) +
                  (local_18 - fVar2) * *(float *)(iVar12 + 0x178);
          if ((1.1 <= fVar4) || (fVar1 <= 0.0)) {
            uVar11 = 0;
          }
          else {
            uVar11 = 1;
          }
          if (*pcVar3 < '\0') {
            cVar6 = FUN_0056cd70();
            fVar16 = extraout_ST0;
            if (cVar6 != '\0') goto LAB_00409523;
          }
          else {
            cVar6 = FUN_0056cd70();
            fVar16 = extraout_ST0_00;
            if (cVar6 == '\0') goto LAB_00409523;
          }
          fVar16 = fVar16 + (float10)3.5;
LAB_00409523:
          if (param_4 != (float *)0x0) {
            *param_4 = local_34;
            param_4[1] = local_30;
            param_4[2] = local_2c;
          }
          if (param_5 != (float *)0x0) {
            *param_5 = local_28;
            param_5[1] = local_24;
            param_5[2] = local_20;
          }
          if (param_6 != (undefined4 *)0x0) {
            *param_6 = local_10;
            param_6[1] = local_c;
            param_6[2] = local_8;
          }
          if (param_7 != (float *)0x0) {
            *param_7 = (float)fVar16;
          }
          if (param_8 != 0) {
            *(bool *)param_8 = fVar4 < 0.7;
          }
          if (param_9 != 0) {
            *(bool *)param_9 = 0.6 < fVar1;
          }
          if (param_10 != (undefined1 *)0x0) {
            *param_10 = uVar11;
          }
          return 1;
        }
        iVar13 = *(int *)(DAT_008802c0 + 0x34);
        iVar8 = (uVar15 & 0xffff) * 0x138;
        uVar15 = *(uint *)(iVar8 + 8 + iVar13);
      } while ((*(char *)(iVar8 + 0x60 + iVar13) != '\0') ||
              (iVar13 = *(int *)(iVar8 + iVar13 + 0x1c), iVar13 == -1));
      psVar14 = (short *)0x0;
      sVar7 = (short)iVar13;
      if ((-1 < sVar7) && (sVar7 < *(short *)(DAT_00880360 + 0x20))) {
        psVar9 = (short *)((int)*(short *)(DAT_00880360 + 0x22) * (int)sVar7 +
                          *(int *)(DAT_00880360 + 0x34));
        sVar7 = *psVar9;
        if ((sVar7 != 0) &&
           ((sVar10 = (short)((uint)iVar13 >> 0x10), sVar10 == 0 || (sVar7 == sVar10)))) {
          psVar14 = psVar9;
        }
      }
    } while ((((psVar14[0x36] != 9) || (*(int *)(psVar14 + 0x4e) != param_2)) ||
             (psVar14[0x50] != (short)param_3)) ||
            (fVar4 * fVar4 <=
             (*(float *)(psVar14 + 0x66) - *(float *)(psVar14 + 0x96)) *
             (*(float *)(psVar14 + 0x66) - *(float *)(psVar14 + 0x96)) +
             (*(float *)(psVar14 + 0x68) - *(float *)(psVar14 + 0x98)) *
             (*(float *)(psVar14 + 0x68) - *(float *)(psVar14 + 0x98))));
  }
  return 0;
}
#endif
