// actor_update_crouch_state  (Ghidra: actor_update_crouch_state, renamed)
// address 0x4213b0, size 1653 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: the five Actor tag fields this function reads are consecutive and name
// themselves -- Actor+0x2f8 defensive_crouch_type (the switch selector), +0x2fc
// attacking_crouch_threshold, +0x300 defending_crouch_threshold, +0x304 min_stand_time and
// +0x308 min_crouch_time (multiplied by 30 to make the tick timer at actor+0x35a). So the
// flag at actor+0x358 that the switch drives is the crouch state, not a charge decision as
// the phase-4 summary guessed. The function also maintains the smoothed threat level at
// actor+0x350 / +0x354 out of actor.tally.by_threat_class, keeps the "dangerous neighbour"
// flags at 0x35c..0x35f by walking the prop list, and finally runs the backup-request scan.
// register convention: actor_index is a genuine stack parameter.
// blam-cc: stack -> actor_index
//
// Facts recovered from the disassembly that Ghidra hides:
//  - the prop-loop calls 0x420b10 (actor_evaluate_flank_offset) three times with
//    ECX = &cover_direction (the 0x420970 out-parameter), EBX = &flank_offset (nullable),
//    ESI = the sample position and EDI = prop.last_known_position.
//  - the two 0x401000 calls are vector3d_magnitude_squared(EAX = &flank_offset), not the
//    zero-argument stub Ghidra prints.
//  - the final dot product is dot(flank_offset, normalized actor.unknown_518). Ghidra prints
//    dot(normalized_518, raw actor.unknown_518), which is a register-tracking error: objdump
//    0x4217b4 multiplies [esp+0x34..0x3c] (the flank offset) by [esp+0x28..0x30] (the
//    in-place-normalized copy of unknown_518).
//  - the crouch/stand timer is (int16)__ftol(time * 30.0), the constant at 0x00672ac8.
// UNSURE: 0x420970, 0x424090, 0x4141a0 and ai_communication_broadcast are not rewritten; the
// argument lists are what this call site allows.
// UNSURE: the six threat levels (0, 0.7, 1.2, 1.6, 1.8, 2.0) and the exponential smoother are
// transcribed literally; the original computes exp(-0.04620981216430664) with the inline x87
// ROUND / f2xm1 / fscale sequence on two compile-time constants.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern tag_instance *tag_instances;  // 0x0087bc14

extern double exp2(double x); // the inline x87 ROUND / f2xm1 / fscale sequence
extern int32_t __ftol(double x); // 0x6391b4, MSVC float-to-long (truncating)

extern float vector3d_magnitude_squared(const real_vector3d *v); // 0x401000, EAX -> v
extern real vector3d_normalize_with_length(real_vector3d *v);    // 0x401990, ECX -> v
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL
extern uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index,
    real_vector3d *out_vector); // 0x420970, EAX, ECX, stack
extern int16_t actor_evaluate_flank_offset(const real_vector3d *cover_direction,
                                           real_vector3d *out_offset,
                                           const real_point3d *threat_position,
                                           const real_point3d *candidate_position); // 0x420b10, this module
extern void actor_scan_allies_for_backup_request(datum_index actor_index); // 0x420ec0, this module
extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag); // 0x421a40, EAX, BL
extern uint8_t actor_evaluate_custom_charge_trigger(datum_index actor_index);            // 0x424090, not yet rewritten
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.

