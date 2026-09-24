// actor_danger_update_reaction  (Ghidra: actor_danger_update_reaction, renamed)
// address 0x41eda0, size 1533 bytes
// name confidence: 0.45  rewrite confidence: 0.6
// evidence: runs once per tick over the single registered danger (actor.danger_type, set by
// actor_danger_register_point @0x41ec90 and actor_danger_register_stationary_object
// @0x41ea60). It refreshes the danger geometry -- the danger object's position into
// actor.flee_from_point, its velocity into actor+0x2bc, a 45-tick extrapolated end point into
// danger_segment_end, the midpoint into danger_center, the half-length plus
// danger_unknown_294 into danger_radius and the distance from the actor's own body position
// into danger_unknown_2d4 -- and then, per danger type, decides whether the actor has
// actually noticed it. When it has, and it had not noticed it on the previous tick, it emits
// perception event 0xc with payload 5.
// register convention: actor_index is a genuine stack parameter (objdump: [esp+0x60]).
// blam-cc: stack -> actor_index
//
// Facts recovered from the disassembly that Ghidra hides:
//  - the 0x38-byte block at [esp+0x34] is actor_get_firing_positions' out-parameter
//    (EAX = actor_index, ECX = block, EDX = &actor.flee_from_point), so Ghidra's
//    local_2c / local_28 / local_24 are block dwords 3..5, i.e. the copy of
//    actor.body_position that function makes, not the output of object_get_position.
//  - object_get_position is called as (EAX = &actor.flee_from_point, ECX = danger_object_index).
//  - Ghidra's local_58 is one 16-bit stack slot used for two unrelated things in two mutually
//    exclusive branches: an out-parameter of 0x564390 in the type-1 branch and a one-byte
//    "already handled" flag in the type-3 branch. Split into two locals here.
// UNSURE: 0x41bb30, 0x42b270, 0x564390, 0x4142d0 and 0x43ea80 are not rewritten; their
// argument lists are what this call site allows.
// UNSURE: actor+0x2bc..0x2c7 is a velocity vector (copied straight out of object.velocity);
// types/ai.h still calls it unknown_2bc[12] and this file does not rename it.
// UNSURE: Ghidra's fVar13 is used as both a float and a pointer in the type-3 branch, which is
// a register-tracking artefact; only its pointer uses are transcribed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern data_array *encounter_data;   // 0x008802c8
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function
extern int32_t fistp_round(float x); // Ghidra ROUND(): a bare x87 fistp, i.e. round-to-nearest-even

extern void actor_begin_vocalization(int16_t event, int16_t priority, void *payload); // 0x4142d0, not yet rewritten
// EBX carries a 16-bit selector this call site leaves live; the six visible arguments are the
// six stack dwords (add esp,0x18 at 0x41f0e3 and 0x41f2d0).
extern int16_t actor_dispatch_look_handler_by_posture(datum_index actor_index, const void *block, const real_point3d *point,
                            int32_t a, int32_t b, uint8_t stance); // 0x41bb30, not yet rewritten
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index,
                                           void *target_ref, int16_t gate,
                                           real_point3d *listener_position); // 0x41c030, this module
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, EAX/ECX/EDX
extern int16_t actor_evaluate_engagement_reachability(uint32_t kind, uint32_t enabled, uint32_t object_index, uint32_t in_vehicle); // 0x42b270, not yet rewritten
extern datum_index actor_find_prop_for_object(datum_index object_index); // 0x43ea80, UNSURE signature
extern int16_t unit_get_animation_frames_remaining(int16_t *out_kind); // 0x564390, not yet rewritten
extern void *object_try_and_get(int32_t kind);  // 0x4f6ec0
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, EAX/ECX
extern datum_index object_get_root_object_index(void); // 0x4f6fb0

