// actor_look_pick_random_point_in_cone  (Ghidra: actor_look_pick_random_point_in_cone, already named)
// address 0x415260, size 540 bytes
// name confidence: 0.55  rewrite confidence: 0.9 (VERIFIED against objdump 0x415260..0x41547b)
// evidence: phase-4 summary matches directly. Builds a "right" axis perpendicular to the
// caller-supplied base_direction (falling back to a global right-axis constant when that
// direction is near-vertical), then up to 10 times: picks a random yaw in [yaw_min,yaw_max]
// and pitch in [pitch_min,pitch_max], rotates a fresh copy of base_direction by pitch around
// the right axis and then by yaw around the global up axis, and (only when check_obstruction
// is set) rejects the result with a sight trace scaled 3x from origin before accepting it.
// The final direction is renormalized (with a near-zero-length epsilon check) and written to
// *out.
// register convention: reconstructed from objdump -d -M intel over 0x415260..0x41547b.
// origin/yaw_min/yaw_max/pitch_min/pitch_max/out are six genuine stack parameters (Ghidra
// found all six); base_direction is ESI and check_obstruction is BL, both implicit register
// arguments Ghidra rendered as unaff_ESI / unaff_BL.
// blam-cc: stack -> origin, stack -> yaw_min, stack -> yaw_max, stack -> pitch_min,
//   stack -> pitch_max, stack -> out, ESI -> base_direction, BL -> check_obstruction
// UNSURE: origin is only ever forwarded to collision_test_movement_segment (a trace/sight-check function outside
// this module, not rewritten here); its own signature is not established, so origin's exact
// type (a position, or an object/actor index this function's own logic never dereferences) is
// a guess from that one downstream use.
// UNSURE: the fallback global at 0x69671c is named global_right3d_pointer by analogy with the
// already-established global_forward3d_pointer (0x696718) / global_up3d_pointer (0x696720)
// pair it sits between; not independently confirmed.
// TYPES-GAP: the 80-byte trace-result buffer collision_test_movement_segment writes into is passed through
// opaquely (uint8_t[80]) since this function never reads it back; not added to types/ai.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern uint32_t random_seed_global; // 0x00719cd0
extern const real_vector3d *global_right3d_pointer; // 0x0069671c, UNSURE: see file header
extern const real_vector3d *global_up3d_pointer;    // 0x00696720

extern double cos(double x);  // FCOS
extern double sin(double x);  // FSIN
extern double sqrt(double x); // FSQRT

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX->v, ECX->axis
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object, void *scratch); // 0x505880, not this module, UNSURE signature

// blam-cc: stack x6, ESI -> base_direction, BL -> check_obstruction
// Returns whether a usable direction was found; on success writes it (unit length where
// possible) to *out.
uint8_t actor_look_pick_random_point_in_cone(void *origin, float yaw_min, float yaw_max, float pitch_min,
                                              float pitch_max, real_vector3d *base_direction,
                                              uint8_t check_obstruction, real_point3d *out)
{
    real_vector3d right_axis;
    real_vector3d direction;
    float yaw, pitch;
    float sin_a, cos_a;
    uint32_t rng;
    int16_t attempt;
    uint8_t trace_buffer[80];

    right_axis.i = -base_direction->j;
    right_axis.j = base_direction->i;
    right_axis.k = 0.0f;
    if (vector3d_normalize_with_length(&right_axis) == 0.0f) {
        right_axis = *global_right3d_pointer;
    }

    for (attempt = 0; attempt < 10; attempt++) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        rng = random_seed_global;
        yaw = (float)(rng >> 0x10) * 1.5259022e-05f * (yaw_max - yaw_min) + yaw_min;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        rng = random_seed_global;
        pitch = (float)(rng >> 0x10) * 1.5259022e-05f * (pitch_max - pitch_min) + pitch_min;

        direction = *base_direction;

        cos_a = (float)cos((double)pitch);
        sin_a = (float)sin((double)pitch);
        vector3d_rotate_about_axis(&direction, &right_axis, sin_a, cos_a);

        cos_a = (float)cos((double)yaw);
        sin_a = (float)sin((double)yaw);
        vector3d_rotate_about_axis(&direction, (real_vector3d *)global_up3d_pointer, sin_a, cos_a);

        if (!check_obstruction) {
            break;
        }

        {
            real_vector3d scaled;
            scaled.i = direction.i * 3.0f;
            scaled.j = direction.j * 3.0f;
            scaled.k = direction.k * 3.0f;
            // collision_test_movement_segment returns nonzero when the trace is obstructed; 0 (clear) accepts.
            if (!collision_test_movement_segment(0x21, origin, &scaled, (uint32_t)k_datum_index_none, trace_buffer)) {
                break;
            }
        }
    }
    if (check_obstruction && attempt >= 10) {
        return 0;
    }

    {
        double length = sqrt((double)(direction.k * direction.k + direction.j * direction.j + direction.i * direction.i));
        if (0.0001 <= ((length < 0.0) ? -length : length)) {
            float inv = 1.0f / (float)length;
            direction.i *= inv;
            direction.j *= inv;
            direction.k *= inv;
        }
        out->x = direction.i;
        out->y = direction.j;
        out->z = direction.k;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x415260):

