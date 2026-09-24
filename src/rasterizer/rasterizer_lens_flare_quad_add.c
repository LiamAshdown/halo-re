// rasterizer_lens_flare_quad_add  (Ghidra: rasterizer_lens_flare_quad_add, already named)
// address 0x537550, size 686 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: functions.md summary ("Builds and appends one screen-space sprite quad (lens flare /
//   decal), with optional rotation and non-uniform scale, into the appropriate material batch,
//   flushing the batch when full"); 6 vertices of 8 floats each (matching lens_flare_vertex
//   exactly) are written into the batch slot chosen by rasterizer_lens_flare_batch_find_slot,
//   using the same 0x6006-dword-stride indexing as the rest of this cluster.
// register convention: EAX -> scale (2 floats, x/y non-uniform scale, NULL means 1.0/1.0), EBX ->
//   diffuse (packed color), stack -> (position, radius, rotation_degrees).
// blam-cc: EAX -> scale, EBX -> diffuse, stack -> (position, radius, rotation_degrees)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern lens_flare_batch lens_flare_batches[k_lens_flare_batch_slots]; // 0x00746fc0
extern uint32_t lens_flare_vertex_specular; // 0x0069e708

extern int32_t rasterizer_lens_flare_batch_find_slot(void); // 0x536cb0
extern void rasterizer_lens_flare_batch_draw_slot(int32_t batch_index); // 0x536c10
extern uint8_t rasterizer_lens_flare_project_to_screen(const real_point3d *position, float radius,
    float *out_screen, float *out_inverse_w, float *out_billboard_size); // 0x536d80
extern double fcos(double x); // FCOS
extern double fsin(double x); // FSIN

// Builds and appends one screen-space sprite quad (lens flare / decal), with optional rotation and
// non-uniform scale, into the appropriate material batch, flushing the batch when full.
void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position,
                                    float radius, float rotation_degrees)
{
    float screen[3];
    float inverse_w;
    float billboard[2];

    if (radius <= 0.0f) {
        return;
    }
    if (!rasterizer_lens_flare_project_to_screen(position, radius, screen, &inverse_w, billboard)) {
        return;
    }

    {
        float axis_u, axis_v;
        float scale_x, scale_y;
        int32_t slot, count;
        lens_flare_vertex *v;

        if (rotation_degrees == 0.0f) {
            axis_u = billboard[0];
            axis_v = billboard[1];
        } else {
            double angle = (double)rotation_degrees * 0.017453292;
            double c = fcos(angle);
            double s = fsin(angle);
            axis_u = (float)((double)billboard[0] * c - (double)billboard[1] * s);
            axis_v = (float)((double)billboard[1] * c + (double)billboard[0] * s);
        }

        if (scale == 0) {
            scale_x = 1.0f;
            scale_y = 1.0f;
        } else {
            scale_x = scale[0];
            scale_y = scale[1];
        }

        slot = rasterizer_lens_flare_batch_find_slot();
        count = lens_flare_batches[slot].vertex_count;
        v = &lens_flare_batches[slot].vertices[count];

        {
            // Four billboard corners built from two (possibly rotated) basis axes; two triangles
            // sharing the corner1-corner3 diagonal (corner1==vertex3, corner3==vertex4 below).
            float corner1_x = screen[0] - scale_x * axis_u, corner1_y = screen[1] - scale_y * axis_v;
            float corner2_x = screen[0] + scale_x * axis_v, corner2_y = screen[1] - scale_y * axis_u;
            float corner3_x = screen[0] + scale_x * axis_u, corner3_y = screen[1] + scale_y * axis_v;
            float corner4_x = screen[0] - scale_x * axis_v, corner4_y = screen[1] + scale_y * axis_u;

            v[0].x = corner1_x; v[0].y = corner1_y; v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].x = corner2_x; v[1].y = corner2_y; v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].x = corner3_x; v[2].y = corner3_y; v[2].u = 1.0f; v[2].v = 1.0f;
            v[3].x = corner1_x; v[3].y = corner1_y; v[3].u = 0.0f; v[3].v = 0.0f;
            v[4].x = corner3_x; v[4].y = corner3_y; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].x = corner4_x; v[5].y = corner4_y; v[5].u = 0.0f; v[5].v = 1.0f;

            {
                int i;
                for (i = 0; i < 6; i++) {
                    v[i].z = screen[2];
                    v[i].rhw = inverse_w;
                    v[i].diffuse = diffuse;
                    v[i].specular = lens_flare_vertex_specular;
                }
            }
        }

        lens_flare_batches[slot].vertex_count = count + 6;
        if (count + 6 == k_lens_flare_batch_vertices) {
            rasterizer_lens_flare_batch_draw_slot(slot);
        }
    }
}

