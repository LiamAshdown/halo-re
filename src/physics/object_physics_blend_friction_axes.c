// object_physics_blend_friction_axes  (Ghidra: FUN_00507c00; renamed)
// address 0x507c00, size 188 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: types/physics.h mass_point_state's nine-float friction triple comment ("the nine-
//   float layout 0x00507c00 expects: the blended result first, then the two candidate axes it
//   mixes, chosen by PhysicsMassPoint.friction_type and scaled by friction_parallel_scale and
//   friction_perpendicular_scale"), matching this function's [0..2]=[3..5]*scale1+[6..8]*scale2
//   tail exactly for all four friction_type paths.
// register convention: unaff_EDI -> friction (float[9], in place: [0..2] holds a caller-seeded
//   reference vector on entry and the blended friction force on return, [3..5]/[6..8] are the
//   two candidate axes). param_1/param_2/param_3 are Ghidra's own recognized parameters
//   (friction_type, parallel_scale, perpendicular_scale).
//   // blam-cc: EDI -> friction, stack -> friction_type, parallel_scale, perpendicular_scale
// UNSURE (major): vector3d_cross_product and vector3d_project_onto_unit_axis are both called
//   with ZERO visible arguments (their real signatures need an output pointer plus one or two
//   input vectors each, confirmed from their own fully-decompiled bodies), and nothing in this
//   function's own decompile identifies which registers carried them. This rewrite declares and
//   calls both exactly as Ghidra shows -- with no arguments -- rather than inventing plausible
//   ones it cannot verify; the friction_type == 2 and == 3 branches are therefore not fully
//   reconstructed, only their control flow is.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void vector3d_cross_product(real_vector3d *out); // 0x4052c0, math module; the two
    // operands arrive in registers, only the output pointer is on the stack (symbols/functions.txt)
extern void vector3d_project_onto_unit_axis(void); // 0x4cda30, math module; UNSURE args, see header

// Chooses and scales two candidate friction axes into friction[3..5] (parallel) and
// friction[6..8] (perpendicular) according to friction_type, then blends them by
// parallel_scale/perpendicular_scale into friction[0..2]:
//   0: candidate1 = the caller-seeded reference vector (friction[0..2] as it was on entry),
//      candidate2 = zero.
//   1: candidates left exactly as the caller already set them (no computation here).
//   2: candidate1 from vector3d_cross_product, then candidate2 from
//      vector3d_project_onto_unit_axis (both UNSURE, see file header).
//   3: candidate2 from vector3d_project_onto_unit_axis only.
//   anything else: candidates left as-is, same as case 1.
void object_physics_blend_friction_axes(int16_t friction_type, float parallel_scale,
    float perpendicular_scale, float *friction)
{
    if (friction_type == 0) {
        friction[3] = friction[0];
        friction[4] = friction[1];
        friction[5] = friction[2];
        friction[6] = 0.0f;
        friction[7] = 0.0f;
        friction[8] = 0.0f;
        return;
    }

    if (friction_type != 1) {
        if (friction_type == 2) {
            vector3d_cross_product((real_vector3d *)&friction[3]); // UNSURE: operands are registers
        } else if (friction_type != 3) {
            goto blend;
        }
        vector3d_project_onto_unit_axis(); // UNSURE args, writes candidate2 (friction[6..8])
    }

blend:
    friction[3] = parallel_scale * friction[3];
    friction[4] = parallel_scale * friction[4];
    friction[5] = parallel_scale * friction[5];
    friction[6] = perpendicular_scale * friction[6];
    friction[7] = perpendicular_scale * friction[7];
    friction[8] = perpendicular_scale * friction[8];
    friction[0] = friction[3] + friction[6];
    friction[1] = friction[7] + friction[4];
    friction[2] = friction[8] + friction[5];
}

#if 0
Original Ghidra decompilation (0x507c00):

void FUN_00507c00(short param_1,float param_2,float param_3)

{
  float *unaff_EDI;

  if (param_1 == 0) {
    unaff_EDI[3] = *unaff_EDI;
    unaff_EDI[4] = unaff_EDI[1];
    unaff_EDI[5] = unaff_EDI[2];
    unaff_EDI[6] = 0.0;
    unaff_EDI[7] = 0.0;
    unaff_EDI[8] = 0.0;
    return;
  }
  if (param_1 != 1) {
    if (param_1 == 2) {
      vector3d_cross_product();
    }
    else if (param_1 != 3) goto LAB_00507c61;
  }
  vector3d_project_onto_unit_axis();
LAB_00507c61:
  unaff_EDI[3] = param_2 * unaff_EDI[3];
  unaff_EDI[4] = param_2 * unaff_EDI[4];
  unaff_EDI[5] = param_2 * unaff_EDI[5];
  unaff_EDI[6] = param_3 * unaff_EDI[6];
  unaff_EDI[7] = param_3 * unaff_EDI[7];
  unaff_EDI[8] = param_3 * unaff_EDI[8];
  *unaff_EDI = unaff_EDI[3] + unaff_EDI[6];
  unaff_EDI[1] = unaff_EDI[7] + unaff_EDI[4];
  unaff_EDI[2] = unaff_EDI[8] + unaff_EDI[5];
  return;
}
#endif
