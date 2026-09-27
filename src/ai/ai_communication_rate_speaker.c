// ai_communication_rate_speaker  (Ghidra: ai_communication_rate_speaker; named for this rewrite)
// address 0x42fb90, size 1002 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: phase-4 summary ("computes a priority score for how suitable a given actor is to
// speak a particular communication line"). Its only two callers are the two speaker pickers
// in this same block (ai_communication_select_speaker_in_reference 0x42ff80 and
// ai_communication_select_speaker_by_team 0x4300d0), both of which keep the highest score
// and return that actor; and its own callees are ai_communication_rate_player_proximity
// (0x4303f0) and ai_communication_line_fade_multiplier (0x42f8c0), i.e. "is a player around
// to hear it" and "has this line been played too recently".
// register convention: EAX -> position_a, ECX -> object_a, recovered from the two call sites
// (`lea eax,[esp+0x44]` / `mov ecx,edi` at 0x43007c and `lea eax,[esp+0x3c]` / `mov ecx,ebp`
// at 0x43028b); param_1..param_10 are genuine stack arguments (`add esp,0x28`).
// blam-cc: EAX -> position_a, ECX -> object_a, stack -> actor_index, object_b, position_b,
//          radius, allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags
//
// UNSURE, substantially:
//  - param_5 / param_6 / param_7 / param_8 / param_9's real meaning is inferred from where
//    each one lands, not from any name in the image: param_6 arrives in BX at the
//    ai_communication_line_fade_multiplier call (it is that function's `unaff_BX`
//    "short_range_limit"), param_7 is that call's `kind`, param_8 is written into the
//    scratch block the same call hands to FUN_00560d00, and param_9 gates a
//    unit_scripted_action_animation_exists (unit seat/state) test.
//  - The three-word scratch the fade call receives is modeled as
//    { float score; int32_t out_a; int32_t line_id; }: the original stores -1 into word 1
//    and param_8 into word 2 (`mov [esp+0x30],0xffffffff` / `mov [esp+0x24],eax` at
//    0x42fd55..0x42fd77) and passes &word1 in EAX and &word2 in ECX, which
//    ai_communication_line_fade_multiplier forwards straight to FUN_00560d00 as its 4th and
//    5th stack arguments. Ghidra renders the param_8 store as `(float)param_8`; the
//    disassembly shows a plain integer `mov`, so it is transcribed as one here.
//  - prop+0x104 is used as a real_point3d (the prop's last known position); types/ai.h has
//    it as three separate unknown_104/108/10c dwords. TYPES-GAP, not folded because only
//    this file and actor_dispatch_look_handler_by_posture's other callers rely on it.
//  - flags bit meanings (1, 2, 4, 8, 0x10) are named below from their effect only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include <stdint.h>

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern data_array *prop_data;   // 0x008802c0

// 0x41bb30 / 0x41be10: the actor target reachability / priority-class helpers, not yet
// rewritten. UNSURE signatures at this call site (Ghidra shows actor_target_get_priority_class with no
// arguments at all).
extern int16_t actor_dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target,
    uint8_t stance_a, uint8_t check_facing, uint16_t range_class); // 0x41bb30, EBX, stack
extern uint16_t actor_target_get_priority_class(datum_index actor_index, datum_index target_prop_index); // 0x41be10, EAX, ECX
extern int16_t ai_communication_line_fade_multiplier(uint32_t unit_index, int16_t priority, int16_t extra_delay,
    uint8_t follow_fallback, uint8_t apply_fade, float *volume, int32_t *chain_value, int16_t *dialogue_index,
    int16_t line_class); // 0x42f8c0, stack, EAX, ECX, BX
extern float ai_communication_rate_player_proximity(uint8_t require_line_of_sight,
                                                    datum_index *out_player_object_index,
                                                    float *out_distance,
                                                    datum_index object_index); // 0x4303f0
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack
extern uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command); // 0x569470, EAX, ECX

