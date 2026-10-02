// rasterizer_lens_flare_occlusion_test_issue  (Ghidra: FUN_00537800)
// address 0x537800, size 826 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Only caller is the thunk 0x512190 (called from
//   0x513cc5), which pushes (ECX, its own stack argument) and leaves EDI as the query slot.
//   The phase 3 file read the callee-saved push of ESI at 0x53799c as an argument and
//   transcribed the probe vertices by stack slot; laid out as rasterizer_screen_vertex the
//   quad is ordinary: the four corners of the rounded screen rectangle at the projected depth
//   and rhw, white, with 0..1 texture coordinates. 0x623e40 is the CRT floor (x87 path runs
//   under control word 0x173f, round down, and reports error code 11, the floor opcode).
// What it does: projects position / radius with rasterizer_lens_flare_project_to_screen, pads
//   the half extent to at least one pixel, floors the rectangle corners (each clamped to
//   +-32767), and returns the pixel area (0 when negative). When occlusion queries exist and
//   the area is positive, draws the rectangle as a triangle fan between Issue(BEGIN) and
//   Issue(END) of the query of this slot. Returns 1 when the debug toggle 0x00689424 is off,
//   0 for a slot past the 1024 queries or a failed projection, and 4 when queries are
//   unsupported.
// register convention: EDI -> slot_index, stack -> (position, radius).
// blam-cc: EDI -> slot_index, stack -> (position, radius)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t console_debug_toggle_689424;                                // 0x00689424
extern uint8_t lens_flare_occlusion_queries_supported;                     // 0x006e1dc0
extern void *lens_flare_occlusion_queries[k_lens_flare_occlusion_queries]; // 0x006e1dc8
extern void *rasterizer_device;                                            // 0x0071d174

extern uint8_t rasterizer_lens_flare_project_to_screen(const real_point3d *position, float radius, float *out_screen, float *out_inverse_w, float *out_billboard_size); // 0x00536d80
extern double floor(double x); // 0x623e40 CRT

typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t primitive_type, uint32_t primitive_count,
                                            const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3d_query_issue_fn)(void *query, uint32_t flags);

static int16_t floor_clamped(float value)
{
    if (value < -32767.0f) {           // test ah,5 / jp: NaN falls through to the upper test
        value = -32767.0f;
    } else if (value > 32767.0f) {
        value = 32767.0f;
    }
    return (int16_t)(int32_t)(float)floor((double)value);
}

static void set_vertex(rasterizer_screen_vertex *vertex, int16_t x, int16_t y, float z, float rhw, float u, float v)
{
    vertex->x = (float)x;
    vertex->y = (float)y;
    vertex->z = z;
    vertex->rhw = rhw;
    vertex->diffuse = 0xffffffff;
    vertex->u = u;
    vertex->v = v;
}

int32_t rasterizer_lens_flare_occlusion_test_issue(int32_t slot_index, const real_point3d *position, float radius)
{
    float screen[3];        // x, y, depth
    float inverse_w;
    float half_size[2];
    int16_t x0, y0, x1, y1;
    int32_t area;

    if (console_debug_toggle_689424 == 0) {
        return 1;
    }
    if (slot_index >= k_lens_flare_occlusion_queries) {
        return 0;
    }
    if ((uint8_t)rasterizer_lens_flare_project_to_screen(position, radius, screen, &inverse_w, half_size) == 0) {
        return 0;
    }
    if (1.0f > half_size[0]) {
        half_size[0] = 1.0f;
    }
    if (1.0f > half_size[1]) {
        half_size[1] = 1.0f;
    }
    x0 = floor_clamped(screen[0] - half_size[0]);
    y0 = floor_clamped(screen[1] - half_size[1]);
    x1 = floor_clamped(screen[0] + half_size[0]);
    y1 = floor_clamped(screen[1] + half_size[1]);
    area = ((int32_t)x1 - (int32_t)x0) * ((int32_t)y1 - (int32_t)y0);
    if (area < 0) {
        area = 0;
    }
    if (lens_flare_occlusion_queries_supported == 0) {
        return 4;
    }
    if (area > 0 && lens_flare_occlusion_queries[slot_index] != 0) {
        void *query = lens_flare_occlusion_queries[slot_index];
        rasterizer_screen_vertex quad[4];

        ((d3d_query_issue_fn)(*(void ***)query)[0x18 / 4])(query, 2);    // D3DISSUE_BEGIN
        set_vertex(&quad[0], x0, y0, screen[2], inverse_w, 0.0f, 0.0f);
        set_vertex(&quad[1], x1, y0, screen[2], inverse_w, 1.0f, 0.0f);
        set_vertex(&quad[2], x1, y1, screen[2], inverse_w, 1.0f, 1.0f);
        set_vertex(&quad[3], x0, y1, screen[2], inverse_w, 0.0f, 1.0f);
        ((d3d_draw_primitive_up_fn)(*(void ***)rasterizer_device)[0x14c / 4])(rasterizer_device, 6, 2, quad,
                                                                            sizeof(rasterizer_screen_vertex));
        query = lens_flare_occlusion_queries[slot_index];
        ((d3d_query_issue_fn)(*(void ***)query)[0x18 / 4])(query, 1);    // D3DISSUE_END
    }
    return area;
}

