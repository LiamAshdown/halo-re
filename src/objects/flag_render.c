// flag_render
// address 0x4fc350, size 1775 bytes, zero recorded callers (dead/unreferenced in this binary)
// name confidence: 0.55 (functions.md: "Builds the flag's cloth mesh geometry (vertex normals
//   and triangulated index list) for the current frame and submits it for rendering")
// rewrite confidence: 0.15 (by far the least confident file in this batch: Ghidra's own
//   decompile uses `unaff_retaddr` -- it could not even place this function's own return
//   address, which only happens when its real stack frame/calling convention is badly
//   mismatched to what Ghidra assumed. Combined with zero callers to cross-check against, and a
//   long tail of raw D3D-style vtable calls (`(**(code **)(*DAT_006e09e8 + 0x30))(...)`) whose
//   targets are entirely outside this module, this file is kept as a close, mechanical
//   transliteration of the decompiled arithmetic rather than a validated reconstruction.
//   Nothing here should be trusted without re-deriving it from the actual disassembly.)
// evidence: types/objects.h flag (object_index 0x08, vertices 0x1c stride 0x18,
//   cell_split_codes 0x1534); types/tags.h Flag (width 0x0c, height 0x0e); the vertex-normal
//   loop's addressing (`param_1 + (row*height+col)*6 + 7`) matches flag.vertices exactly, which
//   is the strongest anchor in the whole function.
// UNSURE: param_1 (flag*) and param_2 (a second, unidentified 0x1d-dword block copied
//   verbatim into a submission buffer) are the only two parameters Ghidra placed on solid
//   ground; `in_EAX` (apparently a Flag tag pointer, tested and read at offsets matching Flag's
//   width/height/etc.) and `unaff_retaddr` (apparently a *second* geometry buffer, indexed the
//   same way as param_1's vertices but with object+0x1c/+0x20/+0x24-style position fields) are
//   both guessed from usage, not confirmed by disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sqrt(double x);

extern int32_t rasterizer_dynamic_index_cache_reserve(void);   // out of module scope, unexamined
extern int32_t rasterizer_dynamic_vertex_cache_reserve(void);   // out of module scope, unexamined
extern void *rasterizer_dynamic_vertex_cache_lock(void);     // out of module scope, unexamined
extern void *rasterizer_dynamic_index_slot_lock(void);     // out of module scope, unexamined
extern void rasterizer_model_draw_prepare_states(uint32_t flag_arg); // out of module scope, unexamined
extern void rasterizer_shader_environment_draw_dispatch(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f); // out of module scope
extern void rasterizer_transparent_geometry_group_build(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f,
                          int32_t g, void *h); // out of module scope
extern void rasterizer_model_draw_restore_states(void);      // out of module scope, unexamined
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *object_data;      // 0x008603b0

