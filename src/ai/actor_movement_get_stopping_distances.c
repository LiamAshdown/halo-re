// actor_movement_get_stopping_distances  (Ghidra: actor_movement_get_stopping_distances, renamed)
// address 0x4173a0, size 460 bytes
// name confidence: 0.45  rewrite confidence: 0.7
// evidence: its only caller is 0x4180c0, the steering routine. It reads the unit's forward
// speed (object.velocity dotted with object.forward) plus the unit definition's per-second
// top speed, acceleration and deceleration scaled to per-tick (x 1/30), and returns two
// classic kinematic distances: v^2 / (2a) to stop from the current speed, and the
// accelerate-to-top-speed-then-stop distance. For a driven unit it takes
// Vehicle.maximum_forward_speed (0x2f8) and Vehicle.speed_acceleration (0x300) instead, and
// uses the same value for both the acceleration and the deceleration term.
// register convention: actor_index in EAX, the accelerate-then-stop out-parameter in EBX and
// the plain stop-distance out-parameter in EDI; both may be null.
// blam-cc: EAX -> actor_index, EBX -> out_accelerate_stop_distance, EDI -> out_stop_distance
// UNSURE: the three fallback constants (1/12, 1/60, 0.8/30) are what the original loads when
// neither branch applies; nothing names them.
// UNSURE: object_try_and_get(1) is called with its register arguments dropped by Ghidra, as
// it is everywhere else in this module; the "1" is the literal the call site pushes.
// UNSURE: actor.unknown_508 is the byte actor_movement_update writes from its crouch
// selector, so the crouch_velocity_modifier scaling below is gated on crouching -- inferred
// from that one write, not stated anywhere.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;       // 0x00880360
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

// blam-cc: EAX -> actor_index, EBX -> out_accelerate_stop_distance, EDI -> out_stop_distance
void actor_movement_get_stopping_distances(datum_index actor_index,
                                           float *out_accelerate_stop_distance,
                                           float *out_stop_distance)
{
    actor *self;
    object *unit_object;
    Biped *biped_definition;
    Vehicle *vehicle_definition;
    float speed;
    float top_speed;
    float acceleration;
    float deceleration;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    speed = 0.0f;
    top_speed = 0.083333336f;
    acceleration = 0.016666668f;
    deceleration = 0.026666667f;

    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if (self->unit_index != (datum_index)k_datum_index_none) {
            unit_object = (object *)object_try_and_get(self->unit_index, 1); // 0x417458: ECX = actor +0x18
            if (unit_object != (object *)0) {
                biped_definition = (Biped *)tag_instances[unit_object->definition_tag & 0xffff].data;
                speed = unit_object->velocity.i * unit_object->forward.i +
                        unit_object->velocity.j * unit_object->forward.j +
                        unit_object->velocity.k * unit_object->forward.k;
                if ((biped_definition->biped_flags & 4) != 0) {
                    top_speed = biped_definition->max_velocity * 0.033333335f;
                    acceleration = biped_definition->acceleration * 0.033333335f;
                    deceleration = biped_definition->deceleration * 0.033333335f;
                    if (self->crouching != 0 && biped_definition->crouch_velocity_modifier > 0.0f) {
                        top_speed = top_speed * biped_definition->crouch_velocity_modifier;
                        acceleration = acceleration * biped_definition->crouch_velocity_modifier;
                        deceleration = deceleration * biped_definition->crouch_velocity_modifier;
                    }
                }
            }
        }
    } else if (self->vehicle_driving_type > 1 && self->vehicle_driving_type < 4) {
        unit_object = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        vehicle_definition = (Vehicle *)tag_instances[unit_object->definition_tag & 0xffff].data;
        speed = unit_object->velocity.i * unit_object->forward.i +
                unit_object->velocity.j * unit_object->forward.j +
                unit_object->velocity.k * unit_object->forward.k;
        top_speed = vehicle_definition->maximum_forward_speed;
        deceleration = vehicle_definition->speed_acceleration;
        acceleration = deceleration;
    }

    if (out_stop_distance != (float *)0) {
        *out_stop_distance = (speed * speed) / (deceleration + deceleration);
    }
    if (out_accelerate_stop_distance != (float *)0) {
        if (top_speed < speed) {
            top_speed = speed;
        }
        *out_accelerate_stop_distance = (top_speed * top_speed) / (deceleration + deceleration) +
                                        (top_speed * top_speed - speed * speed) /
                                        (acceleration + acceleration);
    }
}

#if 0
Original Ghidra decompilation (0x4173a0):

void FUN_004173a0(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  uint *puVar3;
  float *unaff_EBX;
  float *unaff_EDI;
  float10 fVar4;
  float10 extraout_ST0;
  float local_c;
  float local_8;
  float local_4;

  fVar4 = (float10)0.083333336;
  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_c = 0.0;
  local_4 = 0.016666668;
  local_8 = 0.026666667;
  if (*(uint *)(iVar2 + 0x158) == 0xffffffff) {
    if ((*(int *)(iVar2 + 0x18) != -1) &&
       (puVar3 = (uint *)object_try_and_get(1), fVar4 = extraout_ST0, puVar3 != (uint *)0x0)) {
      iVar1 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      local_c = (float)puVar3[0x1a] * (float)puVar3[0x1d] +
                (float)puVar3[0x1e] * (float)puVar3[0x1b] +
                (float)puVar3[0x1f] * (float)puVar3[0x1c];
      if ((*(byte *)(iVar1 + 0x2f4) & 4) != 0) {
        fVar4 = (float10)*(float *)(iVar1 + 0x334) * (float10)0.033333335;
        local_4 = *(float *)(iVar1 + 0x33c) * 0.033333335;
        local_8 = *(float *)(iVar1 + 0x340) * 0.033333335;
        if ((*(char *)(iVar2 + 0x508) != '\0') && (0.0 < *(float *)(iVar1 + 0x34c))) {
          fVar4 = fVar4 * (float10)*(float *)(iVar1 + 0x34c);
          local_4 = local_4 * *(float *)(iVar1 + 0x34c);
          local_8 = local_8 * *(float *)(iVar1 + 0x34c);
        }
      }
    }
  }
  else if ((1 < *(short *)(iVar2 + 0x15e)) && (*(short *)(iVar2 + 0x15e) < 4)) {
    puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar2 + 0x158) & 0xffff) * 0xc);
    iVar2 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_c = (float)puVar3[0x1a] * (float)puVar3[0x1d] +
              (float)puVar3[0x1b] * (float)puVar3[0x1e] + (float)puVar3[0x1c] * (float)puVar3[0x1f];
    fVar4 = (float10)*(float *)(iVar2 + 0x2f8);
    local_8 = *(float *)(iVar2 + 0x300);
    local_4 = local_8;
  }
  if (unaff_EDI != (float *)0x0) {
    *unaff_EDI = (local_c * local_c) / (local_8 + local_8);
  }
  if (unaff_EBX != (float *)0x0) {
    if (fVar4 < (float10)local_c) {
      fVar4 = (float10)local_c;
    }
    *unaff_EBX = (float)((fVar4 * fVar4) / ((float10)local_8 + (float10)local_8) +
                        (fVar4 * fVar4 - (float10)local_c * (float10)local_c) /
                        ((float10)local_4 + (float10)local_4));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
