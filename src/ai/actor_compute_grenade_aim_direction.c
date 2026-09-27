// actor_compute_grenade_aim_direction  (Ghidra: actor_compute_grenade_aim_direction, renamed)
// address 0x40f7e0, size 391 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN: 30-degree clamp decoded from 0x40f89c (see note in body); prop gate and copies verified)
// evidence: phase-4 summary "computes the aim direction for a grenade throw, nudging it
// away from a too-close firing line when needed".
// register convention: actor_index in EAX, target point in EDX, output direction in EDI
// (all Ghidra unaff_/in_ registers), plus a declared stack out-param (param_1, a float).
// blam-cc: EAX -> actor_index, EDX -> target_point, EDI -> out_direction, stack -> out_698
// UNSURE: this file's middle section (the cross-product / perpendicular-fallback / rotate
// sequence once the aim direction is judged too close to the firing line) relies on
// several callees (vector3d_cross_product, and the exact operands handed to
// vector3d_rotate_about_axis) that are only ever called with 0-1 visible arguments in the
// decompiled C; the reconstruction below is a best-effort reading of the surrounding
// register-convention evidence from types/math.h, not a proven one. Needs the disassembly
// review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, UNSURE signature
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820
extern void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3]); // 0x40f9b0, this module

// blam-cc: EAX -> actor_index, EDX -> target_point, EDI -> out_direction, stack -> out_698
uint32_t actor_compute_grenade_aim_direction(datum_index actor_index, real_point3d *target_point,
                                              real_vector3d *out_direction, float *out_698)
{
    actor *self;
    uint32_t result;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    result = (uint32_t)-1;

    if (self->unknown_5f2 == 2) {
        real_vector3d aim_from;

        if (self->unknown_60c == 1 && self->unknown_610 != (uint32_t)-1) {
            prop *p = (prop *)((uint8_t *)prop_data->data + (self->unknown_610 & 0xffff) * sizeof(prop));
            if (1 < p->kind && p->kind < 4) {
                result = p->object_index;
            }
        }

        if (self->unknown_688 == 0) {
            out_direction->i = self->grenade_aim_direction.i - target_point->x;
            out_direction->j = self->grenade_aim_direction.j - target_point->y;
            out_direction->k = self->grenade_aim_direction.k - target_point->z;
            vector3d_normalize_with_length(out_direction);
        } else {
            *out_direction = self->unknown_68c;
        }

        // REWRITTEN (0x40f89c..0x40f952): the actor's aim forward F; when the throw direction D is
        // 30 degrees or more away from it (F.D < cos 30), D is replaced by F turned 30 degrees
        // toward D about normalize(D x F) (a perpendicular of F when that cross is degenerate;
        // no rotation when that fails too). The old C wrote the cross product into D and rotated
        // it about F.
        actor_get_aim_from_position(actor_index, (uint32_t *)&aim_from);

        if (!(aim_from.i * out_direction->i + aim_from.j * out_direction->j + aim_from.k * out_direction->k >= 0.8660254f)) {
            uint8_t should_rotate = 1;
            real_vector3d axis;

            vector3d_cross_product(&axis, out_direction, &aim_from); // EAX axis, ECX D, stack F
            if (vector3d_normalize_with_length(&axis) == 0.0f) {
                vector3d_build_perpendicular(&axis, &aim_from);
                if (vector3d_normalize_with_length(&axis) == 0.0f) {
                    should_rotate = 0;
                }
            }
            *out_direction = aim_from;
            if (should_rotate) {
                vector3d_rotate_about_axis(out_direction, &axis, 0.5f, 0.86602539f);
            }
        }
        *out_698 = self->unknown_698;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x40f7e0):

undefined4 FUN_0040f7e0(undefined4 *param_1)

{
  short sVar1;
  bool bVar2;
  uint in_EAX;
  int iVar3;
  float *in_EDX;
  undefined4 uVar4;
  int iVar5;
  float *unaff_EDI;
  float10 fVar6;
  float local_18;
  float local_14;
  float local_10;

  iVar5 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar4 = 0xffffffff;
  if (*(short *)(iVar5 + 0x5f2) == 2) {
    if ((*(short *)(iVar5 + 0x60c) == 1) && (*(uint *)(iVar5 + 0x610) != 0xffffffff)) {
      iVar3 = (*(uint *)(iVar5 + 0x610) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
      sVar1 = *(short *)(iVar3 + 0x24);
      if ((1 < sVar1) && (sVar1 < 4)) {
        uVar4 = *(undefined4 *)(iVar3 + 0x18);
      }
    }
    if (*(char *)(iVar5 + 0x688) == '\0') {
      *unaff_EDI = *(float *)(iVar5 + 0x67c) - *in_EDX;
      unaff_EDI[1] = *(float *)(iVar5 + 0x680) - in_EDX[1];
      unaff_EDI[2] = *(float *)(iVar5 + 0x684) - in_EDX[2];
      vector3d_normalize_with_length();
    }
    else {
      *unaff_EDI = *(float *)(iVar5 + 0x68c);
      unaff_EDI[1] = *(float *)(iVar5 + 0x690);
      unaff_EDI[2] = *(float *)(iVar5 + 0x694);
    }
    FUN_0040f9b0();
    if (local_18 * *unaff_EDI + local_14 * unaff_EDI[1] + local_10 * unaff_EDI[2] < 0.8660254) {
      bVar2 = true;
      vector3d_cross_product(&local_18);
      fVar6 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar6) {
        vector3d_build_perpendicular();
        fVar6 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 == fVar6) {
          bVar2 = false;
        }
      }
      *unaff_EDI = local_18;
      unaff_EDI[1] = local_14;
      unaff_EDI[2] = local_10;
      if (bVar2) {
        vector3d_rotate_about_axis(0x3f000000,0x3f5db3d7);
      }
    }
    *param_1 = *(undefined4 *)(iVar5 + 0x698);
  }
  return uVar4;
}
#endif
