// vehicle_calculate_animation_controls  (Ghidra: unit_calculate_animation_controls; per
//   out/phase4/units_types_notes.md this is the vehicle row's +0x38 column, so it only ever
//   reads vehicle_data, and the "unit_" prefix in its pre-existing name is misleading -- renamed
//   accordingly, per the task's rule that only still-FUN_xxxxxx names get chosen fresh)
// address 0x5756f0, size 1217 bytes
// name confidence: 0.55 (units_types_notes.md's identification, +0.0 for the name itself which
//   was already wrong)
// rewrite confidence: 0.85 -- four of the "vector3d_length()"/"vector3d_project_onto_unit_axis()"
//   calls (cases 0xe, 0x10, 0x20, 0x24) have no visible argument at all; object.velocity is used
//   as the best-available guess and flagged UNSURE at each site.
// evidence: types/units.h vehicle_data.forward_velocity/.sideways_velocity/.turning_velocity
//   (0x4d4/0x4d8/0x4dc), .wheel_rotation/.left_wheel_rotation/.right_wheel_rotation
//   (0x4e0/0x4e4/0x4e8), .ground_lean/.ground_contact_fraction (0x4ec/0x4f0), .flags (0x4cc,
//   bits 4 and 8), .airborne_ticks (0x4d0); types/objects.h object.flags (0x010),
//   object.velocity/forward/up (0x068/0x074/0x080); types/tags.h Vehicle.maximum_forward_speed/
//   .maximum_reverse_speed (0x2f8/0x2fc), .maximum_left_turn/.maximum_right_turn (0x308/0x30c),
//   .wheel_circumference (0x310), .maximum_left_slide/.maximum_right_slide (0x330/0x334),
//   .vehicle_a_in/.vehicle_b_in/.vehicle_c_in/.vehicle_d_in (0x31c..0x322, the four
//   ObjectFunctionIn selectors this function evaluates).
// UNSURE: the output array (object+0x124, four floats) has no name in types/objects.h, which
//   documents that range only as undocumented _pad_110 padding; kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_length(real_vector3d *v); // 0x401960, UNSURE args at every call site here
extern void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out); // 0x4cda30, EAX parallel, ECX axis, EDX v, ESI perp //  // real signature (vector3d_project_onto_unit_axis.c): void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out); Ghidra recovered 0 of 4 args at this call site
extern float fabsf(float x);

