// actor_target_evaluate_squad_link  (Ghidra: actor_target_evaluate_squad_link; named from out/phase2/results/ai_02.json)
// address 0x41e320, size 1849 bytes
// name confidence: 0.4   rewrite confidence: 0.1
// evidence: out/phase2/results/ai_02.json -- walks a squad's target linked list (a raw object
//   tree via object.next_object/first_child_object -- puVar1[0x45]/[0x46] -- not the prop
//   chain), for each live object checking a danger-radius overlap and calling
//   actor_danger_register_point / actor_danger_register_stationary_object, then appending
//   surviving candidates into one of two caller-supplied 12-byte-stride arrays. Recurses on
//   object.first_child_object, continues on object.next_object.
// register convention: all four parameters are Ghidra's recognized stack parameters.
//   // blam-cc: stack -> actor_index, object_cursor, candidates_a, candidates_b
//
// UNSURE, extremely substantially: this rewrite is a near-literal, goto-preserving
// transliteration rather than a restructured one, because the control flow (several
// re-convergent gotos over partially-overlapping bool/float state) could not be safely
// flattened without a disassembly cross-check this batch does not have. Field names are used
// wherever this batch's headers (types/ai.h actor/encounter, types/objects.h object,
// types/units.h unit_data) resolve the offset with confidence; every other offset -- most of
// what the pointer `puVar15`/`far_tag` touches, and the two output arrays' internal layout --
// is left as a raw cast, matching Ghidra's own uncertainty. object_get_position and
// actor_get_firing_positions are called with no arguments at every site in this function,
// exactly as Ghidra shows (their real signatures are established elsewhere in this batch, but
// neither operand is recoverable here); the four local position buffers they are presumed to
// fill (self/far positions for each of the two branches) are declared but their exact source is
// unconfirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *encounter_data;  // 0x008802c8
extern int32_t object_cluster_stamp; // 0x008603cc
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern double sqrt(double x); // FSQRT
static float sqrt_f(float x) { return (float)sqrt((double)x); }

extern void object_get_position(void);        // 0x4f6900, UNSURE: no traced args at these call sites
extern void actor_get_firing_positions(void);  // 0x41c1e0, this batch, UNSURE: no traced args at these call sites
extern datum_index object_find_nearest_squad_member(void *reference, datum_index exclude_index, char stamp_group); // 0x41c2c0 (object_find_nearest_squad_member, this batch), UNSURE: EAX/actor_index not shown here
extern void actor_target_data_refresh(uint32_t actor_index, datum_index target_prop_index, void *scratch, uint32_t flag_a, uint32_t flag_b); // 0x41c4b0 (actor_target_data_refresh, this batch)
extern void actor_danger_register_stationary_object(uint32_t actor_index, datum_index object_index, uint8_t unknown_byte); // 0x41ea60 (actor_danger_register_stationary_object, this batch)
extern uint8_t actor_danger_register_point(float radius, float distance, char accept_flag, uint8_t unknown_byte); // 0x41ec90 (actor_danger_register_point, this batch)
extern datum_index actor_find_or_allocate_prop(uint32_t actor_index, datum_index object_index, char flag); // 0x43e270, UNSURE signature
extern int8_t teams_are_enemies(void); // 0x45bd50, UNSURE: no traced args
extern int16_t actor_get_current_mode_combat_grade(void); // 0x40e760, UNSURE: no traced args
extern void *object_try_and_get(int32_t kind); // 0x4f6ec0