// blam-cc: EAX -> position_a, ECX -> object_a, stack -> actor_index, object_b, position_b,
//          radius, allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags
// Returns 0.0 when this actor must not speak the line, otherwise a priority score starting
// at 10.0. The actor is rejected outright when it is not at least "alerted"
// (awareness_level < 2), when it controls no unit, when neither subject object is inside
// `radius` of its aim origin, when a player-proximity gate (flags bit 1) scores zero, when
// flags bit 2 demands object_a share this actor's active_unit_index, or when the line has
// been spoken too recently. The score is then raised by how close each subject is (through
// that subject's prop record) and by 5.0 when the seat filter matches.
float ai_communication_rate_speaker(datum_index actor_index, datum_index object_b,
                                    real_point3d *position_b, float radius,
                                    int16_t allow_unreachable, uint32_t fade_limit,
                                    uint32_t line_class, uint32_t line_id, int16_t seat_filter,
                                    uint8_t flags,
                                    real_point3d *position_a /* EAX */,
                                    datum_index object_a /* ECX */)
{
    actor *a;
    struct { float score; int32_t out_a; int32_t line_id; } scratch;
    uint8_t have_subject;
    uint8_t accept;
    uint8_t matched_a;
    uint8_t matched_b;
    float dx, dy, dz;
    float proximity;
    datum_index prop_index;
    prop *p;
    int16_t reach;
    int32_t reach_mode;

    a = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * k_actor_size);

    have_subject = (object_a != (datum_index)k_datum_index_none ||
                    object_b != (datum_index)k_datum_index_none) ? 1 : 0;
    accept = (a->unit_index != (datum_index)k_datum_index_none) ? 1 : 0;
    scratch.score = 10.0f;
    scratch.out_a = -1;
    scratch.line_id = (int32_t)line_id;

    if (a->awareness_level < 2 || !accept) {
        return 0.0f;
    }

    if (have_subject) {
        // Both subjects out of range -> this actor has nothing to talk about.
        if (object_a == (datum_index)k_datum_index_none ||
            ((dx = position_a->x - a->aim_origin.x,
              dy = position_a->y - a->aim_origin.y,
              dz = position_a->z - a->aim_origin.z),
             radius * radius <= dz * dz + dx * dx + dy * dy)) {
            if (object_b == (datum_index)k_datum_index_none ||
                ((dx = position_b->x - a->aim_origin.x,
                  dy = position_b->y - a->aim_origin.y,
                  dz = position_b->z - a->aim_origin.z),
                 radius * radius <= dx * dx + dy * dy + dz * dz)) {
                return 0.0f;
            }
        }
        accept = 1;
    }

    if ((flags & 2) != 0) {
        proximity = ai_communication_rate_player_proximity(0, 0, 0, a->unit_index);
        if (proximity == 0.0f) {
            return 0.0f;
        }
        scratch.score = proximity * 5.0f + 10.0f;
    }

    if ((flags & 4) != 0 && object_a != (datum_index)k_datum_index_none &&
        ((object_header *)object_data->data)[object_a & 0xffff].data->parent_object !=
            a->active_unit_index) {
        return 0.0f;
    }

    if (seat_filter != -1 && unit_scripted_action_animation_exists(a->unit_index, (int16_t)seat_filter) != 0 /* 0x42fd1d */) {
        scratch.score = scratch.score + 5.0f;
    }

    if ((int16_t)line_id != -1) {
        // The original tests only AX of the returned status, so the result is narrowed
        // to int16 before the comparison.
        // 0x42fd41: EAX = &chain (-1), ECX = &a copy of line_id, BX = fade_limit
        int32_t chain_value = -1;                    // [esp+0x18]
        int16_t dialogue_index = (int16_t)line_id;   // [esp+0x1c]

        if (ai_communication_line_fade_multiplier(a->unit_index, (int16_t)line_class, 0, (uint8_t)(flags & 1u), 1,
                &scratch.score, &chain_value, &dialogue_index, (int16_t)fade_limit) == 0) {
            return 0.0f;
        }
    }

    if (have_subject) {
        matched_a = 0;
        matched_b = 0;
        if (object_a != (datum_index)k_datum_index_none) {
            if (a->unit_index == object_a) {
                // flags bit 8: this actor may talk about its own unit.
                if ((flags & 8) == 0) {
                    accept = 0;
                } else {
                    matched_a = 1;
                }
            } else {
                prop_index = actor_find_or_create_shared_prop(object_a, actor_index, 1, 0); // 0x42fdc2: EAX = the subject object
                if (prop_index != (datum_index)k_datum_index_none) {
                    p = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * k_prop_size);
                    if (p->distance <= radius) {
                        reach_mode = 2;
                        if (p->kind < 2 || 3 < p->kind) {
                            if (p->is_unit != 0) {
                                goto check_b;
                            }
                            // Both of these are read as int16 by the original even though
                            // types/ai.h declares prop.unknown_34 as an int32.
                            if (allow_unreachable == 0 &&
                                *(int16_t *)((uint8_t *)p + 0x34) < 2 &&
                                *(int16_t *)((uint8_t *)p + 0x36) < 2) {
                                if (*(uint8_t *)((uint8_t *)p + 0x132) == 0) {
                                    reach_mode = (int32_t)*(int8_t *)&p->unknown_120;
                                }
                                // 0x42fe54..0x42fe76: BX = the prop's +0x38 status, range class from 0x41be10
                                reach = actor_dispatch_look_handler_by_posture(*(int16_t *)((uint8_t *)p + 0x38),
                                                     actor_index, &a->aim_origin, (uint8_t *)p + 0x104,
                                                     (uint8_t)reach_mode, 1,
                                                     actor_target_get_priority_class(actor_index, prop_index));
                                if (reach < 2) {
                                    goto check_b;
                                }
                            }
                        }
                        scratch.score = (1.0f - p->distance / radius) * 10.0f + scratch.score;
                        matched_a = 1;
                    } else {
                        matched_a = 0;
                    }
                }
            }
        }