// Evaluates the four ObjectFunctionIn selectors on the Vehicle tag (vehicle_a_in..d_in) against
// a table of physics-derived control values (speed, turn rate, vertical motion, etc., each
// normalized to 0..1) and writes the results into the object's function-output array
// (object+0x124), used to drive the unit's procedural animation blending.
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
// REWRITTEN 2026-09-28 from objdump 0x5756f0..0x575bb0 (jump table 0x575bb4, 36 entries). The draft stored
//   selectors 0x20 (lateral slide), 0x21 (+0x4ec) and 0x22 (+0x4f0) unclamped; the original clamps them to
//   [0, 1] like every computed value (only 0xb/0xc/0xf/0x10 early-outs and the default store unclamped).
//   The vector3d_length arguments are the velocity (+0x68, EAX) -- not UNSURE any more. Dot products keep
//   the original k, j, i summation order.
// blam-cc: stack -> unit_index
void vehicle_calculate_animation_controls(uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[*(uint32_t *)obj & 0xffff].data;
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *up = (real_vector3d *)(obj + 0x80);
    float forward_velocity = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    float sideways_velocity = ((struct vehicle_object *)obj)->vehicle.sideways_velocity;
    float turning_velocity = ((struct vehicle_object *)obj)->vehicle.turning_velocity;
    float max_forward = fabsf(tag->maximum_forward_speed);     // esp+0x14
    float max_reverse = fabsf(tag->maximum_reverse_speed);     // esp+0x18
    float max_speed = (max_forward > max_reverse) ? max_forward : max_reverse;           // esp+0x10
    float max_left_slide = fabsf(tag->maximum_left_slide);     // esp+0x1c
    float max_right_slide = fabsf(tag->maximum_right_slide);   // esp+0x20
    float max_slide = (max_left_slide > max_right_slide) ? max_left_slide : max_right_slide; // esp+0x24
    float max_left_turn = fabsf(tag->maximum_left_turn);       // esp+0x28
    float max_right_turn = fabsf(tag->maximum_right_turn);     // esp+0x2c
    float max_turn = (max_left_turn > max_right_turn) ? max_left_turn : max_right_turn;     // esp+0x38
    int16_t *selectors = (int16_t *)((uint8_t *)tag + 0x31c);
    float *outputs = (float *)(obj + 0x124);
    int i;

    for (i = 0; i < 4; i++) {
        float value;

        if (selectors[i] == 0) {
            continue; // 0x5757fa: the output is left untouched
        }
        switch (selectors[i]) {
        case 1: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            value = fabsf(forward_velocity) / max_speed;
            break;
        case 2: // 0x57582d: test ah,5 / jp -- not below zero (or NaN) divides the velocity, else 0 / max
            value = !(forward_velocity < 0.0f) ? forward_velocity / max_forward : 0.0f / max_forward;
            break;
        case 3: // 0x575860: test ah,0x41 / jne -- at most zero (or NaN) divides |velocity|, else |0| / max
            value = (forward_velocity > 0.0f) ? 0.0f / max_reverse : fabsf(forward_velocity) / max_reverse;
            break;
        case 4:
            value = fabsf(sideways_velocity) / max_slide;
            break;
        case 5:
            value = fabsf(sideways_velocity) / max_left_slide;
            break;
        case 6:
            value = fabsf(sideways_velocity) / max_right_slide;
            break;
        case 7: {
            float a = fabsf(forward_velocity) / max_speed;
            float b = fabsf(sideways_velocity) / max_slide;

            value = (a > b) ? a : b;
            break;
        }
        case 8:
            value = fabsf(turning_velocity) / max_turn;
            break;
        case 9:
            value = fabsf(turning_velocity) / max_left_turn;
            break;
        case 10:
            value = fabsf(turning_velocity) / max_right_turn;
            break;
        case 0xb: // stored unclamped
            outputs[i] = (obj[0x4cc] & 4) ? 1.0f : 0.0f;
            continue;
        case 0xc:
            outputs[i] = (obj[0x4cc] & 8) ? 1.0f : 0.0f;
            continue;
        case 0xe:
            value = vector3d_length(velocity) / max_speed;
            break;
        case 0xf:
            if ((obj[0x10] & 0x1c) == 0) {
                outputs[i] = 0.0f;
                continue;
            }
            value = vector3d_length(velocity) / max_speed;
            break;
        case 0x10:
            if ((obj[0x10] & 2) == 0) {
                outputs[i] = 0.0f;
                continue;
            }
            value = vector3d_length(velocity) / max_speed;
            break;
        case 0x11: // 0x5759a9: summed k, j, i
            value = fabsf(velocity->k * forward->k + velocity->j * forward->j + velocity->i * forward->i) / max_speed;
            break;
        case 0x12: case 0x13:
            value = fabsf(up->k * velocity->k + up->j * velocity->j + up->i * velocity->i) / max_speed;
            break;
        case 0x14:
            value = ((struct vehicle_object *)obj)->vehicle.left_wheel_rotation / tag->wheel_circumference;
            break;
        case 0x15:
            value = ((struct vehicle_object *)obj)->vehicle.right_wheel_rotation / tag->wheel_circumference;
            break;
        case 0x16:
            value = fabsf(forward_velocity - turning_velocity) / max_speed;
            break;
        case 0x17:
            value = fabsf(turning_velocity + forward_velocity) / max_speed;
            break;
        case 0x18: case 0x19: case 0x1a: case 0x1b:
            value = ((struct vehicle_object *)obj)->vehicle.wheel_rotation / tag->wheel_circumference;
            break;
        case 0x20: {
            real_vector3d parallel;      // esp+0x54
            real_vector3d perpendicular; // esp+0x48
            float slide;

            // 0x575a63: the velocity split along the forward axis; the perpendicular part's length, x 10/3, squared
            vector3d_project_onto_unit_axis(&parallel, forward, velocity, &perpendicular);
            slide = vector3d_length(&perpendicular) * 3.3333333f;
            value = slide * slide;
            break;
        }
        case 0x21:
            value = ((struct vehicle_object *)obj)->vehicle.ground_lean;
            break;
        case 0x22:
            value = ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction;
            break;
        case 0x23: {
            float lean = fabsf(forward->k * velocity->k + forward->j * velocity->j + forward->i * velocity->i) / max_speed;
            float speed = fabsf(forward_velocity) / max_forward;
            float blend = ((float)obj[0x4d0] * 0.2f + 1.0f) * 0.5f;

            if (blend < 0.0f) {
                blend = 0.0f;
            } else if (blend > 1.0f) {
                blend = 1.0f;
            }
            value = lean * (1.0f - blend) + blend * speed;
            break;
        }
        case 0x24:
            value = ((vector3d_length(velocity) / tag->maximum_forward_speed) * ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction - 0.05f) *
                1.1764706f;
            break;
        default: // 0xd and anything past 0x24: 0, unclamped
            outputs[i] = 0.0f;
            continue;
        }

        // 0x575b53: every computed value is clamped to [0, 1] (NaN passes through)
        if (value < 0.0f) {
            value = 0.0f;
        } else if (value > 1.0f) {
            value = 1.0f;
        }
        outputs[i] = value;
    }
}