// blam-cc: stack -> actor_index
void actor_update_crouch_state(datum_index actor_index)
{
    actor *self;
    Actor *actor_definition;
    prop *p;
    datum_index prop_index;
    float *threat_level;          // actor+0x350
    float *threat_level_smoothed; // actor+0x354
    uint8_t *crouching;           // actor+0x358
    int16_t *crouch_timer;        // actor+0x35a
    uint8_t *flag_35c;
    uint8_t *flag_35d;
    uint8_t *flag_35e;
    uint8_t *flag_35f;
    int16_t *countdown_360;
    int16_t *countdown_368;
    real_vector3d cover_direction;
    real_vector3d flank_offset;
    real_vector3d steering_direction;
    real_point3d probe_point;
    float decay;
    float threshold;
    float cosine_limit;
    float dot;
    int16_t combat_status;
    int16_t threat_class;
    int16_t grade;
    int16_t grade_second;
    uint8_t platoon_flag;
    uint8_t want_crouch;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    threat_level           = (float *)&self->unknown_350[0x00];
    threat_level_smoothed  = (float *)&self->unknown_350[0x04];
    crouching              = &self->unknown_350[0x08];
    crouch_timer           = (int16_t *)&self->unknown_350[0x0a];
    flag_35c               = &self->unknown_350[0x0c];
    flag_35d               = &self->unknown_350[0x0d];
    flag_35e               = &self->unknown_350[0x0e];
    flag_35f               = &self->unknown_350[0x0f];
    countdown_360          = (int16_t *)&self->unknown_350[0x10];
    countdown_368          = (int16_t *)&self->unknown_350[0x18];

    if (self->unknown_378 != 0 &&
        (self->combat_status == 0 || self->awareness_level < 3 ||
         (*(int32_t *)&self->unknown_1bc == 0x3f800000 && self->combat_status < 3))) {
        actor_set_combat_alert_flag(actor_index, 0); // 0x42141b: BL = 0
    }

    platoon_flag = self->unknown_1c9;
    if (self->unknown_374 != platoon_flag) {
        self->unknown_374 = platoon_flag;
        if (self->unit_index != (datum_index)k_datum_index_none) {
            ai_communication_broadcast((int16_t)((platoon_flag != 0) + 0x16), self->unit_index,
                                       0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0);
        }
    }

    if (self->unknown_378 == 0 && (actor_definition->flags & 0x800) == 0) {
        self->unknown_375 = 0;
    } else {
        self->unknown_375 = 1;
    }
    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if ((actor_definition->flags & 0x1000000) != 0 && self->unknown_374 == 0) {
            self->unknown_375 = 1;
        }
    } else {
        self->unknown_375 = 0;
    }

    for (threat_class = 9; threat_class > 0; threat_class--) {
        if ((int8_t)self->tally.by_threat_class[threat_class] > 0) {
            break;
        }
    }

    if (threat_class >= 8) {
        *threat_level = 2.0f;
    } else if (threat_class >= 7) {
        *threat_level = 1.8f;
    } else if (threat_class >= 6) {
        *threat_level = 1.6f;
    } else if (threat_class >= 5) {
        *threat_level = 1.2f;
    } else if (threat_class >= 3) {
        *threat_level = 0.7f;
    } else {
        *threat_level = 0.0f;
    }

    decay = (float)exp2(1.4426950408889634 * -0.04620981216430664);
    *threat_level_smoothed = (*threat_level - *threat_level_smoothed) * (1.0f - decay) +
                             *threat_level_smoothed;

    if (self->unknown_1c8 != 0) {
        self->unknown_3b4 = self->unknown_1b8;
    }

    if ((actor_definition->flags & 0xc0000000u) != 0) {
        if (self->active_unit_index == (datum_index)k_datum_index_none && self->combat_status > 2) {
            combat_status = self->target_combat_status;
            *flag_35d = 0;
            *flag_35c = 0;
            *flag_35e = 0;
            *flag_35f = 0;

            for (prop_index = self->first_prop; prop_index != (datum_index)k_datum_index_none;
                 prop_index = p->next_in_actor) {
                p = &((prop *)prop_data->data)[prop_index & 0xffff];

                if (p->state > 1 && p->state < 4 && p->enemy == 0 && p->dead == 0 &&
                    p->swarm_owned == 0 &&
                    (p->is_parented != 0 || p->relationship_object_index == -1)) {

                    // objdump: the 12 bytes 0x420970 fills at [esp+0x58] are the same slot
                    // ECX points at for every 0x420b10 call below.
                    if (actor_get_ranged_attack_vector(prop_index, actor_index, (real_vector3d *)&cover_direction) != 0) {
                        grade = actor_evaluate_flank_offset(&cover_direction, &flank_offset,
                                                            &self->body_position,
                                                            &p->last_known_position);
                        if (grade >= 1) {
                            *flag_35d = 1;
                            if (p->is_parented != 0) {
                                *flag_35c = 1;
                            }
                        }

                        if ((int32_t)actor_definition->flags < 0 && p->is_parented != 0 &&
                            p->shooting != 0 &&
                            vector3d_magnitude_squared(&flank_offset) < 1.0f &&
                            (self->moving != 0 || *countdown_360 > 0)) {

                            steering_direction.i = self->desired_movement_vector.x;
                            steering_direction.j = self->desired_movement_vector.y;
                            steering_direction.k = self->desired_movement_vector.z;
                            if (vector3d_normalize_with_length(&steering_direction) > 0.0f) {
                                probe_point.x = steering_direction.i * 0.4f + self->body_position.x;
                                probe_point.y = steering_direction.j * 0.4f + self->body_position.y;
                                probe_point.z = steering_direction.k * 0.4f + self->body_position.z;
                                grade_second = actor_evaluate_flank_offset(&cover_direction,
                                                                           (real_vector3d *)0,
                                                                           &probe_point,
                                                                           &p->last_known_position);
                                if (grade_second <= grade) {
                                    grade_second = grade;
                                }
                                if (grade_second >= 1) {
                                    dot = flank_offset.i * steering_direction.i +
                                          flank_offset.j * steering_direction.j +
                                          flank_offset.k * steering_direction.k;
                                    if (vector3d_magnitude_squared(&flank_offset) >= 0.25f) {
                                        cosine_limit = 0.8660254f;
                                    } else {
                                        cosine_limit = 0.0f;
                                    }
                                    if (cosine_limit < dot) {
                                        *flag_35f = 1;
                                    }
                                }
                            }
                        }
                    }

                    if (combat_status > 8) {
                        grade = actor_evaluate_flank_offset(&cover_direction, &flank_offset,
                                                            &self->body_position,
                                                            &p->last_known_position);
                        if (grade > 1) {
                            *flag_35e = 1;
                        }
                    }
                }
            }
        } else {
            *flag_35d = 0;
            *flag_35c = 0;
            *flag_35e = 0;
            *flag_35f = 0;
        }
    }

    if (*flag_35f == 0) {
        if (*countdown_360 > 0) {
            *countdown_360 = (int16_t)(*countdown_360 - 1);
        }
    } else {
        actor_push_recognition_entry(actor_index, ((struct actor *)self)->firing_position_index, 1); // 0x42186f
        *countdown_360 = 0x16;
    }

    if (*crouch_timer > 0) {
        *crouch_timer = (int16_t)(*crouch_timer - 1);
    } else {
        if (self->unknown_374 == 0 || self->unknown_378 != 0) {
            threshold = actor_definition->attacking_crouch_threshold;
        } else {
            threshold = actor_definition->defending_crouch_threshold;
        }

        switch (actor_definition->defensive_crouch_type) {
        case 1:
            want_crouch = (uint8_t)(*threat_level_smoothed > threshold);
            break;
        case 2:
            want_crouch = (uint8_t)(*(float *)&self->unknown_1bc > threshold);
            break;
        case 3:
            want_crouch = (uint8_t)(*(float *)&self->unknown_1bc > threshold &&
                                    (int8_t)self->tally.threat_class_2 >= 1);
            break;
        case 4:
            want_crouch = (uint8_t)(self->combat_status > 0);
            break;
        case 5:
            want_crouch = actor_evaluate_custom_charge_trigger(actor_index);
            break;
        default:
            want_crouch = 0;
            break;
        }

        if ((actor_definition->flags & 0x40000000u) != 0) {
            if (*flag_35c != 0) {
                want_crouch = 1;
            } else if (*flag_35e != 0) {
                want_crouch = 0;
            } else if (*flag_35d != 0) {
                want_crouch = 1;
            }
        }

        if (*crouching == 0) {
            if (want_crouch != 0) {
                *crouching = 1;
                threshold = actor_definition->min_crouch_time;
                if (threshold <= 0.0f) {
                    *crouch_timer = 0x2d;
                } else {
                    *crouch_timer = (int16_t)__ftol((double)(threshold * 30.0f));
                }
            }
        } else if (want_crouch == 0) {
            *crouching = 0;
            threshold = actor_definition->min_stand_time;
            if (threshold <= 0.0f) {
                *crouch_timer = 0x2d;
            } else {
                *crouch_timer = (int16_t)__ftol((double)(threshold * 30.0f));
            }
        }
    }

    if (*countdown_368 > 0) {
        *countdown_368 = (int16_t)(*countdown_368 - 1);
    }
    actor_scan_allies_for_backup_request(actor_index);
}