check_b:
        if (object_b != (datum_index)k_datum_index_none) {
            if (a->unit_index == object_b) {
                // flags bit 0x10: this actor may talk about its own unit as subject B.
                if ((flags & 0x10) == 0) {
                    return 0.0f;
                }
                matched_b = 1;
            } else {
                prop_index = actor_find_prop_for_object(object_b, actor_index); // 0x42fec6: ECX = actor (arg 1)
                if (prop_index != (datum_index)k_datum_index_none) {
                    p = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * k_prop_size);
                    if (p->distance <= radius) {
                        if ((1 < p->kind && p->kind < 4) || p->is_unit == 0) {
                            scratch.score = (1.0f - p->distance / radius) * 10.0f + scratch.score;
                            matched_b = 1;
                        }
                    } else {
                        matched_b = 0;
                    }
                }
            }
        }
        if (!accept) {
            return 0.0f;
        }
        accept = (uint8_t)(matched_b | matched_a);
    }

    if (accept) {
        return scratch.score;
    }
    return 0.0f;
}

#if 0
Original Ghidra decompilation (0x42fb90):

float10 FUN_0042fb90(uint param_1,int param_2,float *param_3,float param_4,short param_5,
                    undefined4 param_6,undefined4 param_7,undefined4 param_8,short param_9,
                    byte param_10)