#if 0
Original Ghidra decompilation (0x5756f0):

void unit_calculate_animation_controls(uint param_1)

{
  uint *puVar1;
  int iVar2;
  byte bVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float10 fVar10;
  float local_58;
  float local_44;
  short *local_38;
  float *local_34;
  float local_30;
  int local_2c;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  fVar4 = ABS(*(float *)(iVar2 + 0x2f8));
  fVar5 = ABS(*(float *)(iVar2 + 0x2fc));
  local_58 = fVar5;
  if (fVar5 < fVar4) {
    local_58 = fVar4;
  }
  fVar6 = ABS(*(float *)(iVar2 + 0x330));
  fVar7 = ABS(*(float *)(iVar2 + 0x334));
  local_44 = fVar7;
  if (fVar7 < fVar6) {
    local_44 = fVar6;
  }
  fVar8 = ABS(*(float *)(iVar2 + 0x308));
  fVar9 = ABS(*(float *)(iVar2 + 0x30c));
  local_30 = fVar9;
  if (fVar9 < fVar8) {
    local_30 = fVar8;
  }
  local_34 = (float *)(puVar1 + 0x49);
  local_38 = (short *)(iVar2 + 0x31c);
  local_2c = 4;
  do {
    if (*local_38 == 0) goto LAB_00575b85;
    fVar10 = (float10)0.0;
    switch(*local_38) {
    case 1:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
      fVar10 = ABS((float10)(float)puVar1[0x135]) / (float10)local_58;
      break;
    case 2:
      if (0.0 <= (float)puVar1[0x135]) {
        fVar10 = (float10)(float)puVar1[0x135] / (float10)fVar4;
      }
      else {
        fVar10 = (float10)0.0 / (float10)fVar4;
      }
      break;
    case 3:
      if ((float)puVar1[0x135] <= 0.0) {
        fVar10 = ABS((float10)(float)puVar1[0x135]) / (float10)fVar5;
      }
      else {
        fVar10 = ABS((float10)0.0) / (float10)fVar5;
      }
      break;
    case 4:
      fVar10 = ABS((float10)(float)puVar1[0x136]) / (float10)local_44;
      break;
    case 5:
      fVar10 = ABS((float10)(float)puVar1[0x136]) / (float10)fVar6;
      break;
    case 6:
      fVar10 = ABS((float10)(float)puVar1[0x136]) / (float10)fVar7;
      break;
    case 7:
      fVar10 = ABS((float10)(float)puVar1[0x135]) / (float10)local_58;
      if (fVar10 <= (float10)(ABS((float)puVar1[0x136]) / local_44)) {
        fVar10 = (float10)(ABS((float)puVar1[0x136]) / local_44);
      }
      break;
    case 8:
      fVar10 = ABS((float10)(float)puVar1[0x137]) / (float10)local_30;
      break;
    case 9:
      fVar10 = ABS((float10)(float)puVar1[0x137]) / (float10)fVar8;
      break;
    case 10:
      fVar10 = ABS((float10)(float)puVar1[0x137]) / (float10)fVar9;
      break;
    case 0xb:
      if ((puVar1[0x133] & 4) == 0) goto LAB_00575951;
      goto LAB_00575b79;
    case 0xc:
      if ((puVar1[0x133] & 8) != 0) goto LAB_00575b79;
      fVar10 = (float10)0.0;
    default:
      goto switchD_00575813_caseD_d;
    case 0xe:
      fVar10 = (float10)vector3d_length();
      fVar10 = fVar10 / (float10)local_58;
      break;
    case 0xf:
      bVar3 = (byte)puVar1[4] & 0x1c;
      goto LAB_00575994;
    case 0x10:
      bVar3 = (byte)puVar1[4] & 2;
LAB_00575994:
      if (bVar3 != 0) {
        fVar10 = (float10)vector3d_length();
        fVar10 = fVar10 / (float10)local_58;
        break;
      }
LAB_00575951:
      fVar10 = (float10)0.0;
      goto switchD_00575813_caseD_d;
    case 0x11:
      fVar10 = ABS((float10)(float)puVar1[0x1a] * (float10)(float)puVar1[0x1d] +
                   (float10)(float)puVar1[0x1b] * (float10)(float)puVar1[0x1e] +
                   (float10)(float)puVar1[0x1c] * (float10)(float)puVar1[0x1f]) / (float10)local_58;
      break;
    case 0x12:
    case 0x13:
      fVar10 = ABS((float10)(float)puVar1[0x20] * (float10)(float)puVar1[0x1a] +
                   (float10)(float)puVar1[0x21] * (float10)(float)puVar1[0x1b] +
                   (float10)(float)puVar1[0x22] * (float10)(float)puVar1[0x1c]) / (float10)local_58;
      break;
    case 0x14:
      fVar10 = (float10)(float)puVar1[0x139] / (float10)*(float *)(iVar2 + 0x310);
      break;
    case 0x15:
      fVar10 = (float10)(float)puVar1[0x13a] / (float10)*(float *)(iVar2 + 0x310);
      break;
    case 0x16:
      fVar10 = ABS((float10)(float)puVar1[0x135] - (float10)(float)puVar1[0x137]) /
               (float10)local_58;
      break;
    case 0x17:
      fVar10 = ABS((float10)(float)puVar1[0x137] + (float10)(float)puVar1[0x135]) /
               (float10)local_58;
      break;
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
      fVar10 = (float10)(float)puVar1[0x138] / (float10)*(float *)(iVar2 + 0x310);
      break;
    case 0x20:
      vector3d_project_onto_unit_axis();
      fVar10 = (float10)vector3d_length();
      fVar10 = fVar10 * (float10)3.3333333 * fVar10 * (float10)3.3333333;
      break;
    case 0x21:
      fVar10 = (float10)(float)puVar1[0x13b];
      break;
    case 0x22:
      fVar10 = (float10)(float)puVar1[0x13c];
      break;
    case 0x23:
      fVar10 = ((float10)(byte)puVar1[0x134] * (float10)0.2 + (float10)1.0) * (float10)0.5;
      if ((float10)0.0 <= fVar10) {
        if ((float10)1.0 < fVar10) {
          fVar10 = (float10)1.0;
        }
      }
      else {
        fVar10 = (float10)0.0;
      }
      fVar10 = fVar10 * (ABS((float10)(float)puVar1[0x135]) / (float10)fVar4) +
               ((float10)1.0 - fVar10) *
               (ABS((float10)(float)puVar1[0x1d] * (float10)(float)puVar1[0x1a] +
                    (float10)(float)puVar1[0x1e] * (float10)(float)puVar1[0x1b] +
                    (float10)(float)puVar1[0x1f] * (float10)(float)puVar1[0x1c]) / (float10)local_58
               );
      break;
    case 0x24:
      fVar10 = (float10)vector3d_length();
      fVar10 = ((fVar10 / (float10)*(float *)(iVar2 + 0x2f8)) * (float10)(float)puVar1[0x13c] -
               (float10)0.05) * (float10)1.1764706;
    }
    if ((float10)0.0 <= fVar10) {
      if ((float10)1.0 < fVar10) {
LAB_00575b79:
        fVar10 = (float10)1.0;
      }
    }
    else {
      fVar10 = (float10)0.0;
    }
switchD_00575813_caseD_d:
    *local_34 = (float)fVar10;
LAB_00575b85:
    local_38 = local_38 + 1;
    local_34 = local_34 + 1;
    local_2c = local_2c + -1;
    if (local_2c == 0) {
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