void flag_render(uint32_t *entry /*param_1, flag* */, uint32_t *submission_block /*param_2, UNSURE*/,
                  Flag *tag /*in_EAX, UNSURE*/, uint8_t *second_geometry /*unaff_retaddr, UNSURE*/)
    // blam-cc: UNSURE throughout, see file header; this signature is a guess with no verified
    // call site (zero recorded callers)
{
    int16_t width = ((struct Flag *)tag)->width;
    int16_t height = ((struct Flag *)tag)->height;
    int32_t model_context;
    void *normal_buffer;
    void *index_buffer;
    int16_t col, row;
    int16_t triangle_count = 0;
    int16_t index_count = 0;
    float inv_width_minus1 = 1.0f / (float)(width - 1);
    float inv_height_minus1 = 1.0f / (float)(height - 1);

    model_context = rasterizer_dynamic_index_cache_reserve();
    if (model_context == -1 || rasterizer_dynamic_vertex_cache_reserve() == -1) {
        return;
    }
    normal_buffer = rasterizer_dynamic_vertex_cache_lock();
    index_buffer = rasterizer_dynamic_index_slot_lock();

    // Per-vertex normal computation (cross of the two grid-adjacent edge vectors, normalized).
    for (col = 0; col < width; col++) {
        for (row = 0; row < height; row++) {
            int16_t col_clamped = (col >= width - 1) ? col - 1 : col;
            int16_t row_clamped = (row >= height - 1) ? row - 1 : row;
            int32_t base_index = col_clamped * height + row_clamped;
            uint32_t *next_row_vertex = entry + ((col_clamped + 1) * height + row_clamped) * 6 + 7;
            uint32_t *base_vertex = entry + base_index * 6 + 7;
            float ex_x = *(float *)next_row_vertex - *(float *)(entry + base_index * 6 + 7);
            float ex_y = ((float *)next_row_vertex)[1] - *(float *)(entry + base_index * 6 + 8);
            float ex_z = ((float *)next_row_vertex)[2] - *(float *)(entry + base_index * 6 + 9);
            float ey_x = *(float *)(entry + base_index * 6 + 0xd) - *(float *)(entry + base_index * 6 + 7);
            float ey_y = *(float *)(entry + base_index * 6 + 0xe) - *(float *)(entry + base_index * 6 + 8);
            float ey_z = *(float *)(entry + base_index * 6 + 0xf) - *(float *)(entry + base_index * 6 + 9);
            uint32_t *out = (uint32_t *)((uint8_t *)normal_buffer + triangle_count * 0x44);
            float nx = ey_z * ex_y - ey_y * ex_z;
            float ny = ey_y * ex_x - ey_z * ex_x; // UNSURE: preserved literally from the decompile
            float nz = ey_x * ex_x - ey_y * ex_x; // (this cross product looks miscomputed even in
                                                   // the original; not corrected here)
            float len;

            ((float *)out)[3] = nx;
            ((float *)out)[4] = ny;
            ((float *)out)[5] = nz;
            len = (float)sqrt(nz * nz + ny * ny + nx * nx);
            if (len >= 0.0001f || len <= -0.0001f) {
                float inv = 1.0f / len;
                ((float *)out)[3] = inv * nx;
                ((float *)out)[4] = inv * ny;
                ((float *)out)[5] = inv * nz;
            }

            *(float *)out = *(float *)base_vertex;
            ((float *)out)[1] = ((float *)base_vertex)[1];
            ((float *)out)[2] = ((float *)base_vertex)[2];
            ((float *)out)[0xc] = (float)col * inv_width_minus1;
            ((float *)out)[0xd] = (float)row * inv_height_minus1;
            *(int16_t *)(out + 0xe) = 0;
            *(int16_t *)((uint8_t *)out + 0x3a) = 0;
            out[0xf] = 0x3f000000; // 0.5f bit pattern
            out[0x10] = 0x3f000000;

            triangle_count = triangle_count + 1;
        }
    }

    // Triangulate each cell according to its stored quad-diagonal split code.
    col = 0;
    row = 0;
    if (width != 1 && width - 1 >= 0) {
        int32_t r;
        for (r = 0; r < width - 1; r++) {
            int32_t c;
            for (c = 0; c < height - 1; c++) {
                uint16_t split = *(uint16_t *)((uint8_t *)entry + (r * (height - 1) + c) * 2 + 0x1534);
                int16_t *tri;

                switch (split) {
                case 0:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * (row + 1) + col;
                    index_count++;
                    tri[2] = height * row + 1 + col;
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + 1 + col;
                    tri[1] = height * (row + 1) + col;
                    tri[2] = height * (row + 1) + 1 + col;
                    index_count++;
                    col++;
                    continue;
                case 2:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * row + 1 + col;
                    tri[2] = (row + 1) * height + col;
                    break;
                case 3:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * row + 1 + col;
                    tri[2] = (row + 1) * height + 1 + col;
                    break;
                case 4:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * (row + 1) + 1 + col;
                    tri[2] = (row + 1) * height;
                    break;
                case 5:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + 1 + col;
                    tri[1] = height * (row + 1) + 1 + col;
                    tri[2] = (row + 1) * height;
                    break;
                default:
                    col++;
                    continue;
                }
                index_count++;
                col++;
            }
            row++;
            col = 0;
        }
    }

    // From here on: raw D3D-style rendering submission (vtable calls, per-frame render state,
    // a face-centre average for a lighting/fog sample, and the final draw). Left as a very
    // literal transliteration; none of the vtable targets are in this module and the exact
    // shape of `submission_block` / `second_geometry` was not independently verified.
    {
        void ***device = (void ***)0x006e09e8; // UNSURE: DAT_006e09e8 dereferenced as a COM-style vtable object
        (*(void (__stdcall **)(void *))((uint8_t *)(*device)[0] + 0x30 * 0))(device); // UNSURE: literal vtable slot 0x30 call, arg count guessed
    }

    {
        Flag *fallback_tag = (Flag *)tag_instances[/* UNSURE: (local_114 & 0xffff) */ 0].data;
        int32_t stride = height;
        float *v0 = (float *)(second_geometry + 4 + stride * 0x18);
        float *v1 = (float *)(second_geometry + 0x1c + stride * (width - 1) * 0x18);
        float *v2 = (float *)(second_geometry + 4 + width * stride * 0x18);
        float cx = (v2[0] + v0[0] + v1[0] + *(float *)(second_geometry + 0x1c)) * 0.25f;
        float cy = (v2[1] + v0[1] + v1[1] + *(float *)(second_geometry + 0x20)) * 0.25f;
        float cz = (v2[2] + v0[2] + v1[2] + *(float *)(second_geometry + 0x24)) * 0.25f;

        (void)fallback_tag;
        rasterizer_model_draw_prepare_states(0);
        rasterizer_shader_environment_draw_dispatch(0, 0, 0, row - 1, index_count, 0); // UNSURE: shader-kind branch (cases 1
            // and 5..11 call FUN_0052b180 with an extra `&(cx,cy,cz)` argument instead) collapsed
            // to the common path here; see the #if 0 block for the real branch.
        rasterizer_model_draw_restore_states();
    }
}