{
  float fVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  byte bVar5;
  byte bVar6;
  char cVar7;
  short sVar8;
  float *in_EAX;
  undefined4 uVar9;
  uint uVar10;
  uint in_ECX;
  int iVar11;
  int iVar12;
  int iVar13;
  bool bVar14;
  float10 fVar15;
  float local_c [3];

  iVar11 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((in_ECX != 0xffffffff) || (bVar4 = false, param_2 != -1)) {
    bVar4 = true;
  }
  bVar14 = *(int *)(iVar11 + 0x18) != -1;
  local_c[0] = 10.0;
  if ((*(short *)(iVar11 + 0x6a) < 2) || (!bVar14)) goto LAB_0042ff6c;
  if (bVar4) {
    if (((in_ECX == 0xffffffff) ||
        (fVar1 = *in_EAX - *(float *)(iVar11 + 0x120),
        fVar3 = in_EAX[1] - *(float *)(iVar11 + 0x124),
        fVar2 = in_EAX[2] - *(float *)(iVar11 + 0x128),
        param_4 * param_4 <= fVar2 * fVar2 + fVar1 * fVar1 + fVar3 * fVar3)) &&
       ((param_2 == -1 ||
        (fVar1 = *param_3 - *(float *)(iVar11 + 0x120),
        fVar3 = param_3[1] - *(float *)(iVar11 + 0x124),
        fVar2 = param_3[2] - *(float *)(iVar11 + 0x128),
        param_4 * param_4 <= fVar1 * fVar1 + fVar3 * fVar3 + fVar2 * fVar2)))) goto LAB_0042ff6c;
    bVar14 = true;
  }
  if ((param_10 & 2) != 0) {
    fVar15 = (float10)FUN_004303f0(0,0,0);
    if (fVar15 == (float10)0.0) goto LAB_0042ff6c;
    local_c[0] = (float)(fVar15 * (float10)5.0 + (float10)10.0);
  }
  if ((((param_10 & 4) != 0) && (in_ECX != 0xffffffff)) &&
     (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) + 0x11c) !=
      *(int *)(iVar11 + 0x158))) goto LAB_0042ff6c;
  if ((param_9 != -1) && (cVar7 = FUN_00569470(), cVar7 != '\0')) {
    local_c[0] = local_c[0] + 5.0;
  }
  if ((short)param_8 != -1) {
    local_c[2] = (float)param_8;
    local_c[1] = -NAN;
    sVar8 = FUN_0042f8c0(*(undefined4 *)(iVar11 + 0x18),param_7,0,param_10 & 1,1,local_c);
    if (sVar8 == 0) goto LAB_0042ff6c;
  }
  if (bVar4) {
    bVar5 = 0;
    bVar6 = 0;
    if (in_ECX != 0xffffffff) {
      if (*(uint *)(iVar11 + 0x18) == in_ECX) {
        if ((param_10 & 8) == 0) {
          bVar14 = false;
        }
        else {
LAB_0042fea2:
          bVar5 = 1;
        }
      }
      else {
        uVar10 = FUN_0043eb30(param_1,1,0);
        if (uVar10 != 0xffffffff) {
          iVar12 = (uVar10 & 0xffff) * 0x138;
          iVar13 = iVar12 + *(int *)(DAT_008802c0 + 0x34);
          if (*(float *)(iVar12 + 0x11c + *(int *)(DAT_008802c0 + 0x34)) <= param_4) {
            iVar12 = 2;
            if ((*(short *)(iVar13 + 0x24) < 2) || (3 < *(short *)(iVar13 + 0x24))) {
              if (*(char *)(iVar13 + 0x60) != '\0') goto LAB_0042fea7;
              if (((param_5 == 0) && (*(short *)(iVar13 + 0x34) < 2)) &&
                 (*(short *)(iVar13 + 0x36) < 2)) {
                if (*(char *)(iVar13 + 0x132) == '\0') {
                  iVar12 = (int)*(char *)(iVar13 + 0x120);
                }
                uVar9 = FUN_0041be10();
                sVar8 = FUN_0041bb30(param_1,iVar11 + 0x120,iVar13 + 0x104,iVar12,1,uVar9);
                if (sVar8 < 2) goto LAB_0042fea7;
              }
            }
            local_c[0] = (1.0 - *(float *)(iVar13 + 0x11c) / param_4) * 10.0 + local_c[0];
            goto LAB_0042fea2;
          }
          bVar5 = 0;
        }
      }
    }
LAB_0042fea7:
    if (param_2 != -1) {
      if (*(int *)(iVar11 + 0x18) == param_2) {
        if ((param_10 & 0x10) == 0) goto LAB_0042ff6c;
LAB_0042ff3d:
        bVar6 = 1;
      }
      else {
        uVar10 = FUN_0043ea80(param_2);
        if (uVar10 != 0xffffffff) {
          iVar11 = (uVar10 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
          if (*(float *)(iVar11 + 0x11c) <= param_4) {
            if (((1 < *(short *)(iVar11 + 0x24)) && (*(short *)(iVar11 + 0x24) < 4)) ||
               (*(char *)(iVar11 + 0x60) == '\0')) {
              local_c[0] = (1.0 - *(float *)(iVar11 + 0x11c) / param_4) * 10.0 + local_c[0];
              goto LAB_0042ff3d;
            }
          }
          else {
            bVar6 = 0;
          }
        }
      }
    }
    if (!bVar14) goto LAB_0042ff6c;
    bVar14 = (bool)(bVar6 | bVar5);
  }
  if (bVar14) {
    return (float10)local_c[0];
  }
LAB_0042ff6c:
  return (float10)0.0;
}
#endif
