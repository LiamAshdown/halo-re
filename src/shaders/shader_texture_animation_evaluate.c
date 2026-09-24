// shader_texture_animation_evaluate  (Ghidra: shader_texture_animation_evaluate, already named)
// address 0x53fe50, size 517 bytes
// name confidence: 0.75   rewrite confidence: 0.85 (phase-4 review, objdump checked)
// evidence: out/phase4/shaders_types_notes.md maps every offset unaff_ESI touches onto
//   shader_texture_animation (types/shaders.h) one field at a time -- u/v/rotation channels at
//   +0x00/+0x10/+0x20 (source, function, period, phase, scale) and rotation_center at
//   +0x30/+0x34 -- and confirms the seven call sites: ShaderModel+0xfc (0x528eb3),
//   ShaderTransparentChicagoMap+0xa4 (0x532419) and shader_effect+0x60 (0x534335) load ESI;
//   the render_animation* in ECX comes from transparent_geometry_group.lighting_extra (+0x74,
//   0x53242a/0x534347) or a rasterizer_model_draw_context (+0x84, 0x528eaa); EBX/EDI (the two
//   rows of a shader_texture_transform) sit 0x10 apart in one block at every call site
//   (0x528ebf/0x528ec3, 0x53240d/0x532414, 0x534339/0x534340).
// register convention: ESI -> shader_texture_animation*, ECX -> render_animation* (may be
//   NULL), EBX -> output u row (float[4]), EDI -> output v row (float[4]); stack -> u_scale,
//   v_scale, u_offset, v_offset, rotation_degrees, time (6 floats, callee does not pop them).
// blam-cc: ECX -> frame_animation, ESI -> texture_animation, EBX -> u_row, EDI -> v_row,
//   stack -> (u_scale, v_scale, u_offset, v_offset, rotation_degrees, time)
//   C parameter order follows the existing rasterizer externs (function_source, animation,
//   out_u, out_v, then the six stack floats), so the definition and the callers agree.
// objdump (0x53fe50..0x540054) checked line by line in the phase-4 review:
//   - each periodic_function_evaluate argument is (time + phase) / period computed on the x87
//     stack without an intermediate float rounding, then stored as a qword: written in double.
//   - fcos is applied to the unrounded product rotation * (pi / 180) (float * float, exact in
//     double), fsin to the same product after it was rounded to float by the fst into the
//     time argument slot [esp+0x2c]: the callee overwrites its own time stack argument with
//     the float radians. Callers pop the arguments right after (add esp,0x18) and never reread
//     that slot, so this is only a note for hooks.
//   - EAX, ECX, EDX are clobbered; EBX, ESI, EDI are preserved.
// UNSURE: whether frame_animation ever arrives non-NULL with a source index but a NULL/short
//   function_values array; the original does not guard against that either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "rasterizer.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h> // uintptr_t

extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0, math
    // module; blam-cc: AX (low half of EAX) -> type, stack -> time
extern double cos(double x); // FCOS
extern double sin(double x); // FSIN

// Evaluates one shader_texture_animation (u/v scroll plus rotation) at the given time and
// writes the resulting 2x4 texture transform as two separate rows, u_row and v_row, each
// { cos*u_scale terms, sin*v_scale terms, 0, translation }. frame_animation supplies the
// per-object function_values array that a channel's source field indexes into; when it is
// NULL every channel's source contribution is treated as 1.0 regardless of what the channel's
// source field says.
void shader_texture_animation_evaluate(render_animation *frame_animation,
                                       shader_texture_animation *texture_animation,
                                       float *u_row, float *v_row,
                                       real u_scale, real v_scale, real u_offset, real v_offset,
                                       real rotation_degrees, real time)
{
    real u_period, v_period, rotation_period;
    real u_source_value, v_source_value, rotation_source_value;
    real u_wave, v_wave, rotation_wave;
    real du, dv, total_rotation;
    double cos_r, sin_r; // kept on the x87 stack, never rounded to float, in the original

    u_period = (texture_animation->u.period == 0.0f) ? 1.0f : texture_animation->u.period;
    v_period = (texture_animation->v.period == 0.0f) ? 1.0f : texture_animation->v.period;
    rotation_period = (texture_animation->rotation.period == 0.0f) ? 1.0f : texture_animation->rotation.period;

    if (frame_animation == (render_animation *)0) {
        u_source_value = 1.0f;
        v_source_value = 1.0f;
        rotation_source_value = 1.0f;
    } else {
        real *function_values = (real *)(uintptr_t)frame_animation->function_values;

        u_source_value = (texture_animation->u.source == 0) ? 1.0f
                                                             : function_values[texture_animation->u.source - 1];
        v_source_value = (texture_animation->v.source == 0) ? 1.0f
                                                             : function_values[texture_animation->v.source - 1];
        rotation_source_value = (texture_animation->rotation.source == 0)
                                     ? 1.0f
                                     : function_values[texture_animation->rotation.source - 1];
    }

    u_wave = periodic_function_evaluate(texture_animation->u.function,
                                        ((double)time + texture_animation->u.phase) / u_period);
    v_wave = periodic_function_evaluate(texture_animation->v.function,
                                        ((double)time + texture_animation->v.phase) / v_period);
    rotation_wave = periodic_function_evaluate(
        texture_animation->rotation.function,
        ((double)time + texture_animation->rotation.phase) / rotation_period);

    du = (u_offset - texture_animation->rotation_center.x) +
         u_wave * texture_animation->u.scale * u_source_value;
    dv = (v_offset - texture_animation->rotation_center.y) +
         v_wave * texture_animation->v.scale * v_source_value;
    total_rotation = rotation_wave * texture_animation->rotation.scale * rotation_source_value +
                     rotation_degrees;

    if (total_rotation == 0.0f || total_rotation == (real)k_shader_texture_rotation_full_turn) {
        cos_r = 1.0f;
        sin_r = 0.0f;
    } else {
        // 0x00672c38 pi / 180. fcos sees the unrounded product, fsin the float-rounded one.
        double radians = (double)total_rotation * (double)0.017453292f;

        cos_r = cos(radians);
        sin_r = sin((double)(real)radians);
    }

    u_row[2] = 0.0f;
    u_row[0] = (float)(cos_r * u_scale);
    u_row[1] = (float)-(v_scale * sin_r);
    u_row[3] = (float)((cos_r * du - sin_r * dv) + texture_animation->rotation_center.x);

    v_row[2] = 0.0f;
    v_row[0] = (float)(u_scale * sin_r);
    v_row[1] = (float)(cos_r * v_scale);
    v_row[3] = (float)((sin_r * du + cos_r * dv) + texture_animation->rotation_center.y);
}

