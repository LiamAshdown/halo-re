// unit_update_up_vector  (Ghidra: unit_update_up_vector)
// address 0x560800, size 1136 bytes
// name confidence: 0.45 (phase2 candidate)   rewrite confidence: 0.85 (step 1: rewritten from
//   objdump -d 0x560800..0x560c6f with every helper operand read off the call sites)
// evidence: types/objects.h object.forward/up (0x74/0x80); types/units.h biped_data
//   .ground_surface_index/.ground_normal/.unknown_510 (0x4d8/0x514/0x510), biped_data.flags bit 0
//   (airborne -- the solver's result bit 0); Biped tag flags 0x2f4 ("flying" 0x4, "can climb any
//   surface" 0x40); object.vitality_flags health-frozen bit (0x106 & 4).
// register convention: the Biped tag data in EAX, the object pointer in ECX.
//   // blam-cc: EAX -> biped_tag, ECX -> obj
// Four cases:
//   flying (alive): up = the facing frame's up rolled by biped.unknown_510;
//   climbs any surface (alive): turn up toward the ground normal (or keep it when there is no
//     ground surface), at most 10 degrees past a flip, and rebuild forward from it;
//   otherwise when grounded: rotate up and forward together onto the ground normal;
//   otherwise (airborne, or alive without either flag): flatten forward, up = world up.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern real_vector3d *global_forward3d_pointer; // 0x00696718 (1, 0, 0)
extern real_vector3d *global_left3d_pointer;    // 0x0069671c (0, 1, 0)
extern real_vector3d *global_up3d_pointer;      // 0x00696720 (0, 0, 1)

extern double cos(double x);
extern double sin(double x);
extern double acos(double x); // 0x628140 is the CRT's x87 _CIacos (atan2(sqrt((1+x)(1-x)), x))
extern double fabs(double x);
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX v
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, blam-cc: EAX out, ECX a, stack b -- computes b x a
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
    // 0x4cd820, blam-cc: EAX v, ECX axis, stack (sin, cos)

static void level_to_world_up(object *obj)
{
    // 0x560c18
    obj->forward.k = 0.0f;
    if (vector3d_normalize_with_length(&obj->forward) == 0.0f) {
        obj->forward = *global_forward3d_pointer;
    }
    obj->up = *global_up3d_pointer;
}

// blam-cc: EAX -> biped_tag, ECX -> obj
void unit_update_up_vector(Biped *biped_tag, object *obj)
{
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    uint32_t tag_flags = biped_tag->biped_flags;
    uint8_t frozen = (obj->vitality_flags & _object_health_frozen_bit) != 0;

    if ((tag_flags & 4) != 0 && !frozen) {
        // flying (0x560823): side = forward x world up, up0 = side x forward
        real_vector3d up0;                           // [esp+0x10]
        real_vector3d side;                          // [esp+0x1c]
        float c, s;

        vector3d_cross_product(&side, global_up3d_pointer, &obj->forward);
        vector3d_cross_product(&up0, &obj->forward, &side);
        if (vector3d_normalize_with_length(&up0) == 0.0f) {
            up0 = *global_forward3d_pointer;
            side = *global_left3d_pointer;
        }
        c = (float)cos((double)biped->bank_angle);
        s = (float)sin((double)biped->bank_angle);
        up0.i *= c;
        up0.j *= c;
        up0.k *= c;
        vector3d_normalize_with_length(&side);
        obj->up.i = side.i * s + up0.i;
        obj->up.j = side.j * s + up0.j;
        obj->up.k = side.k * s + up0.k;
        return;
    }

    if ((tag_flags & 0x40) != 0 && !frozen) {
        // climbs any surface (0x560926)
        real_vector3d target;                        // [esp+0x18] (4 pushes)
        real_vector3d cross1;                        // [esp+0x3c]
        real_vector3d frame;                         // [esp+0x30]

        if (biped->ground_surface_index == 0xffffffff) {
            target = obj->up;
        } else {
            real_vector3d axis;                      // [esp+0x24]
            real_vector3d turned;                    // [esp+0x30]
            real_vector3d check;                     // [esp+0x3c]
            uint8_t use_target = 0;

            target = biped->ground_normal;
            vector3d_cross_product(&axis, &target, &obj->up);          // up x target
            if (vector3d_normalize_with_length(&axis) == 0.0f) {
                if (target.j * obj->up.j + target.k * obj->up.k + target.i * obj->up.i > 0.0f) {
                    use_target = 1;                                     // already aligned
                } else {
                    axis = obj->forward;                                // flipped: turn about forward
                }
            }
            if (!use_target) {
                float c = (float)cos(0.1745329201221466);             // 0x672f18, 10 degrees
                float s = (float)sin(0.1745329201221466);
                turned = obj->up;
                vector3d_rotate_about_axis(&turned, &axis, s, c);
                vector3d_cross_product(&check, &target, &turned);       // turned x target
                if (check.k * axis.k + check.j * axis.j + check.i * axis.i > 0.0f) {
                    target = turned;                                    // not past the target yet
                }
            }
        }

        // 0x560a59: forward = target x (forward x target), falling back to target x (target x up)
        vector3d_cross_product(&cross1, &target, &obj->forward);
        vector3d_cross_product(&frame, &cross1, &target);
        if (vector3d_normalize_with_length(&frame) == 0.0f) {
            vector3d_cross_product(&cross1, &obj->up, &target);
            vector3d_cross_product(&frame, &cross1, &target);
            if (vector3d_normalize_with_length(&frame) == 0.0f) {
                target = *global_up3d_pointer;
                frame = *global_forward3d_pointer;
            }
        }
        obj->up = target;
        obj->forward = frame;
        return;
    }

    if (!frozen) {
        // alive without flying / climbing: 0x560b1c test cl,al -> not frozen -> level
        level_to_world_up(obj);
        return;
    }

    // frozen (0x560b24): align to the ground when grounded, else level
    if ((biped->flags & 1) != 0) {
        level_to_world_up(obj);
        return;
    }
    {
        real_vector3d *normal = &biped->ground_normal;
        float dot = obj->up.k * normal->k + obj->up.j * normal->j + obj->up.i * normal->i;
        float angle;
        real_vector3d axis;                          // [esp+0x38]
        float c, s;

        if ((float)fabs((double)(dot - 1.0f)) < 0.0001f) { // 0x560b6e jnp (double compare)
            return;
        }
        angle = (float)acos((double)dot);
        if (angle == 0.0f) {
            return;
        }
        vector3d_cross_product(&axis, normal, &obj->up);                // up x normal
        if (vector3d_normalize_with_length(&axis) == 0.0f) {
            return;
        }
        c = (float)cos((double)angle);
        s = (float)sin((double)angle);
        vector3d_rotate_about_axis(&obj->up, &axis, s, c);
        vector3d_rotate_about_axis(&obj->forward, &axis, s, c);
        vector3d_normalize_with_length(&obj->up);
        vector3d_normalize_with_length(&obj->forward);
    }
}