// blam-cc: stack -> actor_index
void actor_danger_update_reaction(datum_index actor_index)
{
    actor *self;
    object *danger_object;
    object *root_object;
    encounter *owning_encounter;
    datum_index related_prop_index;
    datum_index root_object_index;
    real_vector3d *danger_velocity;
    uint8_t noticed;
    uint8_t is_own_unit;
    uint8_t already_handled;
    uint8_t stance;
    int16_t danger_type;
    int16_t detection;
    int16_t sound_kind;
    int16_t sound_result;
    int16_t status;
    float dx, dy, dz;
    float shield_fraction;
    int16_t event_payload[8];
    uint8_t position_block[0x38];
    float body_x, body_y, body_z;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->danger_type < 1) {
        return;
    }

    danger_object = (object *)object_try_and_get(-1);
    if (danger_object == (object *)0) {
        self->danger_type = 0;
        return;
    }

    object_get_position(&self->flee_from_point, self->danger_object_index);
    actor_get_firing_positions(actor_index, (uint32_t *)position_block, &self->flee_from_point);
    body_x = ((float *)position_block)[3];
    body_y = ((float *)position_block)[4];
    body_z = ((float *)position_block)[5];

    danger_velocity = (real_vector3d *)&self->unknown_2bc;
    danger_velocity->i = danger_object->velocity.i;
    danger_velocity->j = danger_object->velocity.j;
    danger_velocity->k = danger_object->velocity.k;

    dy = self->flee_from_point.y - body_y;
    dz = self->flee_from_point.z - body_z;
    dx = self->flee_from_point.x - body_x;
    self->danger_unknown_2d4 = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

    self->danger_segment_end.x = danger_velocity->i * 45.0f + self->flee_from_point.x;
    self->danger_segment_end.y = danger_velocity->j * 45.0f + self->flee_from_point.y;
    self->danger_segment_end.z = danger_velocity->k * 45.0f + self->flee_from_point.z;
    self->danger_center.x = (self->danger_segment_end.x + self->flee_from_point.x) * 0.5f;
    self->danger_center.y = (self->flee_from_point.y + self->danger_segment_end.y) * 0.5f;
    self->danger_center.z = (self->flee_from_point.z + self->danger_segment_end.z) * 0.5f;

    dx = self->flee_from_point.x - self->danger_center.x;
    dy = self->flee_from_point.y - self->danger_center.y;
    dz = self->flee_from_point.z - self->danger_center.z;
    self->danger_radius = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) +
                          self->danger_unknown_294;

    danger_type = self->danger_type;
    noticed = 0;
    is_own_unit = 0;

    if (danger_type == 1) {
        noticed = self->danger_unknown_286;
        if (noticed == 0) {
            related_prop_index = actor_find_prop_for_object(self->danger_object_index);
            if (related_prop_index != (datum_index)k_datum_index_none) {
                noticed = (uint8_t)(((prop *)prop_data->data)[related_prop_index & 0xffff].unknown_30 > 1);
            }
        }
        sound_result = unit_get_animation_frames_remaining(&sound_kind);
        if (sound_kind != 0x19) {
            sound_result = -1;
        }
        *(int16_t *)&self->unknown_2e8[0] = sound_result;
        if (noticed == 0) {
            goto store_and_return;
        }
    } else if (danger_type == 2) {
        if (self->unit_index != (datum_index)k_datum_index_none &&
            *(datum_index *)((uint8_t *)danger_object + 0x11c) == self->unit_index) {
            is_own_unit = 1;
        }
        if (*(float *)((uint8_t *)danger_object + 0x240) <= 0.0f ||
            *(float *)((uint8_t *)danger_object + 0x244) <= 0.0f) {
            *(int16_t *)&self->unknown_2e8[0] = -1;
        } else {
            shield_fraction = (1.0f - *(float *)((uint8_t *)danger_object + 0x240)) /
                              *(float *)((uint8_t *)danger_object + 0x244);
            *(int16_t *)&self->unknown_2e8[0] = (int16_t)fistp_round(shield_fraction);
        }
        if (self->danger_unknown_286 == 0 && is_own_unit == 0) {
            noticed = 0;
            owning_encounter = (encounter *)0;
            if (self->encounter_index != (datum_index)k_datum_index_none) {
                owning_encounter = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
            }
            if (self->awareness_level == 1 ||
                (owning_encounter != (encounter *)0 &&
                 *((uint8_t *)owning_encounter + 0x40) != 0) ||
                *(float *)((uint8_t *)tag_instances[danger_object->definition_tag & 0xffff].data + 0x19c) <=
                    self->danger_unknown_2d4) {
                goto store_and_return;
            }
            if (*(datum_index *)((uint8_t *)danger_object + 0x11c) != (datum_index)k_datum_index_none) {
                object_get_root_object_index();
            }
            actor_evaluate_engagement_reachability(0, 0, (uint32_t)self->danger_object_index,
                         self->active_unit_index != (datum_index)k_datum_index_none);
            self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
            if (self->unknown_6e < 2) {
                stance = (uint8_t)(self->awareness_level > 2);
            } else {
                stance = 2;
            }
            detection = actor_dispatch_look_handler_by_posture(actor_index, position_block, &self->flee_from_point, 0, 1, stance);
            if (detection < 2) {
                goto store_and_return;
            }
            noticed = 1;
        } else {
            noticed = 1;
        }
    } else {
        if (danger_type != 3) {
            goto store_and_return;
        }
        if (danger_object->velocity.k * danger_object->velocity.k +
            danger_object->velocity.j * danger_object->velocity.j +
            danger_object->velocity.i * danger_object->velocity.i < 4.4444445e-05f ||
            *(float *)((uint8_t *)tag_instances[danger_object->definition_tag & 0xffff].data + 4) +
                10.0f < self->danger_unknown_2d4) {
            self->danger_type = 0;
            goto store_and_return;
        }
        noticed = self->danger_unknown_286;
        if (noticed == 0) {
            if (*(datum_index *)((uint8_t *)danger_object + 0x324) != (datum_index)k_datum_index_none) {
                related_prop_index = actor_find_prop_for_object(*(datum_index *)((uint8_t *)danger_object + 0x324));
                if (related_prop_index != (datum_index)k_datum_index_none) {
                    noticed = (uint8_t)(((prop *)prop_data->data)[related_prop_index & 0xffff].unknown_30 > 1);
                    if (noticed == 0) {
                        goto store_and_return;
                    }
                    goto emit_event;
                }
            }
            owning_encounter = (encounter *)0;
            if (self->encounter_index != (datum_index)k_datum_index_none) {
                owning_encounter = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
            }
            already_handled = 0;
            if (self->awareness_level == 1 ||
                (owning_encounter != (encounter *)0 &&
                 *((uint8_t *)owning_encounter + 0x40) != 0)) {
                already_handled = 1;
            }
            root_object = danger_object;
            if (*(datum_index *)((uint8_t *)danger_object + 0x11c) != (datum_index)k_datum_index_none) {
                root_object_index = object_get_root_object_index();
                root_object = ((object_header *)object_data->data)[root_object_index & 0xffff].data;
            }
            status = actor_evaluate_engagement_reachability(0, 0, (uint32_t)self->danger_object_index,
                                  self->active_unit_index != (datum_index)k_datum_index_none);
            if (already_handled == 0) {
                self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
                if (self->unknown_6e < 2) {
                    stance = (uint8_t)(self->awareness_level > 2);
                } else {
                    stance = 2;
                }
                detection = actor_dispatch_look_handler_by_posture(actor_index, position_block, &self->flee_from_point, 0, 1, stance);
                if (detection > 1) {
                    noticed = 1;
                    goto emit_event;
                }
            }
            if ((int16_t)actor_target_hearing_check((uint8_t *)root_object + 0x98, status, actor_index,
                                                    (void *)0, 0, &self->flee_from_point) < 2) {
                goto store_and_return;
            }
            noticed = 1;
        }
    }

