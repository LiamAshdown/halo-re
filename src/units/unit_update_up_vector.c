// unit_update_up_vector  (Ghidra: unit_update_up_vector)
// address 0x560800, size 1136 bytes
// name confidence: 0.45 (phase2 candidate)   rewrite confidence: 0.15
// evidence: types/objects.h object.forward/up (0x74/0x80); types/units.h
//   biped_data.ground_surface_index/.ground_normal/.unknown_510 (0x4d8/0x514/0x510),
//   biped_data.flags (0x4cc, bit 0 grounded); types/tags.h BipedFlags (tag+0x2f4, "flying" bit
//   0x4, "can_climb_any_surface" bit 0x40); types/math.h global_forward3d_pointer/
//   global_left3d_pointer/global_up3d_pointer (0x696718/0x69671c/0x696720).
// register convention: the owning Biped tag data pointer in EAX, the unit/object pointer in
//   ECX.
//   // blam-cc: in_EAX -> biped_tag, in_ECX -> unit_index (folded into an object pointer)
// UNSURE: this is one of the least certain files in the batch. Every vector3d_cross_product /
//   vector3d_normalize_with_length / vector3d_rotate_about_axis call here is a math-module
//   helper whose exact argument layout (single in/out vector vs. two operands via adjacent
//   stack slots) could not be pinned down from this decompile alone -- Ghidra shows most of
//   them with only one visible argument despite the surrounding stack locals clearly holding
//   several vectors at once. The four branches (flying, can-climb-any-surface with an active
//   ground plane, can-climb-any-surface while airborne, and the grounded/ungrounded fallback
//   that levels toward ground_normal or the world up axis) are reproduced structurally with
//   their gating conditions and final object.up/object.forward writes, but the intermediate
//   10-degree-bounded rotation math is a best-effort sketch and is very likely inexact.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_vector3d *global_left3d_pointer;    // 0x0069671c
extern real_vector3d *global_up3d_pointer;      // 0x00696720

extern double cos(double x); // x87 FCOS
extern double sin(double x); // x87 FSIN
extern real vector3d_normalize_with_length(real_vector3d *v);          // 0x401990, in place, returns the original length, vector in ECX
// vector3d_cross_product (0x4052c0) computes  *out = stack_operand x ecx_operand,  with out
// in EAX, ecx_operand in ECX and stack_operand pushed -- read out of the callee own
// decompilation (in_EAX / in_ECX / param_1) and matching
// src/objects/object_set_position_and_orientation.c. Ghidra binds only the stack operand at
// the call sites below, so the declaration is left unprototyped.
extern void vector3d_cross_product(); // 0x4052c0
// vector3d_rotate_about_axis (0x4cd820) rotates the vector in EAX about the axis in ECX in
// place, by the (sin_angle, cos_angle) pair pushed on the stack -- the callee own
// decompilation is a Rodrigues formula over in_EAX / in_ECX / param_1 / param_2, and
// src/math/vector3d_rotate_toward.c reads it the same way. Ghidra binds only the two stack
// arguments at the call sites below, so the declaration is left unprototyped.
extern void vector3d_rotate_about_axis(); // 0x4cd820  // real signature (vector3d_rotate_about_axis.c): void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 0 of 4 args at this call site
extern float FUN_00628140(void); // 0x628140, UNSURE signature/module (returns an angle)

void unit_update_up_vector(Biped *biped_tag, object *obj) // blam-cc: see file header
{
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    uint8_t frozen = (obj->vitality_flags & _object_health_frozen_bit) != 0;

    if ((biped_tag->biped_flags & 4) != 0 && !frozen) { // flying
        real_vector3d ref = *global_forward3d_pointer;
        real_vector3d perp = *global_left3d_pointer;
        vector3d_cross_product(&obj->forward, &ref);   // UNSURE: exact operand pairing
        vector3d_cross_product(&perp, &perp);
        if (vector3d_normalize_with_length(&perp) == 0.0f) {
            ref = *global_forward3d_pointer;
            perp = *global_left3d_pointer;
        }
        float c = (float)cos((double)biped->unknown_510);
        float s = (float)sin((double)biped->unknown_510);
        ref.i *= c; ref.j *= c; ref.k *= c;
        vector3d_normalize_with_length(&perp);
        obj->up.i = perp.i * s + ref.i;
        obj->up.j = perp.j * s + ref.j;
        obj->up.k = perp.k * s + ref.k;
        return;
    }

    if ((biped_tag->biped_flags & 0x40) == 0) { // not can_climb_any_surface
        if (!frozen) {
            goto level_to_world_up;
        }
        // frozen: falls through to the grounded/ungrounded fallback below
    } else if (!frozen) {
        real_vector3d target;
        if (biped->ground_surface_index == (uint32_t)-1) { // airborne; UNSURE, see unit_find_nearest_valid_surface_plane.c
            target = obj->up;
        } else {
            target = biped->ground_normal;
            real_vector3d probe = target;
            vector3d_cross_product(&obj->up, &probe);
            if (vector3d_normalize_with_length(&probe) == 0.0f) {
                float dot = target.i * obj->up.i + target.j * obj->up.j + target.k * obj->up.k;
                if (dot <= 0.0f) {
                    real_vector3d axis = obj->forward;
                    float c = (float)cos(0.17453292f); // ~10 degrees
                    float s = (float)sin(0.17453292f);
                    vector3d_rotate_about_axis(&target, &axis, s, c);
                    vector3d_cross_product(&target, &probe);
                    float dot2 = probe.i * axis.i + probe.j * axis.j + probe.k * axis.k;
                    if (dot2 <= 0.0f) {
                        target = obj->up;
                    }
                }
            }
        }

        real_vector3d out = target;
        vector3d_cross_product(&obj->forward, &out);
        if (vector3d_normalize_with_length(&out) == 0.0f) {
            vector3d_cross_product(&out, &out);
            vector3d_cross_product(&out, &out);
            if (vector3d_normalize_with_length(&out) == 0.0f) {
                out = *global_up3d_pointer;
                obj->forward = *global_forward3d_pointer;
            }
        }
        obj->up = out;
        return;
    }

    if ((biped->flags & 1) == 0) { // not grounded
        float dot = obj->up.i * biped->ground_normal.i + obj->up.j * biped->ground_normal.j +
                    obj->up.k * biped->ground_normal.k;
        if (dot - 1.0f < 0.0001f && dot - 1.0f > -0.0001f) {
            return;
        }
        float angle = FUN_00628140();
        if (angle == 0.0f) {
            return;
        }
        real_vector3d axis = obj->up;
        vector3d_cross_product(&biped->ground_normal, &axis);
        if (vector3d_normalize_with_length(&axis) == 0.0f) {
            return;
        }
        float c = (float)cos((double)angle);
        float s = (float)sin((double)angle);
        vector3d_rotate_about_axis(&obj->up, &axis, s, c);
        vector3d_rotate_about_axis(&obj->forward, &axis, s, c); // UNSURE: extraout_EDX operand
        vector3d_normalize_with_length(&obj->up);
        vector3d_normalize_with_length(&obj->forward);
        return;
    }

level_to_world_up:
    obj->forward.k = 0.0f;
    if (vector3d_normalize_with_length(&obj->forward) == 0.0f) {
        obj->forward = *global_forward3d_pointer;
    }
    obj->up = *global_up3d_pointer;
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