#if 0
Original Ghidra decompilation (0x537800): phase 3 body replaced from raw disassembly

uint FUN_00537800(undefined4 param_1,undefined4 param_2)

{
  int *piVar1;
  float fVar2;
  char cVar3;
  uint uVar4;
  int unaff_ESI;
  int unaff_EDI;
  float10 fVar5;
  float local_94;
  float local_90;
  int local_8c;
  int local_88;
  int local_84;
  int local_80;
  float local_7c;
  float local_78;
  float fStack_74;
  float fStack_70;
  float fStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  undefined4 uStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;
  float fStack_50;
  undefined4 uStack_4c;
  undefined4 uStack_48;
  undefined4 uStack_44;
  float fStack_40;
  float fStack_3c;
  float fStack_38;
  float fStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  undefined4 uStack_c;
  
  if (DAT_00689424 == '\0') {
    return 1;
  }
  if (unaff_EDI < 0x400) {
    cVar3 = rasterizer_lens_flare_project_to_screen(param_1,param_2,&local_7c,&local_8c,&local_94);
    if (cVar3 != '\0') {
      if (local_94 < 1.0) {
        local_94 = 1.0;
      }
      if (local_90 < 1.0) {
        local_90 = 1.0;
      }
      fVar2 = local_7c - local_94;
      if (-32767.0 <= fVar2) {
        if (32767.0 < fVar2) {
          fVar2 = 32767.0;
        }
      }
      else {
        fVar2 = -32767.0;
      }
      fVar5 = (float10)FUN_00623e40((double)fVar2);
      local_84 = (int)ROUND((float)fVar5);
      fVar2 = local_78 - local_90;
      if (-32767.0 <= fVar2) {
        if (32767.0 < fVar2) {
          fVar2 = 32767.0;
        }
      }
      else {
        fVar2 = -32767.0;
      }
      fVar5 = (float10)FUN_00623e40((double)fVar2);
      local_88 = (int)ROUND((float)fVar5);
      fVar2 = local_7c + local_94;
      if (-32767.0 <= fVar2) {
        if (32767.0 < fVar2) {
          fVar2 = 32767.0;
        }
      }
      else {
        fVar2 = -32767.0;
      }
      fVar5 = (float10)FUN_00623e40((double)fVar2);
      local_80 = (int)ROUND((float)fVar5);
      local_78 = local_78 + local_90;
      if (-32767.0 <= local_78) {
        if (32767.0 < local_78) {
          local_78 = 32767.0;
        }
      }
      else {
        local_78 = -32767.0;
      }
      fVar5 = (float10)FUN_00623e40((double)local_78);
      local_94._0_2_ = (short)(int)ROUND((float)fVar5);
      local_94 = (float)(int)local_94._0_2_;
      local_88 = (int)(short)local_88;
      local_80 = (int)(short)local_80;
      local_84 = (int)(short)local_84;
      uVar4 = (local_80 - local_84) * ((int)local_94 - local_88);
      uVar4 = ((int)uVar4 < 0) - 1 & uVar4;
      if (DAT_006e1dc0 != '\0') {
        if ((0 < (int)uVar4) && (piVar1 = (int *)(&DAT_006e1dc8)[unaff_EDI], piVar1 != (int *)0x0))
        {
          (**(code **)(*piVar1 + 0x18))(piVar1,2);
          local_78 = (float)local_8c;
          uStack_14 = 0xffffffff;
          fStack_74 = (float)(int)local_90;
          uStack_30 = 0xffffffff;
          uStack_4c = 0xffffffff;
          uStack_68 = 0xffffffff;
          fStack_5c = (float)local_88;
          fStack_38 = local_7c;
          fStack_1c = local_7c;
          fStack_70 = local_7c;
          fStack_50 = local_94;
          fStack_3c = (float)unaff_ESI;
          fStack_54 = local_7c;
          fStack_18 = local_94;
          fStack_6c = local_94;
          uStack_64 = 0;
          uStack_60 = 0;
          uStack_48 = 0x3f800000;
          uStack_44 = 0;
          uStack_2c = 0x3f800000;
          uStack_28 = 0x3f800000;
          uStack_10 = 0;
          uStack_c = 0x3f800000;
          fStack_34 = local_94;
          fStack_58 = fStack_74;
          fStack_40 = fStack_5c;
          fStack_24 = local_78;
          fStack_20 = fStack_3c;
          (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&local_78,0x1c);
          (**(code **)(*(int *)(&DAT_006e1dc8)[unaff_EDI] + 0x18))
                    ((int *)(&DAT_006e1dc8)[unaff_EDI],1);
        }
        return uVar4;
      }
      return 4;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
