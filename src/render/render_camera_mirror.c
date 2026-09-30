// render_camera_mirror  (Ghidra: FUN_0050c660; named from out/phase4/render_types_notes.md,
// which documents this exact address as "render_camera_mirror 0x50c660" and its
// structure_bsp_mirror_result argument evidence)
// address 0x50c660, size 818 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/render_types_notes.md's structure_bsp_mirror_result row: "render_camera_
//   mirror 0x50c660 reads the plane, +0x10 and +0x14"; types/structures.h's own note that those
//   two floats come from the mirror shader (ShaderEnvironment +0x30c/+0x310) when it is a
//   shadertype_environment. Disassembly (objdump -d -M intel, 0x50c660..0x50c991) confirms:
//   EBX is a real register parameter (its own prologue rep-movsd's 0x15 dwords from ESI=EBX into
//   EDI=the second stack argument, i.e. the whole render_camera), the "else" branch's
//   vector3d_cross_product_length call takes EAX=&(the mirror plane's saved normal, spilled to
//   the stack right after it is loaded) and ECX=&camera->forward, and the final three field
//   writes (float* indices 0xf, 0x11-0x14) land on render_camera.z_near (+0x3c) and
//   .mirror_plane (+0x44, a real_plane3d) respectively.
// review fix (phase-4 gate, objdump 0x50c660..0x50c991): the nudged plane offset projects the
//   camera along the mirror normal (fld [esp+0x28] = plane.i; fmul st,st(1); fadd [ebx]), not
//   along the camera forward vector as the first draft had it.
// register convention: EBX = source_camera (render_camera*), stack = (mirror, out_camera).
//   // blam-cc: EBX=source_camera, stack=(mirror, out_camera)
// UNSURE: the near-grazing-angle correction (the `< 0.0125` branch) and the portal-shift branch's
//   formula are reproduced arithmetically exactly; their geometric intent (avoiding a degenerate
//   reflection when the camera looks edge-on into the mirror, and a shader-driven parallax shift
//   for portal-style mirrors) is inferred from shape, not confirmed against another source.
// reconciled: R47 structure_bsp_mirror_result.unknown_10/_14 -> shader_mirror_value_0/_1 (ShaderEnvironment runtime_mirror_value_0/_1 at +0x30c/+0x310)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "structures.h"
#include "fn_math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction
extern double fabs(double x); // ABS is a single x87 FABS instruction


                                                               // blam-cc: ECX -> v

                                                                               // module; blam-cc:
                                                                               // EAX -> a, ECX -> b