#if 0
Original Ghidra decompilation (0x560800):

void FUN_00560800(void)

{
  float *pfVar1;
  undefined *puVar2;
  int in_EAX;
  undefined4 uVar3;
  int in_ECX;
  float fVar4;
  undefined4 extraout_EDX;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float10 fVar9;
  float10 fVar10;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if (((*(uint *)(in_EAX + 0x2f4) & 4) != 0) && ((*(byte *)(in_ECX + 0x106) & 4) == 0)) {
    vector3d_cross_product(in_ECX + 0x74);
    vector3d_cross_product(&local_24);
    fVar9 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar9) {
      local_30 = *(float *)PTR_DAT_00696718;
      local_2c = *(float *)(PTR_DAT_00696718 + 4);
      local_28 = *(float *)(PTR_DAT_00696718 + 8);
      local_24 = *(float *)PTR_DAT_0069671c;
      local_20 = *(float *)(PTR_DAT_0069671c + 4);
      local_1c = *(float *)(PTR_DAT_0069671c + 8);
    }
    fVar9 = (float10)fcos((float10)*(float *)(in_ECX + 0x510));
    fVar10 = (float10)fsin((float10)*(float *)(in_ECX + 0x510));
    fVar6 = (float)fVar10;
    local_30 = (float)((float10)local_30 * fVar9);
    local_2c = (float)((float10)local_2c * fVar9);
    local_28 = (float)((float10)local_28 * fVar9);
    vector3d_normalize_with_length();
    *(float *)(in_ECX + 0x80) = local_24 * fVar6 + local_30;
    *(float *)(in_ECX + 0x84) = local_20 * fVar6 + local_2c;
    *(float *)(in_ECX + 0x88) = local_1c * fVar6 + local_28;
    return;
  }
  if ((*(uint *)(in_EAX + 0x2f4) & 0x40) == 0) {
    if ((*(byte *)(in_ECX + 0x106) & 4) == 0) goto LAB_00560c18;
  }
  else if ((*(byte *)(in_ECX + 0x106) & 4) == 0) {
    pfVar1 = (float *)(in_ECX + 0x80);
    if (*(int *)(in_ECX + 0x4d8) == -1) {
      fVar4 = *(float *)(in_ECX + 0x88);
      fVar5 = *pfVar1;
      fVar7 = *(float *)(in_ECX + 0x84);
    }
    else {
      fVar6 = *(float *)(in_ECX + 0x514);
      fVar8 = *(float *)(in_ECX + 0x518);
      local_28 = *(float *)(in_ECX + 0x51c);
      local_30 = fVar6;
      local_2c = fVar8;
      vector3d_cross_product(pfVar1);
      fVar9 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar9) {
        if (0.0 < local_30 * *pfVar1 +
                  local_28 * *(float *)(in_ECX + 0x88) + local_2c * *(float *)(in_ECX + 0x84))
        goto LAB_00560a59;
        local_24 = *(float *)(in_ECX + 0x74);
        local_20 = *(float *)(in_ECX + 0x78);
        local_1c = *(float *)(in_ECX + 0x7c);
      }
      fVar9 = (float10)fcos((float10)0.1745329201221466);
      local_18 = *pfVar1;
      local_14 = *(float *)(in_ECX + 0x84);
      local_10 = *(float *)(in_ECX + 0x88);
      fVar10 = (float10)fsin((float10)0.1745329201221466);
      uVar3 = vector3d_rotate_about_axis((float)fVar10,(float)fVar9);
      vector3d_cross_product(uVar3);
      fVar4 = local_10;
      fVar5 = local_18;
      fVar7 = local_14;
      if (local_c * local_24 + local_8 * local_20 + local_4 * local_1c <= 0.0) goto LAB_00560a59;
    }
    fVar6 = fVar5;
    fVar8 = fVar7;
    local_30 = fVar5;
    local_2c = fVar7;
    local_28 = fVar4;