emit_event:
    if (self->danger_unknown_286 == 0) {
        event_payload[0] = 5;
        actor_begin_vocalization(0xc, 1, event_payload);
    }

store_and_return:
    self->danger_unknown_286 = noticed;
    self->unknown_28a = is_own_unit;
}

#if 0
Original Ghidra decompilation (0x41eda0):

void FUN_0041eda0(uint param_1)

{
  float *pfVar1;
  float fVar2;
  uint *puVar3;
  float fVar4;
  ushort uVar5;
  undefined1 uVar6;
  short sVar7;
  undefined2 uVar8;
  uint *puVar9;
  int iVar10;
  uint uVar11;
  undefined4 uVar12;
  float fVar13;
  int iVar14;
  undefined8 uVar15;
  char local_5a;
  char local_59;
  undefined2 local_58;
  uint *local_54;
  int local_50;
  float local_4c;
  undefined2 local_48 [8];
  undefined1 local_38 [12];
  float local_2c;
  float local_28;
  float local_24;

  local_50 = (param_1 & 0xffff) * 0x724;
  iVar14 = *(int *)(DAT_00880360 + 0x34) + local_50;
  if (*(short *)(iVar14 + 0x280) < 1) {
    return;
  }
  puVar9 = (uint *)object_try_and_get(0xffffffff);
  if (puVar9 == (uint *)0x0) {
    *(undefined2 *)(iVar14 + 0x280) = 0;
    return;
  }
  pfVar1 = (float *)(iVar14 + 0x2b0);
  local_54 = puVar9;
  object_get_position();
  actor_get_firing_positions();
  puVar3 = local_54;
  *(float *)(iVar14 + 700) = (float)puVar9[0x1a];
  *(uint *)(iVar14 + 0x2c0) = puVar9[0x1b];
  *(uint *)(iVar14 + 0x2c4) = puVar9[0x1c];
  local_28 = *(float *)(iVar14 + 0x2b4) - local_28;
  local_24 = *(float *)(iVar14 + 0x2b8) - local_24;
  *(float *)(iVar14 + 0x2d4) =
       SQRT((*pfVar1 - local_2c) * (*pfVar1 - local_2c) + local_28 * local_28 + local_24 * local_24)
  ;
  *(float *)(iVar14 + 0x2c8) = *(float *)(iVar14 + 700) * 45.0 + *pfVar1;
  *(float *)(iVar14 + 0x2cc) = *(float *)(iVar14 + 0x2c0) * 45.0 + *(float *)(iVar14 + 0x2b4);
  *(float *)(iVar14 + 0x2d0) = *(float *)(iVar14 + 0x2c4) * 45.0 + *(float *)(iVar14 + 0x2b8);
  *(float *)(iVar14 + 0x2dc) = (*(float *)(iVar14 + 0x2c8) + *pfVar1) * 0.5;
  *(float *)(iVar14 + 0x2e0) = (*(float *)(iVar14 + 0x2b4) + *(float *)(iVar14 + 0x2cc)) * 0.5;
  *(float *)(iVar14 + 0x2e4) = (*(float *)(iVar14 + 0x2b8) + *(float *)(iVar14 + 0x2d0)) * 0.5;
  fVar2 = *pfVar1 - *(float *)(iVar14 + 0x2dc);
  fVar4 = *(float *)(iVar14 + 0x2b4) - *(float *)(iVar14 + 0x2e0);
  fVar13 = *(float *)(iVar14 + 0x2b8) - *(float *)(iVar14 + 0x2e4);
  sVar7 = *(short *)(iVar14 + 0x280);
  local_5a = '\0';
  local_59 = '\0';
  *(float *)(iVar14 + 0x2d8) =
       SQRT(fVar2 * fVar2 + fVar4 * fVar4 + fVar13 * fVar13) + *(float *)(iVar14 + 0x294);
  if (sVar7 == 1) {
    local_5a = *(char *)(iVar14 + 0x286);
    if ((local_5a == '\0') &&
       (uVar11 = FUN_0043ea80(*(undefined4 *)(iVar14 + 0x28c)), uVar11 != 0xffffffff)) {
      local_5a = 1 < *(short *)((uVar11 & 0xffff) * 0x138 + 0x30 + *(int *)(DAT_008802c0 + 0x34));
    }
    uVar8 = FUN_00564390(&local_58);
    if (local_58 != 0x19) {
      uVar8 = 0xffff;
    }
    *(undefined2 *)(iVar14 + 0x2e8) = uVar8;
LAB_0041f353:
    if (local_5a == '\0') goto LAB_0041f381;
  }
  else if (sVar7 == 2) {
    if ((*(uint *)(iVar14 + 0x18) != 0xffffffff) && (local_54[0x47] == *(uint *)(iVar14 + 0x18))) {
      local_59 = '\x01';
    }
    if (((float)local_54[0x90] <= 0.0) || ((float)local_54[0x91] <= 0.0)) {
      *(undefined2 *)(iVar14 + 0x2e8) = 0xffff;
    }
    else {
      local_4c = (1.0 - (float)local_54[0x90]) / (float)local_54[0x91];
      local_54 = (uint *)(int)ROUND(local_4c);
      *(undefined2 *)(iVar14 + 0x2e8) = local_54._0_2_;
    }
    if ((*(char *)(iVar14 + 0x286) == '\0') && (local_59 == '\0')) {
      local_5a = '\0';
      if (*(uint *)(iVar14 + 0x34) == 0xffffffff) {
        iVar10 = 0;
      }
      else {
        iVar10 = (*(uint *)(iVar14 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      }
      if ((*(short *)(iVar14 + 0x6a) == 1) ||
         (((iVar10 != 0 && (*(char *)(iVar10 + 0x40) != '\0')) ||
          (*(float *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x19c) <=
           *(float *)(iVar14 + 0x2d4))))) goto LAB_0041f381;
      if (puVar3[0x47] != 0xffffffff) {
        object_get_root_object_index();
      }
      FUN_0042b270(0,0,*(undefined4 *)(iVar14 + 0x28c),*(int *)(iVar14 + 0x158) != -1);
      iVar10 = *(int *)(DAT_00880360 + 0x34) + local_50;
      if (*(short *)(iVar10 + 0x6e) < 2) {
        uVar6 = 2 < *(short *)(iVar10 + 0x6a);
      }
      else {
        uVar6 = 2;
      }
      sVar7 = FUN_0041bb30(param_1,local_38,pfVar1,0,1,uVar6);
      if (sVar7 < 2) goto LAB_0041f381;
      local_5a = '\x01';
    }
    else {
      local_5a = '\x01';
    }
  }
  else {
    if (sVar7 != 3) goto LAB_0041f381;
    fVar2 = (float)puVar9[0x1a];
    fVar13 = *(float *)((*local_54 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (((float)puVar9[0x1c] * (float)puVar9[0x1c] +
         (float)puVar9[0x1b] * (float)puVar9[0x1b] + fVar2 * fVar2 < 4.4444445e-05) ||
       (*(float *)((int)fVar13 + 4) + 10.0 < *(float *)(iVar14 + 0x2d4))) {
      *(undefined2 *)(iVar14 + 0x280) = 0;
      goto LAB_0041f381;
    }
    local_5a = *(char *)(iVar14 + 0x286);
    local_4c = fVar13;
    if (local_5a == '\0') {
      if (local_54[0xc9] != 0xffffffff) {
        uVar15 = FUN_0043ea80(local_54[0xc9]);
        fVar13 = (float)((ulonglong)uVar15 >> 0x20);
        if ((uint)uVar15 != 0xffffffff) {
          local_5a = 1 < *(short *)(((uint)uVar15 & 0xffff) * 0x138 + 0x30 +
                                   *(int *)(DAT_008802c0 + 0x34));
          goto LAB_0041f353;
        }
      }
      if (*(uint *)(iVar14 + 0x34) == 0xffffffff) {
        iVar10 = 0;
      }
      else {
        fVar13 = *(float *)(DAT_008802c8 + 0x34);
        iVar10 = (*(uint *)(iVar14 + 0x34) & 0xffff) * 0x6c + (int)fVar13;
      }
      uVar5 = local_58 >> 8;
      local_58 = local_58 & 0xff00;
      if ((*(short *)(iVar14 + 0x6a) == 1) || ((iVar10 != 0 && (*(char *)(iVar10 + 0x40) != '\0'))))
      {
        local_58 = CONCAT11((char)uVar5,1);
      }
      if (puVar3[0x47] != 0xffffffff) {
        uVar11 = object_get_root_object_index();
        fVar13 = (float)((uVar11 & 0xffff) * 3);
        puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar11 & 0xffff) * 0xc);
      }
      local_54 = puVar3 + 0x26;
      uVar12 = FUN_0042b270(0,0,*(undefined4 *)(iVar14 + 0x28c),
                            CONCAT31((int3)((uint)fVar13 >> 8),*(int *)(iVar14 + 0x158) != -1));
      puVar9 = puVar3 + 0x26;
      if ((char)local_58 == '\0') {
        iVar10 = *(int *)(DAT_00880360 + 0x34) + local_50;
        if (*(short *)(iVar10 + 0x6e) < 2) {
          uVar6 = 2 < *(short *)(iVar10 + 0x6a);
        }
        else {
          uVar6 = 2;
        }
        sVar7 = FUN_0041bb30(param_1,local_38,pfVar1,0,1,uVar6);
        puVar9 = local_54;
        if (1 < sVar7) {
          local_5a = '\x01';
          goto LAB_0041f35b;
        }
      }
      sVar7 = FUN_0041c030(puVar9,uVar12);
      if (sVar7 < 2) goto LAB_0041f381;
      local_5a = '\x01';
    }
  }
LAB_0041f35b:
  if (*(char *)(iVar14 + 0x286) == '\0') {
    local_48[0] = 5;
    FUN_004142d0(0xc,1,local_48);
  }
LAB_0041f381:
  *(char *)(iVar14 + 0x286) = local_5a;
  *(char *)(iVar14 + 0x28a) = local_59;
  return;
}
#endif