// Builds a mirrored copy of source_camera across the mirror's plane: for an ordinary mirror
// (mirror->shader_mirror_value_0 == 0) this is a full reflection of position, forward and up (with a small
// correction when the camera looks nearly edge-on into the plane, and an extra flip of the
// reflected up vector to keep the basis right-handed for the rasterizer); for a portal-style
// mirror it instead shifts the camera along the plane's normal by an amount driven by the mirror
// shader's two runtime floats. Either way the mirror flag is set (or cleared) and the camera's
// near clip plane and mirror_plane fields are stamped for the rasterizer.
void render_camera_mirror(render_camera *source_camera, structure_bsp_mirror_result *mirror,
                           render_camera *out_camera) // blam-cc: EBX=source_camera, stack=(mirror, out_camera)
{
    real_vector3d plane_normal = mirror->plane.normal;
    float plane_d = mirror->plane.d;

    *out_camera = *source_camera;

    if (mirror->shader_mirror_value_0 == 0.0f) {
        real_vector3d reflect_dir = plane_normal;
        float baseline = plane_d;
        float dot_forward;
        float dot_up;
        float dot_position;

        if ((float)fabs((double)(plane_normal.i * source_camera->forward.i +
                                  plane_normal.j * source_camera->forward.j +
                                  plane_normal.k * source_camera->forward.k)) < 0.0125f) {
            // Nudges the reflection normal toward the camera's forward direction (0.005859375,
            // 0x672f38) when |n . forward| < 0.0125 (double 0x672f40) and re-derives the plane
            // offset from the camera's projection onto the mirror plane.
            float signed_offset = -((plane_normal.j * source_camera->position.y +
                                      plane_normal.k * source_camera->position.z +
                                      plane_normal.i * source_camera->position.x) - plane_d);
            reflect_dir.i = source_camera->forward.i * 0.005859375f + plane_normal.i;
            reflect_dir.j = source_camera->forward.j * 0.005859375f + plane_normal.j;
            reflect_dir.k = source_camera->forward.k * 0.005859375f + plane_normal.k;
            vector3d_normalize_with_length(&reflect_dir);
            // the camera position projected onto the mirror plane (normal * offset + position,
            // 0x50c6fa..0x50c71c), measured along the nudged normal
            baseline = reflect_dir.k * (plane_normal.k * signed_offset + source_camera->position.z) +
                       reflect_dir.j * (plane_normal.j * signed_offset + source_camera->position.y) +
                       reflect_dir.i * (plane_normal.i * signed_offset + source_camera->position.x);
        }

        dot_forward = (reflect_dir.i * source_camera->forward.i + reflect_dir.j * source_camera->forward.j +
                       reflect_dir.k * source_camera->forward.k) * 2.0f;
        out_camera->forward.i = source_camera->forward.i - reflect_dir.i * dot_forward;
        out_camera->forward.j = source_camera->forward.j - reflect_dir.j * dot_forward;
        out_camera->forward.k = source_camera->forward.k - reflect_dir.k * dot_forward;

        dot_up = (reflect_dir.i * source_camera->up.i + reflect_dir.j * source_camera->up.j +
                  reflect_dir.k * source_camera->up.k) * 2.0f;
        out_camera->up.i = source_camera->up.i - reflect_dir.i * dot_up;
        out_camera->up.j = source_camera->up.j - reflect_dir.j * dot_up;
        out_camera->up.k = source_camera->up.k - reflect_dir.k * dot_up;

        dot_position = ((reflect_dir.j * source_camera->position.y + reflect_dir.k * source_camera->position.z +
                         reflect_dir.i * source_camera->position.x) - baseline) * -2.0f;
        out_camera->position.x = reflect_dir.i * dot_position + source_camera->position.x;
        out_camera->position.y = reflect_dir.j * dot_position + source_camera->position.y;
        out_camera->position.z = reflect_dir.k * dot_position + source_camera->position.z;

        out_camera->mirrored = (uint8_t)(source_camera->mirrored == 0);

        // Flips the just-reflected up vector again, to keep the mirrored camera's basis
        // right-handed for the rasterizer.
        out_camera->up.i = -out_camera->up.i;
        out_camera->up.j = -out_camera->up.j;
        out_camera->up.k = -out_camera->up.k;
    } else {
        // UNSURE: portal shift, driven by the mirror shader's two runtime floats
        // (mirror->shader_mirror_value_0/unknown_14, ShaderEnvironment +0x30c/+0x310).
        float inverse_forward_length =
            1.0f / (float)sqrt((double)(source_camera->forward.k * source_camera->forward.k +
                                         source_camera->forward.j * source_camera->forward.j +
                                         source_camera->forward.i * source_camera->forward.i));
        float sin_angle = vector3d_cross_product_length(&plane_normal, &source_camera->forward) *
                           inverse_forward_length;
        float shift = sin_angle * mirror->shader_mirror_value_0;

        if (sin_angle == 0.0f) {
            shift = 0.0f;
        } else {
            float dot_normal_forward = plane_normal.i * source_camera->forward.i +
                                        plane_normal.j * source_camera->forward.j +
                                        plane_normal.k * source_camera->forward.k;
            shift = -((dot_normal_forward * inverse_forward_length * shift * mirror->shader_mirror_value_1) /
                      ((float)sqrt((double)(1.0f - shift * shift)) * sin_angle));
        }

        out_camera->position.x = plane_normal.i * shift + source_camera->position.x;
        out_camera->position.y = plane_normal.j * shift + source_camera->position.y;
        out_camera->position.z = plane_normal.k * shift + source_camera->position.z;
    }

    out_camera->z_near = 0.0f;
    out_camera->mirror_plane.normal = plane_normal;
    out_camera->mirror_plane.d = plane_d;
}

#if 0
Original Ghidra decompilation (0x50c660):