#if 0
Original Ghidra decompilation (0x53fe50):

void shader_texture_animation_evaluate
               (float param_1,float param_2,float param_3,float param_4,float param_5,float param_6)

{
  float fVar1;
  float fVar2;
  int iVar3;
  float fVar4;
  int in_ECX;
  float *unaff_EBX;
  short *unaff_ESI;
  float *unaff_EDI;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if (*(float *)(unaff_ESI + 2) == 0.0) {
    fVar1 = 1.0;
  }
  else {
    fVar1 = *(float *)(unaff_ESI + 2);
  }
  if (*(float *)(unaff_ESI + 10) == 0.0) {
    local_8 = 1.0;
  }
  else {
    local_8 = *(float *)(unaff_ESI + 10);
  }
  if (*(float *)(unaff_ESI + 0x12) == 0.0) {
    local_4 = 1.0;
  }
  else {
    local_4 = *(float *)(unaff_ESI + 0x12);
  }
  if (in_ECX == 0) {
    local_14 = 1.0;
    local_10 = 1.0;
  }
  else {
    iVar3 = *(int *)(in_ECX + 4);
    if (*unaff_ESI == 0) {
      local_10 = 1.0;
    }
    else {
      local_10 = *(float *)(iVar3 + -4 + *unaff_ESI * 4);
    }
    if (unaff_ESI[8] == 0) {
      local_14 = 1.0;
    }
    else {
      local_14 = *(float *)(iVar3 + -4 + unaff_ESI[8] * 4);
    }
    if (unaff_ESI[0x10] != 0) {
      local_c = *(float *)(iVar3 + -4 + unaff_ESI[0x10] * 4);
      goto LAB_0053ff23;
    }
  }
  local_c = 1.0;
LAB_0053ff23:
  fVar5 = (float10)periodic_function_evaluate
                             ((double)((param_6 + *(float *)(unaff_ESI + 4)) / fVar1));
  fVar1 = *(float *)(unaff_ESI + 6);
  fVar6 = (float10)periodic_function_evaluate
                             ((double)((param_6 + *(float *)(unaff_ESI + 0xc)) / local_8));
  fVar2 = *(float *)(unaff_ESI + 0xe);
  fVar7 = (float10)periodic_function_evaluate
                             ((double)((param_6 + *(float *)(unaff_ESI + 0x14)) / local_4));
  fVar4 = (param_3 - *(float *)(unaff_ESI + 0x18)) +
          (float)(fVar5 * (float10)fVar1 * (float10)local_10);
  fVar2 = (param_4 - *(float *)(unaff_ESI + 0x1a)) +
          (float)(fVar6 * (float10)fVar2 * (float10)local_14);
  fVar1 = (float)(fVar7 * (float10)*(float *)(unaff_ESI + 0x16) * (float10)local_c +
                 (float10)param_5);
  if ((fVar1 == 0.0) || (fVar1 == 360.0)) {
    fVar5 = (float10)1.0;
    fVar6 = (float10)0.0;
  }
  else {
    fVar5 = (float10)fcos((float10)fVar1 * (float10)0.017453292);
    fVar6 = (float10)fsin((float10)(float)((float10)fVar1 * (float10)0.017453292));
  }
  unaff_EBX[2] = 0.0;
  *unaff_EBX = (float)(fVar5 * (float10)param_1);
  unaff_EBX[1] = (float)-((float10)param_2 * fVar6);
  unaff_EBX[3] = (float)((fVar5 * (float10)fVar4 - fVar6 * (float10)fVar2) +
                        (float10)*(float *)(unaff_ESI + 0x18));
  unaff_EDI[2] = 0.0;
  *unaff_EDI = (float)((float10)param_1 * fVar6);
  unaff_EDI[1] = (float)(fVar5 * (float10)param_2);
  unaff_EDI[3] = (float)(fVar6 * (float10)fVar4 + fVar5 * (float10)fVar2 +
                        (float10)*(float *)(unaff_ESI + 0x1a));
  return;
}
#endif