#if 0
Original Ghidra decompilation (0x4fc350):

void flag_render(undefined4 *param_1,undefined4 *param_2)

{
  float *pfVar1;
  float *pfVar2;
  float *pfVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  undefined4 uVar12;
  short sVar13;
  short sVar14;
  int in_EAX;
  int iVar15;
  short sVar16;
  int iVar17;
  short *psVar18;
  short sVar19;
  int *piVar20;
  undefined4 *puVar21;
  int unaff_retaddr;
  int local_118;
  uint local_114;
  float local_110;
  float local_10c;
  float local_108;
  int iStack_104;
  int local_100;
  int local_fc;
  int local_f8;
  int local_f4;
  float local_f0;
  int local_ec;
  undefined4 *local_e8;
  int local_e4;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d4;
  int local_d0 [2];
  undefined *puStack_c8;
  undefined2 uStack_c4;
  undefined4 auStack_c0 [29];
  undefined4 uStack_4c;
  undefined4 uStack_48;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  undefined4 uStack_c;
  undefined4 uStack_8;

  if (*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1[2] & 0xffff) * 0xc) + 0xb8)
      == 0) {
    local_110 = *(float *)(in_EAX + 0x24);
  }
  else {
    local_110 = *(float *)(in_EAX + 0x50);
  }
  if (local_110 == -NAN) {
    local_110 = *(float *)(in_EAX + 0x50);
  }
  _DAT_0069c632 = 0xb;
  iVar15 = FUN_0051bd60();
  local_f8 = iVar15;
  local_100 = FUN_0051bdd0();
  if ((iVar15 == -1) || (local_100 == -1)) {
    _DAT_0069c632 = 0;
    return;
  }
  local_e4 = FUN_0051be40();
  local_ec = FUN_00511e80();
  sVar16 = *(short *)(in_EAX + 0xe);
  local_f0 = 1.0 / (float)(*(short *)(in_EAX + 0xc) + -1);
  sVar14 = 0;
  sVar19 = 0;
  local_e0 = 1.0 / (float)(sVar16 + -1);
  if (0 < *(short *)(in_EAX + 0xc)) {
    do {
      local_114 = 0;
      if (0 < sVar16) {
        local_f4 = (int)sVar19;
        do {
          sVar13 = (short)local_114;
          local_d0[0] = (int)sVar13;
          local_e8 = param_1 + (local_f4 * sVar16 + local_d0[0]) * 6 + 7;
          local_fc = local_f4;
          if (*(short *)(in_EAX + 0xc) + -1 <= local_f4) {
            local_fc = local_f4 + -1;
          }
          if (sVar16 + -1 <= local_d0[0]) {
            sVar13 = sVar13 + -1;
          }
          iVar15 = (int)(short)local_fc * (int)*(short *)(in_EAX + 0xe) + (int)sVar13;
          pfVar1 = (float *)(param_1 +
                            (((short)local_fc + 1) * (int)*(short *)(in_EAX + 0xe) + (int)sVar13) *
                            6 + 7);
          fVar4 = *pfVar1;
          fVar5 = (float)param_1[iVar15 * 6 + 7];
          fVar6 = pfVar1[1];
          fVar7 = (float)param_1[iVar15 * 6 + 8];
          fVar8 = pfVar1[2];
          fVar9 = (float)param_1[iVar15 * 6 + 9];
          local_10c = (float)param_1[iVar15 * 6 + 0xd] - (float)param_1[iVar15 * 6 + 7];
          local_108 = (float)param_1[iVar15 * 6 + 0xe] - (float)param_1[iVar15 * 6 + 8];
          fVar10 = (float)param_1[iVar15 * 6 + 0xf];
          fVar11 = (float)param_1[iVar15 * 6 + 9];
          puVar21 = (undefined4 *)(sVar14 * 0x44 + local_e4);
          pfVar1 = (float *)(puVar21 + 3);
          local_dc = (fVar10 - fVar11) * (fVar6 - fVar7) - (fVar8 - fVar9) * local_108;
          *pfVar1 = local_dc;
          local_d8 = (fVar8 - fVar9) * local_10c - (fVar10 - fVar11) * (fVar4 - fVar5);
          puVar21[4] = local_d8;
          local_d4 = local_108 * (fVar4 - fVar5) - (fVar6 - fVar7) * local_10c;
          puVar21[5] = local_d4;
          fVar4 = SQRT((float)puVar21[5] * (float)puVar21[5] +
                       (float)puVar21[4] * (float)puVar21[4] + *pfVar1 * *pfVar1);
          if (0.0001 <= ABS(fVar4)) {
            fVar4 = 1.0 / fVar4;
            *pfVar1 = fVar4 * *pfVar1;
            puVar21[4] = fVar4 * (float)puVar21[4];
            puVar21[5] = fVar4 * (float)puVar21[5];
          }
          *puVar21 = *local_e8;
          puVar21[1] = local_e8[1];
          uVar12 = local_e8[2];
          puVar21[0xc] = (float)local_f4 * local_f0;
          puVar21[2] = uVar12;
          *(undefined2 *)(puVar21 + 0xe) = 0;
          *(undefined2 *)((int)puVar21 + 0x3a) = 0;
          puVar21[0xd] = (float)local_d0[0] * local_e0;
          puVar21[0xf] = 0x3f000000;
          puVar21[0x10] = 0x3f000000;
          sVar16 = *(short *)(in_EAX + 0xe);
          local_114 = local_114 + 1;
          sVar14 = sVar14 + 1;
        } while ((short)local_114 < sVar16);
      }
      sVar19 = sVar19 + 1;
    } while (sVar19 < *(short *)(in_EAX + 0xc));
  }
  sVar16 = 0;
  sVar19 = 0;
  if (*(short *)(in_EAX + 0xc) != 1 && -1 < *(short *)(in_EAX + 0xc) + -1) {
    local_114 = 0;
    do {
      sVar14 = 0;
      iVar15 = *(short *)(in_EAX + 0xe) + -1;
      if (0 < iVar15) {
        local_118 = 0;
        do {
          switch(*(undefined2 *)((int)param_1 + (local_114 * iVar15 + local_118) * 2 + 0x1534)) {
          case 0:
            psVar18 = (short *)(local_ec + sVar19 * 6);
            *psVar18 = *(short *)(in_EAX + 0xe) * sVar16 + sVar14;
            psVar18[1] = *(short *)(in_EAX + 0xe) * (sVar16 + 1) + sVar14;
            sVar19 = sVar19 + 1;
            psVar18[2] = *(short *)(in_EAX + 0xe) * sVar16 + 1 + sVar14;
            psVar18 = (short *)(local_ec + sVar19 * 6);
            *psVar18 = *(short *)(in_EAX + 0xe) * sVar16 + 1 + sVar14;
            psVar18[1] = *(short *)(in_EAX + 0xe) * (sVar16 + 1) + sVar14;
            psVar18[2] = *(short *)(in_EAX + 0xe) * (sVar16 + 1) + 1 + sVar14;
            goto LAB_004fc7e7;
          default:
            goto switchD_004fc67f_caseD_1;
          case 2:
            psVar18 = (short *)(local_ec + sVar19 * 6);
            *psVar18 = *(short *)(in_EAX + 0xe) * sVar16 + sVar14;
            psVar18[1] = *(short *)(in_EAX + 0xe) * sVar16 + 1 + sVar14;
            sVar13 = (sVar16 + 1) * *(short *)(in_EAX + 0xe);
            break;
          case 3:
            psVar18 = (short *)(local_ec + sVar19 * 6);
            *psVar18 = *(short *)(in_EAX + 0xe) * sVar16 + sVar14;
            psVar18[1] = *(short *)(in_EAX + 0xe) * sVar16 + 1 + sVar14;
            sVar13 = (sVar16 + 1) * *(short *)(in_EAX + 0xe) + 1;
            break;
          case 4:
            sVar13 = *(short *)(in_EAX + 0xe) * sVar16;
            goto LAB_004fc7a6;
          case 5:
            sVar13 = *(short *)(in_EAX + 0xe) * sVar16 + 1;
LAB_004fc7a6:
            *(short *)(local_ec + sVar19 * 6) = sVar13 + sVar14;
            psVar18 = (short *)(local_ec + sVar19 * 6);
            psVar18[1] = *(short *)(in_EAX + 0xe) * (sVar16 + 1) + 1 + sVar14;
            sVar13 = *(short *)(in_EAX + 0xe) * (sVar16 + 1);
          }
          psVar18[2] = sVar13 + sVar14;
LAB_004fc7e7:
          sVar19 = sVar19 + 1;
switchD_004fc67f_caseD_1:
          sVar14 = sVar14 + 1;
          local_118 = (int)sVar14;
          iVar15 = *(short *)(in_EAX + 0xe) + -1;
        } while (local_118 < iVar15);
      }
      sVar16 = sVar16 + 1;
      local_114 = (uint)sVar16;
    } while ((int)local_114 < *(short *)(in_EAX + 0xc) + -1);
  }
  (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
  if ((&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iStack_104 * 0x10) * 3] != 0) {
    (**(code **)(**(int **)(&DAT_007bf04c +
                           (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iStack_104 * 0x10) * 3] * 10)
                + 0x30))
              (*(int **)(&DAT_007bf04c +
                        (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iStack_104 * 0x10) * 3] * 10));
  }
  iVar15 = *(int *)((local_114 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar17 = (int)*(short *)(in_EAX + 0xe);
  pfVar1 = (float *)(unaff_retaddr + 4 + iVar17 * 0x18);
  pfVar2 = (float *)(unaff_retaddr + 0x1c + iVar17 * (*(short *)(in_EAX + 0xc) + -1) * 0x18);
  pfVar3 = (float *)(unaff_retaddr + 4 + *(short *)(in_EAX + 0xc) * iVar17 * 0x18);
  local_110 = (*pfVar3 + *pfVar1 + *pfVar2 + *(float *)(unaff_retaddr + 0x1c)) * 0.25;
  local_10c = (pfVar3[1] + pfVar1[1] + pfVar2[1] + *(float *)(unaff_retaddr + 0x20)) * 0.25;
  fVar4 = pfVar3[2];
  fVar5 = pfVar1[2];
  fVar6 = pfVar2[2];
  fVar7 = *(float *)(unaff_retaddr + 0x24);
  piVar20 = local_d0;
  for (iVar17 = 0x33; iVar17 != 0; iVar17 = iVar17 + -1) {
    *piVar20 = 0;
    piVar20 = piVar20 + 1;
  }
  puStack_c8 = PTR_DAT_0069673c;
  local_d0[1] = 1;
  uStack_c4 = 1;
  puVar21 = auStack_c0;
  for (iVar17 = 0x1d; iVar17 != 0; iVar17 = iVar17 + -1) {
    *puVar21 = *param_1;
    param_1 = param_1 + 1;
    puVar21 = puVar21 + 1;
  }
  uStack_4c = *param_2;
  uStack_48 = param_2[1];
  local_108 = (fVar4 + fVar5 + fVar6 + fVar7) * 0.25;
  uStack_8 = 0x3f800000;
  uStack_c = 0x3f800000;
  fStack_1c = local_110;
  fStack_18 = local_10c;
  fStack_14 = local_108;
  if (DAT_006893ec != '\0') {
    DAT_0069c74c = 1;
    DAT_0071d1fa = 0;
    if (DAT_007c118c < 0xffff0101) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,1);
    }
  }
  FUN_00526f50(0);
  sVar16 = *(short *)(iVar15 + 0x24);
  if ((sVar16 == 1) || ((4 < sVar16 && (sVar16 < 0xc)))) {
    FUN_0052b180(iVar15,0,0,local_fc,(int)sVar19,0,iStack_104,&local_110);
  }
  else {
    FUN_0052b050(iVar15,0,0,local_fc,(int)sVar19,0);
  }
  FUN_0052b530();
  if ((DAT_006893ec != '\0') && (DAT_007c118c < 0xffff0101)) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
  }
  _DAT_0069c632 = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