// blam-cc: stack -> actor_index, object_cursor, candidates_a, candidates_b
// Iterates a squad's list of known targets evaluating danger radius and distance to select the
// most relevant one for the actor to assist against.
void actor_target_evaluate_squad_link(uint32_t actor_index, datum_index object_cursor, int16_t *candidates_a, int16_t *candidates_b)
{
    actor *self;
    uint32_t *cursor_obj;
    uint32_t *far_obj;
    int16_t *psVar16;
    uint32_t uVar17, uVar20;
    uint8_t *iVar11_tag;
    int32_t iVar18;
    int32_t iVar12, iVar14;
    uint8_t bVar4, bVar5, bVar6, bVar7;
    int8_t cVar8;
    int16_t sVar9;
    float fVar2, fVar3;
    float local_7c, local_78, local_74; // far object position (object_get_position, kind==0 branch)
    float local_64, local_60, local_5c; // self firing/aim position (actor_get_firing_positions, kind==0 branch)
    float local_88, local_84, local_80; // far tag danger center? (object_get_position, kind==5 branch)
    float local_2c, local_28, local_24; // self firing/aim position (actor_get_firing_positions, kind==5 branch)

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

top:
    if (object_cursor == k_datum_index_none) {
        return;
    }

    cursor_obj = (uint32_t *)((object_header *)object_data->data)[object_cursor & 0xffff].data;

    if (cursor_obj[5] != (uint32_t)object_cluster_stamp) {
        *(int32_t *)((uint8_t *)cursor_obj + 0x14) = object_cluster_stamp;
        sVar9 = (int16_t)cursor_obj[0x2d]; // object.type

        if (sVar9 == 0) {
            object_get_position();
            actor_get_firing_positions();
            uVar17 = cursor_obj[0x7e]; // unit.swarm_actor_index

            if (uVar17 == 0xffffffff) {
                uVar17 = cursor_obj[0x7d]; // unit.actor_index
                far_obj = cursor_obj;
            } else {
                object_cursor = object_find_nearest_squad_member((void *)0, k_datum_index_none, 1); // UNSURE: real args unresolved
                if (object_cursor == k_datum_index_none) {
                    goto after_switch;
                }
                far_obj = (uint32_t *)((object_header *)object_data->data)[object_cursor & 0xffff].data;
                object_get_position();
            }

            if (object_cursor != k_datum_index_none && uVar17 != actor_index) {
                uVar20 = far_obj[0x86]; // unit.controlling_player
                iVar11_tag = (uint8_t *)tag_instances[far_obj[0] & 0xffff].data;
                cVar8 = teams_are_enemies();

                if ((((uint8_t *)far_obj)[0x106] & 4) == 0 || *(int16_t *)((uint8_t *)far_obj + 0x420) != 0) {
                    bVar4 = 0; bVar6 = 0; sVar9 = 0;
                } else {
                    bVar4 = 1; bVar6 = 1;
                    if (far_obj[0x107] == 0xffffffff) {
                        sVar9 = 0x7fff;
                    } else {
                        sVar9 = (int16_t)(game_time->game_time - (int16_t)far_obj[0x107]);
                    }
                }

                fVar2 = *(float *)(iVar11_tag + 0x284);
                fVar3 = (local_7c - local_64) * (local_7c - local_64) +
                        (local_78 - local_60) * (local_78 - local_60) +
                        (local_74 - local_5c) * (local_74 - local_5c);

                if (0.0f < fVar2 && (bVar4 != 0 || *(int8_t *)((uint8_t *)far_obj + 0x2a3) == 0x1e)) {
                    actor_danger_register_point(fVar2, sqrt_f(fVar3), (char)cVar8, 0);
                    bVar4 = bVar6;
                }

                if (uVar17 == 0xffffffff) {
                    iVar18 = 0;
                } else {
                    iVar18 = (int32_t)((uint8_t *)actor_data->data + (uVar17 & 0xffff) * sizeof(actor));
                }
                bVar7 = 0;

                if (uVar20 != 0xffffffff) {
                    goto have_candidate;
                }
                if (((iVar18 == 0) ||
                     (((actor *)iVar18)->active != 0 && ((actor *)iVar18)->keep_unit_alive == 0)) &&
                    (fVar3 <= 1600.0f)) {
                    if (bVar4 != 0) {
                        // FIXED: Ghidra reads actor+0x34, which is encounter_index, not
                        // unit_index (0x18); the handle is then scaled by the 0x6c encounter
                        // stride, which only makes sense for an encounter datum.
                        uVar17 = self->encounter_index;
                        bVar4 = 1;
                        if (uVar17 == 0xffffffff) {
                            goto check_flee_range;
                        } else {
                            iVar12 = (int32_t)((uint8_t *)encounter_data->data + (uVar17 & 0xffff) * sizeof(encounter));
                            iVar18 = self->unknown_3a0; // datum_index, but compared as int32
                            iVar14 = *(int32_t *)((uint8_t *)iVar12 + 0x58);
                            if (iVar14 <= iVar18) {
                                iVar14 = iVar18;
                            }
                            if (iVar14 != -1 &&
                                (iVar18 = *(int32_t *)((uint8_t *)cursor_obj + 0x41c), iVar18 == -1 || iVar18 < iVar14)) {
                                bVar4 = 0;
                            }
                            if (*(uint8_t *)((uint8_t *)iVar12 + 0x45) == 0 && *(uint8_t *)((uint8_t *)iVar12 + 0x44) == 0 &&
                                *(uint8_t *)((uint8_t *)iVar12 + 0x42) == 0) {
                                bVar5 = 1;
                            } else {
                                bVar5 = 0;
                            }
                            if (bVar4 != 0) {
                                if (bVar5 == 0) {
                                    goto check_flee_range;
                                }
                                if (fVar3 < 225.0f) {
                                    goto have_candidate;
                                }
                            }
                        }
                        goto skip_append;
                    check_flee_range:
                        if (0.0f < fVar2) {
                            goto have_candidate;
                        }
                        if ((cVar8 == 0 || sVar9 < 0x97) && (sVar9 = actor_get_current_mode_combat_grade(), sVar9 < 2)) {
                            fVar2 = 16.0f;
                            if (cVar8 == 0 && self->awareness_level < 3) {
                                fVar2 = 64.0f;
                            }
                            if (fVar3 < fVar2) {
                                goto have_candidate;
                            }
                        }
                        goto skip_append;
                    } else {
                        goto check_flee_range2;
                    }
                }
                goto skip_append;

            check_flee_range2:
                if (cVar8 != 0) {
                    bVar7 = (fVar3 > 36.0f);
                    goto have_candidate;
                }
                if (self->unknown_6e < 4) {
                    bVar7 = (self->unknown_1cc == 0 && fVar3 > 16.0f);
                } else {
                    bVar7 = 1;
                }
                if (225.0f <= fVar3) {
                    goto after_switch;
                }
                psVar16 = candidates_b;
                goto append_candidate;

            have_candidate:
                psVar16 = candidates_a;
                if (cVar8 == 0) {
                    psVar16 = candidates_b;
                }
            append_candidate:
                if (bVar7 != 0) {
                    sVar9 = psVar16[1];
                    if (sVar9 < 0x80) {
                        (psVar16 + sVar9 * 6 + 4)[0] = -1;
                        (psVar16 + sVar9 * 6 + 4)[1] = -1;
                        *(uint32_t *)(psVar16 + psVar16[1] * 6 + 2) = object_cursor;
                        *(float *)(psVar16 + (psVar16[1] + 1) * 6) = fVar3;
                        psVar16[1] = psVar16[1] + 1;
                    }
                } else {
                    iVar18 = actor_find_or_allocate_prop(actor_index, object_cursor, cVar8);
                    if (iVar18 != -1) {
                        actor_target_data_refresh(actor_index, (datum_index)iVar18, (void *)0, 0, 0);
                        if (bVar6 == 0) {
                            *psVar16 = *psVar16 + 1;
                        }
                    }
                }
            skip_append:;
            }
        } else if (sVar9 == 1) {
            if (cursor_obj[0xc9] == 0xffffffff) { // 0x324, unit.driver_unit_index offset reused
                actor_danger_register_stationary_object(actor_index, object_cursor, 0);
            }
        } else if (sVar9 == 5) {
            uint8_t *tag5 = (uint8_t *)tag_instances[cursor_obj[0] & 0xffff].data;
            if (0.0f < *(float *)(tag5 + 0x1a8) &&
                (cursor_obj[0x47] == 0xffffffff || (cursor_obj[0x8b] & 0x20) != 0)) {
                object_get_position();
                actor_get_firing_positions();
                fVar2 = sqrt_f((local_88 - local_2c) * (local_88 - local_2c) +
                               (local_84 - local_28) * (local_84 - local_28) +
                               (local_80 - local_24) * (local_80 - local_24));

                if (fVar2 < *(float *)(tag5 + 0x1a8) + 10.0f) {
                    if (self->danger_type < 2 ||
                        (self->danger_type == 2 && self->danger_object_index != object_cursor &&
                         fVar2 < self->danger_unknown_2d4)) {
                        uint32_t *clear = (uint32_t *)&self->danger_type;
                        int32_t i;
                        for (i = 0x1b; i != 0; i--) {
                            *clear = 0;
                            clear++;
                        }
                        self->danger_type = 2;
                        self->danger_object_index = object_cursor;
                        self->danger_unknown_294 = *(float *)(tag5 + 0x1a8);
                        self->danger_unknown_298 = local_88;
                        self->danger_unknown_29c = local_84;
                        self->danger_unknown_2a0 = local_80;
                        self->danger_unknown_2a4 = cursor_obj[0x1a];
                        self->danger_unknown_2a8 = cursor_obj[0x1b];
                        self->danger_unknown_2ac = cursor_obj[0x1c];
                        self->danger_unknown_284 = 0x1e;
                        self->danger_unknown_286 = 0;
                        self->danger_unknown_282 = 0;

                        uVar17 = cursor_obj[0x31];
                        uVar20 = 0xffffffff;
                        if (uVar17 != 0xffffffff) {
                            void *ctx = object_try_and_get(-1);
                            if (ctx != (void *)0 && (1 << (*((uint8_t *)ctx + 0xb4) & 0x1f) & 3) != 0) {
                                uVar20 = uVar17;
                                if (self->unit_index == k_datum_index_none || uVar17 != self->unit_index) {
                                    cVar8 = teams_are_enemies();
                                    if (cVar8 == 0) {
                                        self->danger_unknown_282 = 1;
                                    }
                                } else {
                                    self->danger_unknown_282 = 2;
                                }
                            }
                        }
                        self->danger_unknown_290 = uVar20;
                    }
                }
            }
        }
    }

after_switch:
    if (cursor_obj[0x46] != 0xffffffff) { // object.first_child_object
        actor_target_evaluate_squad_link(actor_index, cursor_obj[0x46], candidates_a, candidates_b);
    }
    object_cursor = cursor_obj[0x45]; // object.next_object
    goto top;
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
