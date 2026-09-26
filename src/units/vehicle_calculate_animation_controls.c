// vehicle_calculate_animation_controls  (Ghidra: unit_calculate_animation_controls; per
//   out/phase4/units_types_notes.md this is the vehicle row's +0x38 column, so it only ever
//   reads vehicle_data, and the "unit_" prefix in its pre-existing name is misleading -- renamed
//   accordingly, per the task's rule that only still-FUN_xxxxxx names get chosen fresh)
// address 0x5756f0, size 1217 bytes
// name confidence: 0.55 (units_types_notes.md's identification, +0.0 for the name itself which
//   was already wrong)
// rewrite confidence: 0.2 -- four of the "vector3d_length()"/"vector3d_project_onto_unit_axis()"
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

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_length(real_vector3d *v); // 0x401960, UNSURE args at every call site here
extern void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out); // 0x4cda30, EAX parallel, ECX axis, EDX v, ESI perp //  // real signature (vector3d_project_onto_unit_axis.c): void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out); Ghidra recovered 0 of 4 args at this call site
extern float fabsf(float x);

static float clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Evaluates the four ObjectFunctionIn selectors on the Vehicle tag (vehicle_a_in..d_in) against
// a table of physics-derived control values (speed, turn rate, vertical motion, etc., each
// normalized to 0..1) and writes the results into the object's function-output array
// (object+0x124), used to drive the unit's procedural animation blending.
// FIXED (register inputs, objdump + difftest): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> unit_index
void vehicle_calculate_animation_controls(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    float max_forward = fabsf(tag->maximum_forward_speed);
    float max_reverse = fabsf(tag->maximum_reverse_speed);
    float max_speed = (max_reverse < max_forward) ? max_forward : max_reverse;
    float max_left_slide = fabsf(tag->maximum_left_slide);
    float max_right_slide = fabsf(tag->maximum_right_slide);
    float max_slide = (max_right_slide < max_left_slide) ? max_left_slide : max_right_slide;
    float max_left_turn = fabsf(tag->maximum_left_turn);
    float max_right_turn = fabsf(tag->maximum_right_turn);
    float max_turn = (max_right_turn < max_left_turn) ? max_left_turn : max_right_turn;
    int16_t *selectors = (int16_t *)((uint8_t *)tag + 0x31c);
    float *outputs = (float *)((uint8_t *)obj + 0x124);
    int i;

    for (i = 0; i < 4; i++) {
        float value = 0.0f;

        switch (selectors[i]) {
        case 0: goto next; // selector 0 means "unused", leaves outputs[i] untouched
        case 1: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            value = fabsf(vehicle->forward_velocity) / max_speed;
            break;
        case 2:
            value = (vehicle->forward_velocity >= 0.0f) ? vehicle->forward_velocity / max_forward : 0.0f;
            break;
        case 3:
            value = (vehicle->forward_velocity <= 0.0f) ? fabsf(vehicle->forward_velocity) / max_reverse : 0.0f;
            break;
        case 4:
            value = fabsf(vehicle->sideways_velocity) / max_slide;
            break;
        case 5:
            value = fabsf(vehicle->sideways_velocity) / max_left_slide;
            break;
        case 6:
            value = fabsf(vehicle->sideways_velocity) / max_right_slide;
            break;
        case 7: {
            float a = fabsf(vehicle->forward_velocity) / max_speed;
            float b = fabsf(vehicle->sideways_velocity) / max_slide;
            value = (a > b) ? a : b;
            break;
        }
        case 8:
            value = fabsf(vehicle->turning_velocity) / max_turn;
            break;
        case 9:
            value = fabsf(vehicle->turning_velocity) / max_left_turn;
            break;
        case 10:
            value = fabsf(vehicle->turning_velocity) / max_right_turn;
            break;
        case 0xb:
            if ((vehicle->flags & 4) == 0) {
                value = 0.0f;
                goto store;
            }
            value = 1.0f;
            goto store;
        case 0xc:
            if ((vehicle->flags & 8) != 0) {
                value = 1.0f;
                goto store;
            }
            value = 0.0f;
            break;
        case 0xe:
            value = vector3d_length(&obj->velocity) / max_speed; // UNSURE argument
            break;
        case 0xf:
            if (((uint8_t)obj->flags & 0x1c) == 0) { value = 0.0f; break; }
            value = vector3d_length(&obj->velocity) / max_speed; // UNSURE argument
            break;
        case 0x10:
            if (((uint8_t)obj->flags & 2) == 0) { value = 0.0f; break; }
            value = vector3d_length(&obj->velocity) / max_speed; // UNSURE argument
            break;
        case 0x11:
            value = fabsf(obj->velocity.i * obj->forward.i + obj->velocity.j * obj->forward.j +
                          obj->velocity.k * obj->forward.k) / max_speed;
            break;
        case 0x12: case 0x13:
            value = fabsf(obj->up.i * obj->velocity.i + obj->up.j * obj->velocity.j +
                          obj->up.k * obj->velocity.k) / max_speed;
            break;
        case 0x14:
            value = vehicle->left_wheel_rotation / tag->wheel_circumference;
            break;
        case 0x15:
            value = vehicle->right_wheel_rotation / tag->wheel_circumference;
            break;
        case 0x16:
            value = fabsf(vehicle->forward_velocity - vehicle->turning_velocity) / max_speed;
            break;
        case 0x17:
            value = fabsf(vehicle->turning_velocity + vehicle->forward_velocity) / max_speed;
            break;
        case 0x18: case 0x19: case 0x1a: case 0x1b:
            value = vehicle->wheel_rotation / tag->wheel_circumference;
            break;
        case 0x20: {
            real fraction;
            real_vector3d parallel, perpendicular;
            // 0x575a63: the velocity (+0x68) split along the forward axis (+0x74); the length of the part
            // perpendicular to it (ESI) is what gets scaled
            vector3d_project_onto_unit_axis(&parallel, (real_vector3d *)((uint8_t *)obj + 0x74),
                                            (real_vector3d *)((uint8_t *)obj + 0x68), &perpendicular);
            fraction = vector3d_length(&perpendicular);
            value = fraction * 3.3333333f * fraction * 3.3333333f;
            goto store;
        }
        case 0x21:
            value = vehicle->ground_lean;
            goto store;
        case 0x22:
            value = vehicle->ground_contact_fraction;
            goto store;
        case 0x23: {
            float blend = clamp01(((float)vehicle->airborne_ticks * 0.2f + 1.0f) * 0.5f);
            float speed_term = fabsf(vehicle->forward_velocity) / max_forward;
            float lean_term = fabsf(obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                                    obj->forward.k * obj->velocity.k) / max_speed;
            value = blend * speed_term + (1.0f - blend) * lean_term;
            break;
        }
        case 0x24: {
            real speed = vector3d_length(&obj->velocity); // UNSURE argument
            value = ((speed / tag->maximum_forward_speed) * vehicle->ground_contact_fraction - 0.05f) * 1.1764706f;
            break;
        }
        default:
            goto store;
        }

        value = clamp01(value);
    store:
        outputs[i] = value;
    next:;
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