#if 0
Original Ghidra decompilation (0x537550):

void rasterizer_lens_flare_quad_add(undefined4 param_1,float param_2,float param_3)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  char cVar8;
  float *in_EAX;
  int iVar9;
  float unaff_EBX;
  float10 fVar10;
  float10 fVar11;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if (0.0 < param_2) {
    cVar8 = rasterizer_lens_flare_project_to_screen(param_1,param_2,&local_c,&param_2,&local_14);
    if (cVar8 != '\0') {
      if (param_3 == 0.0) {
        local_20 = local_14;
        local_28 = local_10;
      }
      else {
        fVar10 = (float10)fcos((float10)param_3 * (float10)0.017453292);
        fVar11 = (float10)fsin((float10)param_3 * (float10)0.017453292);
        local_20 = (float)((float10)local_14 * fVar10 - (float10)local_10 * fVar11);
        local_28 = (float)((float10)local_10 * fVar10 + (float10)local_14 * fVar11);
      }
      if (in_EAX == (float *)0x0) {
        local_24 = 1.0;
        local_2c = 1.0;
      }
      else {
        local_2c = *in_EAX;
        local_24 = in_EAX[1];
      }
      local_14 = (float)FUN_00536cb0();
      fVar7 = DAT_0069e708;
      iVar9 = (int)local_14 * 0x18018;
      fVar3 = local_c - local_2c * local_20;
      iVar2 = (&DAT_0075efc0)[(int)local_14 * 0x6006];
      pfVar1 = (float *)(&DAT_00746fc0 + iVar9 + iVar2 * 0x20);
      *pfVar1 = fVar3;
      pfVar1[6] = 0.0;
      pfVar1[7] = 0.0;
      pfVar1[0xf] = 0.0;
      pfVar1[2] = local_4;
      fVar4 = local_8 - local_24 * local_28;
      pfVar1[3] = param_2;
      pfVar1[10] = local_4;
      pfVar1[1] = fVar4;
      pfVar1[0xb] = param_2;
      pfVar1[4] = unaff_EBX;
      pfVar1[5] = fVar7;
      pfVar1[0xc] = unaff_EBX;
      pfVar1[0xd] = fVar7;
      pfVar1[0xe] = 1.0;
      pfVar1[0x14] = unaff_EBX;
      pfVar1[8] = local_2c * local_28 + local_c;
      pfVar1[0x15] = fVar7;
      pfVar1[0x16] = 1.0;
      pfVar1[0x17] = 1.0;
      pfVar1[0x1c] = unaff_EBX;
      pfVar1[0x1d] = fVar7;
      pfVar1[9] = local_8 - local_24 * local_20;
      fVar5 = local_2c * local_20 + local_c;
      *(float *)(&DAT_00746fc0 + (iVar2 + 2) * 0x20 + iVar9) = fVar5;
      pfVar1[0x12] = local_4;
      fVar6 = local_24 * local_28 + local_8;
      pfVar1[0x13] = param_2;
      pfVar1[0x11] = fVar6;
      *(float *)(&DAT_00746fc0 + (iVar2 + 3) * 0x20 + iVar9) = fVar3;
      pfVar1[0x1a] = local_4;
      pfVar1[0x19] = fVar4;
      pfVar1[0x1b] = param_2;
      pfVar1[0x1e] = 0.0;
      pfVar1[0x1f] = 0.0;
      *(float *)(&DAT_00746fc0 + (iVar2 + 4) * 0x20 + iVar9) = fVar5;
      pfVar1[0x21] = fVar6;
      pfVar1[0x22] = local_4;
      pfVar1[0x23] = param_2;
      *(float *)(&DAT_00746fc0 + (iVar2 + 5) * 0x20 + iVar9) = local_c - local_2c * local_28;
      pfVar1[0x2a] = local_4;
      pfVar1[0x25] = fVar7;
      pfVar1[0x2d] = fVar7;
      pfVar1[0x29] = local_24 * local_20 + local_8;
      pfVar1[0x2b] = param_2;
      pfVar1[0x24] = unaff_EBX;
      pfVar1[0x26] = 1.0;
      pfVar1[0x27] = 1.0;
      pfVar1[0x2c] = unaff_EBX;
      pfVar1[0x2e] = 0.0;
      pfVar1[0x2f] = 1.0;
      (&DAT_0075efc0)[(int)local_14 * 0x6006] = iVar2 + 6;
      if (iVar2 + 6 == 0xc00) {
        FUN_00536c10();
      }
    }
  }
  return;
}
#endif