void FUN_0050c660(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  int iVar9;
  float *unaff_EBX;
  float *pfVar10;
  float *pfVar11;
  float10 fVar12;
  float local_10;
  float local_c;
  float local_8;

  fVar4 = *param_1;
  fVar5 = param_1[1];
  fVar6 = param_1[2];
  fVar7 = param_1[3];
  pfVar10 = unaff_EBX;
  pfVar11 = param_2;
  for (iVar9 = 0x15; iVar9 != 0; iVar9 = iVar9 + -1) {
    *pfVar11 = *pfVar10;
    pfVar10 = pfVar10 + 1;
    pfVar11 = pfVar11 + 1;
  }
  if (param_1[4] == 0.0) {
    fVar1 = fVar7;
    local_10 = fVar4;
    local_c = fVar5;
    local_8 = fVar6;
    if (ABS(fVar4 * unaff_EBX[3] + fVar5 * unaff_EBX[4] + fVar6 * unaff_EBX[5]) < 0.0125) {
      fVar8 = -((fVar5 * unaff_EBX[1] + fVar6 * unaff_EBX[2] + fVar4 * *unaff_EBX) - fVar7);
      fVar1 = *unaff_EBX;
      fVar2 = unaff_EBX[1];
      fVar3 = unaff_EBX[2];
      local_10 = unaff_EBX[3] * 0.005859375 + fVar4;
      local_c = unaff_EBX[4] * 0.005859375 + fVar5;
      local_8 = unaff_EBX[5] * 0.005859375 + fVar6;
      vector3d_normalize_with_length();
      fVar1 = local_10 * (fVar4 * fVar8 + fVar1) +
              local_c * (fVar5 * fVar8 + fVar2) + local_8 * (fVar6 * fVar8 + fVar3);
    }
    fVar2 = local_c * unaff_EBX[4] + local_8 * unaff_EBX[5] + local_10 * unaff_EBX[3];
    fVar2 = fVar2 + fVar2;
    param_2[3] = unaff_EBX[3] - local_10 * fVar2;
    param_2[4] = unaff_EBX[4] - local_c * fVar2;
    param_2[5] = unaff_EBX[5] - local_8 * fVar2;
    fVar2 = local_c * unaff_EBX[7] + local_8 * unaff_EBX[8] + local_10 * unaff_EBX[6];
    fVar2 = fVar2 + fVar2;
    param_2[6] = unaff_EBX[6] - local_10 * fVar2;
    param_2[7] = unaff_EBX[7] - local_c * fVar2;
    param_2[8] = unaff_EBX[8] - local_8 * fVar2;
    fVar1 = ((local_c * unaff_EBX[1] + local_8 * unaff_EBX[2] + local_10 * *unaff_EBX) - fVar1) *
            -2.0;
    *param_2 = local_10 * fVar1 + *unaff_EBX;
    param_2[1] = local_c * fVar1 + unaff_EBX[1];
    param_2[2] = local_8 * fVar1 + unaff_EBX[2];
    *(bool *)(param_2 + 9) = *(char *)(unaff_EBX + 9) == '\0';
    param_2[6] = -param_2[6];
    param_2[7] = -param_2[7];
    param_2[8] = -param_2[8];
  }
  else {
    fVar1 = unaff_EBX[3];
    fVar2 = 1.0 / SQRT(unaff_EBX[5] * unaff_EBX[5] + unaff_EBX[4] * unaff_EBX[4] + fVar1 * fVar1);
    fVar12 = (float10)vector3d_cross_product_length();
    fVar12 = fVar12 * (float10)fVar2;
    fVar1 = (float)fVar12;
    fVar12 = fVar12 * (float10)param_1[4];
    if (fVar1 == 0.0) {
      fVar12 = (float10)0.0;
    }
    else {
      fVar12 = -((((float10)fVar4 * (float10)unaff_EBX[3] +
                  (float10)fVar5 * (float10)unaff_EBX[4] + (float10)fVar6 * (float10)unaff_EBX[5]) *
                  (float10)fVar2 * fVar12 * (float10)param_1[5]) /
                (SQRT((float10)1.0 - fVar12 * fVar12) * (float10)fVar1));
    }
    *param_2 = (float)((float10)fVar4 * fVar12 + (float10)*unaff_EBX);
    param_2[1] = (float)((float10)fVar5 * fVar12 + (float10)unaff_EBX[1]);
    param_2[2] = (float)((float10)fVar6 * fVar12 + (float10)unaff_EBX[2]);
  }
  param_2[0xf] = 0.0;
  param_2[0x11] = fVar4;
  param_2[0x12] = fVar5;
  param_2[0x13] = fVar6;
  param_2[0x14] = fVar7;
  return;
}

Key disassembly excerpts (objdump -d -M intel) confirming the register-passed EBX parameter and
the vector3d_cross_product_length arguments:

0050c660:
  sub    esp,0x3c
  mov    eax,DWORD PTR [esp+0x40]     ; stack arg 1 = mirror
  ...
  push   ebp
  mov    ebp,DWORD PTR [esp+0x48]     ; stack arg 2 = out_camera
  ...
  mov    ecx,0x15
  mov    esi,ebx                      ; source = EBX (source_camera)
  mov    edi,ebp                      ; dest = out_camera
  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]   ; *out_camera = *source_camera
  ...
0050c8a1 (the mirror->unknown_10 != 0 branch):
  lea    esi,[ebx+0xc]                ; &source_camera->forward
  lea    eax,[esp+0x28]               ; &plane_normal (spilled to the stack earlier)
  mov    ecx,esi                      ; ecx = &source_camera->forward
  ...
  call   0x4cd380                     ; vector3d_cross_product_length(EAX=&plane_normal, ECX=&forward)
#endif