#if 0
Original Ghidra decompilation (0x4213b0):

void FUN_004213b0(uint param_1)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  undefined1 uVar6;
  char cVar7;
  short sVar8;
  short sVar9;
  short sVar10;
  undefined2 uVar11;
  int iVar12;
  int iVar13;
  float10 fVar14;
  float10 fVar15;
  float local_50;
  uint local_40;
  float local_30;
  float local_2c;
  float local_28;
  undefined1 local_c [12];

  iVar13 = (param_1 & 0xffff) * 0x724;
  iVar12 = *(int *)(DAT_00880360 + 0x34) + iVar13;
  puVar1 = *(uint **)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar13) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  if ((*(char *)(iVar12 + 0x378) != '\0') &&
     (((*(short *)(iVar12 + 0x6e) == 0 || (*(short *)(iVar12 + 0x6a) < 3)) ||
      ((*(int *)(iVar12 + 0x1bc) == 0x3f800000 && (*(short *)(iVar12 + 0x6e) < 3)))))) {
    actor_set_combat_alert_flag();
  }
  cVar7 = *(char *)(iVar12 + 0x1c9);
  if (*(char *)(iVar12 + 0x374) != cVar7) {
    *(char *)(iVar12 + 0x374) = cVar7;
    if (*(int *)(iVar12 + 0x18) != -1) {
      ai_communication_broadcast
                ((cVar7 != '\0') + '\x16',*(int *)(iVar12 + 0x18),0xffffffff,0xffffffff,0xffffffff,
                 0xffffffff,0);
    }
  }
  if ((*(char *)(iVar12 + 0x378) == '\0') && ((*puVar1 & 0x800) == 0)) {
    uVar6 = 0;
  }
  else {
    uVar6 = 1;
  }
  *(undefined1 *)(iVar12 + 0x375) = uVar6;
  if (*(int *)(iVar12 + 0x158) == -1) {
    if (((*puVar1 & 0x1000000) != 0) && (*(char *)(iVar12 + 0x374) == '\0')) {
      *(undefined1 *)(iVar12 + 0x375) = 1;
    }
  }
  else {
    *(undefined1 *)(iVar12 + 0x375) = 0;
  }
  sVar8 = 9;
  do {
    if ('\0' < *(char *)(sVar8 + 0x1ee + iVar12)) break;
    sVar8 = sVar8 + -1;
  } while (0 < sVar8);
  if (sVar8 < 8) {
    if (sVar8 < 7) {
      if (sVar8 < 6) {
        if (sVar8 < 5) {
          if (sVar8 < 3) {
            *(undefined4 *)(iVar12 + 0x350) = 0;
          }
          else {
            *(undefined4 *)(iVar12 + 0x350) = 0x3f333333;
          }
        }
        else {
          *(undefined4 *)(iVar12 + 0x350) = 0x3f99999a;
        }
      }
      else {
        *(undefined4 *)(iVar12 + 0x350) = 0x3fcccccd;
      }
    }
    else {
      *(undefined4 *)(iVar12 + 0x350) = 0x3fe66666;
    }
  }
  else {
    *(undefined4 *)(iVar12 + 0x350) = 0x40000000;
  }
  fVar14 = ROUND((float10)1.4426950408889634 * (float10)-0.04620981216430664);
  fVar15 = (float10)f2xm1((float10)1.4426950408889634 * (float10)-0.04620981216430664 - fVar14);
  fVar14 = (float10)fscale((float10)1 + fVar15,fVar14);
  *(float *)(iVar12 + 0x354) =
       (float)(((float10)*(float *)(iVar12 + 0x350) - (float10)*(float *)(iVar12 + 0x354)) *
               ((float10)1.0 - fVar14) + (float10)*(float *)(iVar12 + 0x354));
  if (*(char *)(iVar12 + 0x1c8) != '\0') {
    *(undefined4 *)(iVar12 + 0x3b4) = *(undefined4 *)(iVar12 + 0x1b8);
  }
  if ((*puVar1 & 0xc0000000) != 0) {
    if ((*(int *)(iVar12 + 0x158) == -1) && (2 < *(short *)(iVar12 + 0x6e))) {
      sVar8 = *(short *)(iVar12 + 0x268);
      *(undefined1 *)(iVar12 + 0x35d) = 0;
      *(undefined1 *)(iVar12 + 0x35c) = 0;
      *(undefined1 *)(iVar12 + 0x35e) = 0;
      *(undefined1 *)(iVar12 + 0x35f) = 0;
      local_40 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x50 + iVar13);
      while (local_40 != 0xffffffff) {
        iVar13 = (local_40 & 0xffff) * 0x138;
        local_40 = *(uint *)(iVar13 + 8 + *(int *)(DAT_008802c0 + 0x34));
        iVar13 = iVar13 + *(int *)(DAT_008802c0 + 0x34);
        if (((((1 < *(short *)(iVar13 + 0x24)) && (*(short *)(iVar13 + 0x24) < 4)) &&
             (*(char *)(iVar13 + 0x60) == '\0')) &&
            ((*(char *)(iVar13 + 0x127) == '\0' && (*(char *)(iVar13 + 0x14) == '\0')))) &&
           ((*(char *)(iVar13 + 0x12e) != '\0' || (*(int *)(iVar13 + 0x110) == -1)))) {
          cVar7 = FUN_00420970(local_c);
          if (cVar7 != '\0') {
            sVar9 = FUN_00420b10();
            if ((0 < sVar9) &&
               (*(undefined1 *)(iVar12 + 0x35d) = 1, *(char *)(iVar13 + 0x12e) != '\0')) {
              *(undefined1 *)(iVar12 + 0x35c) = 1;
            }
            if (((((int)*puVar1 < 0) && (*(char *)(iVar13 + 0x12e) != '\0')) &&
                (*(char *)(iVar13 + 0x12f) != '\0')) &&
               ((fVar14 = (float10)FUN_00401000(), fVar14 < (float10)1.0 &&
                ((*(char *)(iVar12 + 0x504) != '\0' || (0 < *(short *)(iVar12 + 0x360))))))) {
              fVar2 = *(float *)(iVar12 + 0x518);
              fVar3 = *(float *)(iVar12 + 0x51c);
              fVar4 = *(float *)(iVar12 + 0x520);
              fVar14 = (float10)vector3d_normalize_with_length();
              if ((float10)0.0 < fVar14) {
                sVar10 = FUN_00420b10();
                if (sVar10 <= sVar9) {
                  sVar10 = sVar9;
                }
                if (0 < sVar10) {
                  fVar14 = (float10)FUN_00401000();
                  if ((float10)0.25 <= fVar14) {
                    fVar5 = 0.8660254;
                  }
                  else {
                    fVar5 = 0.0;
                  }
                  if (fVar5 < local_30 * fVar2 + local_2c * fVar3 + local_28 * fVar4) {
                    *(undefined1 *)(iVar12 + 0x35f) = 1;
                  }
                }
              }
            }
          }
          if ((8 < sVar8) && (sVar9 = FUN_00420b10(), 1 < sVar9)) {
            *(undefined1 *)(iVar12 + 0x35e) = 1;
          }
        }
      }
    }
    else {
      *(undefined1 *)(iVar12 + 0x35d) = 0;
      *(undefined1 *)(iVar12 + 0x35c) = 0;
      *(undefined1 *)(iVar12 + 0x35e) = 0;
      *(undefined1 *)(iVar12 + 0x35f) = 0;
    }
  }
  if (*(char *)(iVar12 + 0x35f) == '\0') {
    if (0 < *(short *)(iVar12 + 0x360)) {
      *(short *)(iVar12 + 0x360) = *(short *)(iVar12 + 0x360) + -1;
    }
  }
  else {
    FUN_004141a0();
    *(undefined2 *)(iVar12 + 0x360) = 0x16;
  }
  if (0 < *(short *)(iVar12 + 0x35a)) {
    *(short *)(iVar12 + 0x35a) = *(short *)(iVar12 + 0x35a) + -1;
    goto LAB_004219fa;
  }
  if ((*(char *)(iVar12 + 0x374) == '\0') || (*(char *)(iVar12 + 0x378) != '\0')) {
    local_50 = (float)puVar1[0xbf];
  }
  else {
    local_50 = (float)puVar1[0xc0];
  }
  switch((short)puVar1[0xbe]) {
  case 1:
    if (*(float *)(iVar12 + 0x354) <= local_50) goto switchD_004218f8_default;
    cVar7 = '\x01';
    break;
  case 2:
    if (local_50 <= *(float *)(iVar12 + 0x1bc)) goto switchD_004218f8_default;
    cVar7 = '\x01';
    break;
  case 3:
    if ((*(float *)(iVar12 + 0x1bc) <= local_50) || (*(char *)(iVar12 + 0x1f9) < '\x01'))
    goto switchD_004218f8_default;
    cVar7 = '\x01';
    break;
  case 4:
    cVar7 = 0 < *(short *)(iVar12 + 0x6e);
    break;
  case 5:
    cVar7 = FUN_00424090(param_1);
    break;
  default:
switchD_004218f8_default:
    cVar7 = '\0';
  }
  if ((*puVar1 & 0x40000000) != 0) {
    if (*(char *)(iVar12 + 0x35c) == '\0') {
      if (*(char *)(iVar12 + 0x35e) == '\0') {
        if (*(char *)(iVar12 + 0x35d) != '\0') goto LAB_00421984;
      }
      else {
        cVar7 = '\0';
      }
    }
    else {
LAB_00421984:
      cVar7 = '\x01';
    }
  }
  if (*(char *)(iVar12 + 0x358) == '\0') {
LAB_004219b9:
    if (cVar7 == '\0') goto LAB_004219fa;
    *(undefined1 *)(iVar12 + 0x358) = 1;
    fVar2 = (float)puVar1[0xc2];
  }
  else {
    if (cVar7 != '\0') {
      if (*(char *)(iVar12 + 0x358) != '\0') goto LAB_004219fa;
      goto LAB_004219b9;
    }
    *(undefined1 *)(iVar12 + 0x358) = 0;
    fVar2 = (float)puVar1[0xc1];
  }
  if (fVar2 <= 0.0) {
    *(undefined2 *)(iVar12 + 0x35a) = 0x2d;
  }
  else {
    uVar11 = __ftol();
    *(undefined2 *)(iVar12 + 0x35a) = uVar11;
  }
LAB_004219fa:
  if (0 < *(short *)(iVar12 + 0x368)) {
    *(short *)(iVar12 + 0x368) = *(short *)(iVar12 + 0x368) + -1;
  }
  FUN_00420ec0(param_1);
  return;
}
#endif