LAB_00560a59:
    vector3d_cross_product((float *)(in_ECX + 0x74));
    vector3d_cross_product(&local_30);
    fVar9 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar9) {
      vector3d_cross_product(&local_30);
      vector3d_cross_product(&local_30);
      fVar9 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar9) {
        fVar6 = *(float *)PTR_DAT_00696720;
        fVar8 = *(float *)(PTR_DAT_00696720 + 4);
        local_28 = *(float *)(PTR_DAT_00696720 + 8);
        local_18 = *(float *)PTR_DAT_00696718;
        local_14 = *(float *)(PTR_DAT_00696718 + 4);
        local_10 = *(float *)(PTR_DAT_00696718 + 8);
      }
    }
    *pfVar1 = fVar6;
    *(float *)(in_ECX + 0x84) = fVar8;
    *(float *)(in_ECX + 0x74) = local_18;
    *(float *)(in_ECX + 0x78) = local_14;
    *(float *)(in_ECX + 0x88) = local_28;
    *(float *)(in_ECX + 0x7c) = local_10;
    return;
  }
  if ((*(byte *)(in_ECX + 0x4cc) & 1) == 0) {
    if (ABS((*(float *)(in_ECX + 0x80) * *(float *)(in_ECX + 0x514) +
            *(float *)(in_ECX + 0x84) * *(float *)(in_ECX + 0x518) +
            *(float *)(in_ECX + 0x88) * *(float *)(in_ECX + 0x51c)) - 1.0) < 0.0001) {
      return;
    }
    fVar9 = (float10)FUN_00628140();
    fVar6 = (float)fVar9;
    if (fVar6 == 0.0) {
      return;
    }
    vector3d_cross_product((float *)(in_ECX + 0x80));
    fVar9 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar9) {
      return;
    }
    fVar9 = (float10)fcos((float10)fVar6);
    fVar10 = (float10)fsin((float10)fVar6);
    vector3d_rotate_about_axis((float)fVar10,(float)fVar9);
    vector3d_rotate_about_axis((float)fVar10,extraout_EDX);
    vector3d_normalize_with_length();
    vector3d_normalize_with_length();
    return;
  }
LAB_00560c18:
  *(undefined4 *)(in_ECX + 0x7c) = 0;
  fVar9 = (float10)vector3d_normalize_with_length();
  puVar2 = PTR_DAT_00696718;
  if ((float10)0.0 == fVar9) {
    *(undefined4 *)(in_ECX + 0x74) = *(undefined4 *)PTR_DAT_00696718;
    *(undefined4 *)(in_ECX + 0x78) = *(undefined4 *)(puVar2 + 4);
    *(undefined4 *)(in_ECX + 0x7c) = *(undefined4 *)(puVar2 + 8);
  }
  puVar2 = PTR_DAT_00696720;
  *(undefined4 *)(in_ECX + 0x80) = *(undefined4 *)PTR_DAT_00696720;
  *(undefined4 *)(in_ECX + 0x84) = *(undefined4 *)(puVar2 + 4);
  *(undefined4 *)(in_ECX + 0x88) = *(undefined4 *)(puVar2 + 8);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