undefined4
actor_look_pick_random_point_in_cone
          (undefined4 param_1,float param_2,float param_3,float param_4,float param_5,float *param_6
          )

{
  float fVar1;
  char cVar2;
  uint uVar3;
  char unaff_BL;
  short sVar4;
  float *unaff_ESI;
  float10 fVar5;
  float10 fVar6;
  float local_88;
  float local_84;
  float local_80;
  float local_5c;
  float local_58;
  float local_54;
  undefined1 local_50 [80];

  vector3d_normalize_with_length();
  sVar4 = 0;
  while( true ) {
    uVar3 = random_seed_global * 0x19660d + 0x3c6ef35f;
    random_seed_global = uVar3 * 0x19660d + 0x3c6ef35f;
    local_88 = *unaff_ESI;
    local_84 = unaff_ESI[1];
    local_80 = unaff_ESI[2];
    fVar1 = (float)(uVar3 >> 0x10) * 1.5259022e-05 * (param_3 - param_2) + param_2;
    fVar5 = (float10)(random_seed_global >> 0x10) * (float10)1.5259022e-05 *
            (float10)(param_5 - param_4) + (float10)param_4;
    fVar6 = (float10)fcos(fVar5);
    fVar5 = (float10)fsin(fVar5);
    vector3d_rotate_about_axis((float)fVar5,(float)fVar6);
    fVar5 = (float10)fcos((float10)fVar1);
    fVar6 = (float10)fsin((float10)fVar1);
    vector3d_rotate_about_axis((float)fVar6,(float)fVar5);
    if (unaff_BL == '\0') break;
    local_5c = local_88 * 3.0;
    local_58 = local_84 * 3.0;
    local_54 = local_80 * 3.0;
    cVar2 = FUN_00505880(0x21,param_1,&local_5c,0xffffffff,local_50);
    if (cVar2 == '\0') break;
    sVar4 = sVar4 + 1;
    if (9 < sVar4) {
      return 0;
    }
  }
  fVar1 = SQRT(local_88 * local_88 + local_84 * local_84 + local_80 * local_80);
  if (0.0001 <= ABS(fVar1)) {
    fVar1 = 1.0 / fVar1;
    local_88 = local_88 * fVar1;
    local_84 = local_84 * fVar1;
    local_80 = local_80 * fVar1;
  }
  *param_6 = local_88;
  param_6[1] = local_84;
  param_6[2] = local_80;
  return 1;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe, 0x415260..0x41547b) resolved:
  - the leading normalize (0x415281) operates on a LOCAL {-esi.j, esi.i, 0} vector (the right
    axis), not on *esi itself -- esi's own components are read raw, matching a caller
    contract that base_direction already arrives unit length (as 0x414d00 and 0x414f50 both
    ensure).
  - the fallback at 0x415295 loads 3 dwords from ds:0x69671c when the right-axis length
    compares equal to 0x672ac0 (0.0f).
  - edi is loaded once from ds:0x696720 (global_up3d_pointer) and reused unchanged as the ECX
    axis argument for the second (yaw) rotate call; eax/ecx for the two
    vector3d_rotate_about_axis calls point at a stack-local copy of esi's vector (v, rotated
    in place both times) and at the right-axis / global_up3d_pointer respectively (axis),
    matching src/math/vector3d_rotate_about_axis.c's EAX->v, ECX->axis convention exactly.
  - unaff_BL is read directly (test bl,bl) with no push/pop of ebx in this function, and its
    two known callers set it explicitly right before the call (0x414d00: mov bl,1; 0x414f50:
    xor bl,bl), confirming it is a genuine caller-supplied flag, not a spilled local.
#endif
