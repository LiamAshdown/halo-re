// actor_movement_flying_needs_steering  (Ghidra: actor_movement_flying_needs_steering, renamed)
// address 0x41aab0, size 275 bytes
// name confidence: 0.45  rewrite confidence: 0.8
// evidence: called from actor_movement_action_resolve on the flying/vehicle branch, in the
// slot where the ground branch tests "the destination has a valid surface index or a
// non-zero radius". It answers 1 (steer to the destination) unless the actor is in
// movement mode 4 with a positive Vehicle.ai_avoidance_distance (Vehicle+0x388), a smoothing
// factor above 0.9, and a destination that is already within about 10 degrees of the actor
// facing vector -- in which case there is nothing to steer around and it answers 0, which
// makes the caller complete the movement action. The avoidance distance is reported through
// the optional out-parameter.
// register convention: actor_index in EAX, destination in ECX, out_avoidance_distance in EDI
// (all three confirmed by objdump: fld [ecx], fsub [esi+0x12c] builds the delta; the
// normalize call takes lea ecx,[esp+0xc]; and the tail is test edi,edi / mov [edi],ecx).
// blam-cc: EAX -> actor_index, ECX -> destination, EDI -> out_avoidance_distance
// UNSURE: Ghidra re-uses the pre-normalization component expressions for the dot product,
// which cannot be what the code does (the threshold 0.984 is a cosine). The disassembly
// settles it: vector3d_normalize_with_length normalizes the stack delta in place at
// [esp+0xc..0x14] and the fmul chain at 0x41ab80 reads those same slots back, so the dot
// product is taken against the normalized delta. Written that way here.
// UNSURE: actor.unknown_15e == 4 is transcribed literally; nothing in this function says
// what movement mode 4 means.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#include "units.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX -> v

// blam-cc: EAX -> actor_index, ECX -> destination, EDI -> out_avoidance_distance
uint8_t actor_movement_flying_needs_steering(datum_index actor_index, const real_point3d *destination,
                                             float *out_avoidance_distance)
{
    actor *self;
    object *unit_object;
    Vehicle *vehicle_definition;
    real_vector3d delta;
    float avoidance_distance;
    float length;
    float facing_dot;
    uint8_t needs_steering;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    avoidance_distance = 0.0f;
    needs_steering = 1;

    if (self->vehicle_driving_type == 4) {
        unit_object = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        vehicle_definition = (Vehicle *)tag_instances[unit_object->definition_tag & 0xffff].data;
        avoidance_distance = vehicle_definition->ai_avoidance_distance;
        if (avoidance_distance > 0.0f && self->avoidance_emergency > 0.9f) {
            delta.i = destination->x - self->body_position.x;
            delta.j = destination->y - self->body_position.y;
            delta.k = destination->z - self->body_position.z;
            length = vector3d_normalize_with_length(&delta);
            if (length > 0.0f) {
                facing_dot = delta.i * self->facing.i + delta.j * self->facing.j +
                             delta.k * self->facing.k;
                if (facing_dot > 0.984f) {
                    needs_steering = 0;
                }
            }
        }
    }

    if (out_avoidance_distance != (float *)0) {
        *out_avoidance_distance = avoidance_distance;
    }
    return needs_steering;
}

#if 0
Original Ghidra decompilation (0x41aab0):

undefined4 FUN_0041aab0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float10 fVar7;
  uint in_EAX;
  int iVar8;
  int iVar9;
  undefined2 extraout_var;
  float *in_ECX;
  undefined1 uVar11;
  float *unaff_EDI;
  float10 fVar12;
  float local_10;
  undefined2 uVar10;

  iVar8 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_10 = 0.0;
  uVar11 = 1;
  iVar9 = iVar8;
  if (*(short *)(iVar8 + 0x15e) == 4) {
    local_10 = *(float *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (*(uint *)(iVar8 + 0x158) & 0xffff) * 0xc) & 0xffff)
                                   * 0x20 + 0x14 + DAT_0087bc14) + 0x388);
    uVar10 = (undefined2)((uint)local_10 >> 0x10);
    iVar9 = CONCAT22(uVar10,(ushort)(local_10 < 0.0) << 8 | (ushort)NAN(local_10) << 10 |
                            (ushort)(local_10 == 0.0) << 0xe);
    if (local_10 < 0.0 == 0 && (local_10 == 0.0) == 0) {
      fVar1 = *(float *)(iVar8 + 0x5ec);
      iVar9 = CONCAT22(uVar10,(ushort)(fVar1 < 0.9) << 8 | (ushort)NAN(fVar1) << 10 |
                              (ushort)(fVar1 == 0.9) << 0xe);
      if (fVar1 < 0.9 == 0 && (fVar1 == 0.9) == 0) {
        fVar1 = *in_ECX;
        fVar2 = *(float *)(iVar8 + 300);
        fVar3 = in_ECX[1];
        fVar4 = *(float *)(iVar8 + 0x130);
        fVar5 = in_ECX[2];
        fVar6 = *(float *)(iVar8 + 0x134);
        fVar12 = (float10)vector3d_normalize_with_length();
        fVar7 = (float10)0.0;
        iVar9 = CONCAT22(extraout_var,
                         (ushort)(fVar12 < fVar7) << 8 | (ushort)(NAN(fVar12) || NAN(fVar7)) << 10 |
                         (ushort)(fVar12 == fVar7) << 0xe);
        if (fVar12 < fVar7 == 0 && (fVar12 == fVar7) == 0) {
          fVar1 = (fVar1 - fVar2) * *(float *)(iVar8 + 0x174) +
                  (fVar3 - fVar4) * *(float *)(iVar8 + 0x178) +
                  (fVar5 - fVar6) * *(float *)(iVar8 + 0x17c);
          iVar9 = CONCAT22(extraout_var,
                           (ushort)(fVar1 < 0.984) << 8 | (ushort)NAN(fVar1) << 10 |
                           (ushort)(fVar1 == 0.984) << 0xe);
          if (fVar1 < 0.984 == 0 && (fVar1 == 0.984) == 0) {
            uVar11 = 0;
          }
        }
      }
    }
  }
  if (unaff_EDI != (float *)0x0) {
    *unaff_EDI = local_10;
  }
  return CONCAT31((int3)((uint)iVar9 >> 8),uVar11);
}
#endif
