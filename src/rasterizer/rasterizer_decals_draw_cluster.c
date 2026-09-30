// rasterizer_decals_draw_cluster  (Ghidra: FUN_0051aa50, unnamed)
// address 0x51aa50, size 1636 bytes (0x51aa50..0x51b0b3; Ghidra said 826 bytes, the body really runs to 0x51b0b2 and owns the jump table at
// 0x51b0b4; Ghidra split two of its switch cases off as the fake functions 0x51acd0 and 0x51ad91)
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x51aa50..0x51b0b2 (loop, blend switch head, texture rebind, constants, declaration/shader tables, DrawPrimitive(list, (offset>>4)*1.5, blocks*2)).)
// evidence: walks the singly linked decal list of one (decal layer, cluster) bucket: the head is
//   0x006b0ad8[layer * 0x200 + cluster], every link is a datum in decal_data (0x0087abe4, 0x38
//   byte records, next at +0x34). Per decal it switches the framebuffer blend function when the
//   decal definition's type (definition +0xc0) changes, sets the texture stage states for that
//   type (switch through 0x51b0b4: 0 -> 0x51ad91, 1/5 -> 0x51ac80, 2 -> 0x51acd7, 3/4/6 ->
//   0x51ab8b, 7 -> 0x51ae36), rebinds the bitmap when definition +0xe4 or the frame byte +0x1b
//   changes, uploads the decal color as vertex shader c10 and draws its vertices out of the
//   decal vertex cache as a triangle list with vertex shader 2 and the decal declaration (type
//   10). Any failed device call clears a latch that stops further draws but not the blend
//   mode tracking.
//   Ghidra dropped the vertex cache lookup (0x0071d1c0) and the first vertex computation as
//   unreachable, and passed the wrong register to the texture bind; this follows the raw code.
// register convention: __cdecl, cluster index (int16) on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "fn_rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern data_array *decal_data;                                      // 0x0087abe4
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern uint32_t *decal_grid_block;                               // 0x006b0ad8 [layer * 0x200 + cluster],
                                                                    //   decal datum index, -1 for none
extern uint8_t *rasterizer_decal_vertex_cache_handle;               // 0x0071d1c0 UNSURE: +0x2c shift, +0x3c data_array
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t decals_for_all_responses;                         // 0x006893f5
extern int16_t rasterizer_decal_layer;                              // 0x006d98dc set by rasterizer_decal_pass_begin
extern int16_t rasterizer_decal_blend_mode;                         // 0x006d98d8 last framebuffer blend function
extern uint32_t rasterizer_decal_bitmap_tag;                        // 0x006d98e0 last bound decal bitmap
extern int16_t rasterizer_decal_bitmap_frame;                       // 0x006d98e4 last bound decal frame

// blam-cc: CX -> mode
// blam-cc: CX -> mode

// blam-cc: EAX -> bitmap_tag_id, the rest on the stack

// __ftol (0x6391b4, input on the FPU stack, chops toward zero) is written as a (long long) cast below

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static void rasterizer_set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

void rasterizer_decals_draw_cluster(int16_t cluster_index)
{
    uint8_t succeeded = 1;
    uint8_t layer_enabled = 1;
    uint32_t decal_index;

    if (decals_for_all_responses == 0 && rasterizer_decal_layer != 3) {
        layer_enabled = 0;
    }
    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || !layer_enabled) {
        return;
    }

    decal_index = decal_grid_block[rasterizer_decal_layer * 0x200 + cluster_index];
    while (decal_index != 0xffffffff) {
        uint8_t *decal = (uint8_t *)decal_data->data + (decal_index & 0xffff) * 0x38;
        uint32_t definition_tag = *(uint32_t *)&((struct decal *)decal)->definition_index;
        uint8_t *definition = (uint8_t *)tag_instances[definition_tag & 0xffff].data + 0xbc;
        int16_t type = *(int16_t *)(definition + 4);

        if (rasterizer_decal_blend_mode != type) {
            rasterizer_decal_blend_mode = type;
            if (type == 1 || type == 2) {
                rasterizer_set_render_state(0xa8, 0xf);         // D3DRS_COLORWRITEENABLE rgba
            } else {
                rasterizer_set_render_state(0xa8, 7);           // rgb
            }
            chimera__rasterizer_set_framebuffer_blend_function(rasterizer_decal_blend_mode);
        }

        if (succeeded) {
            uint8_t *cache = rasterizer_decal_vertex_cache_handle;
            data_array *blocks = *(data_array **)(cache + 0x3c);
            uint32_t first_offset = *(uint32_t *)((uint8_t *)blocks->data + (decal_index & 0xffff) * 0x1c + 8)
                                    << (*(uint32_t *)(cache + 0x2c) & 0x1f);
            uint32_t color = ((struct decal *)decal)->color;
            uint32_t alpha = (((struct decal *)decal)->alpha * (color >> 24) + 0x7f) >> 8;
            int32_t primitive_count;
            int32_t first_vertex;
            int8_t frame;
            float constants[4];

            switch (rasterizer_decal_blend_mode) {
            case 0:                                             // 0x51ad91
                rasterizer_set_texture_stage_state(0, 1, 4);
                rasterizer_set_texture_stage_state(0, 2, 2);
                rasterizer_set_texture_stage_state(0, 3, 0);
                rasterizer_set_texture_stage_state(0, 4, 4);
                rasterizer_set_texture_stage_state(0, 5, 2);
                rasterizer_set_texture_stage_state(0, 6, 0x10);
                rasterizer_set_texture_stage_state(1, 1, 1);
                rasterizer_set_texture_stage_state(1, 4, 1);
                break;
            case 1:                                             // 0x51ac80
            case 5:
                rasterizer_set_texture_stage_state(0, 1, 4);
                rasterizer_set_texture_stage_state(0, 2, 2);
                rasterizer_set_texture_stage_state(0, 3, 0);
                rasterizer_set_texture_stage_state(0, 4, 2);
                rasterizer_set_texture_stage_state(0, 5, 2);    // 0x51acd0: push 2; jmp 0x51ae89
                rasterizer_set_texture_stage_state(1, 1, 1);
                rasterizer_set_texture_stage_state(1, 4, 1);
                break;
            case 2:                                             // 0x51acd7
                rasterizer_set_render_state(0x3c, 0x7f7f7f7f);  // D3DRS_TEXTUREFACTOR
                rasterizer_set_texture_stage_state(0, 1, 0x19);
                rasterizer_set_texture_stage_state(0, 2, 2);
                rasterizer_set_texture_stage_state(0, 3, 1);
                rasterizer_set_texture_stage_state(0, 0x1a, 0x10);
                rasterizer_set_texture_stage_state(0, 4, 2);
                rasterizer_set_texture_stage_state(0, 5, 3);
                rasterizer_set_texture_stage_state(1, 1, 1);
                rasterizer_set_texture_stage_state(1, 4, 1);
                break;
            case 3:                                             // 0x51ab8b
            case 4:
            case 6:
                rasterizer_set_texture_stage_state(0, 1, 4);
                rasterizer_set_texture_stage_state(0, 2, 2);
                rasterizer_set_texture_stage_state(0, 3, 0);
                rasterizer_set_texture_stage_state(0, 4, 2);
                rasterizer_set_texture_stage_state(0, 5, 1);
                rasterizer_set_texture_stage_state(1, 1, 4);
                rasterizer_set_texture_stage_state(1, 2, 1);
                rasterizer_set_texture_stage_state(1, 3, 0x30);
                rasterizer_set_texture_stage_state(1, 4, 2);
                rasterizer_set_texture_stage_state(1, 5, 1);
                rasterizer_set_texture_stage_state(2, 4, 1);
                rasterizer_set_texture_stage_state(2, 1, 1);
                break;
            case 7:                                             // 0x51ae36
                rasterizer_set_render_state(0x3c, 0xffff0000);  // D3DRS_TEXTUREFACTOR
                rasterizer_set_texture_stage_state(0, 1, 2);
                rasterizer_set_texture_stage_state(0, 2, 3);
                rasterizer_set_texture_stage_state(0, 4, 2);
                rasterizer_set_texture_stage_state(0, 5, 3);
                rasterizer_set_texture_stage_state(1, 1, 1);
                rasterizer_set_texture_stage_state(1, 4, 1);
                break;
            default:
                break;
            }

            // quads were expanded to six vertices each: first vertex = (offset / 16) * 1.5
            primitive_count = ((struct decal *)decal)->triangle_count * 2;
            first_vertex = (int32_t)(long long)((double)(first_offset >> 4) * 1.5);

            frame = *(int8_t *)&((struct decal *)decal)->unknown_1b;
            if (rasterizer_decal_bitmap_tag != *(uint32_t *)(definition + 0x28) ||
                rasterizer_decal_bitmap_frame != (int16_t)frame) {
                rasterizer_decal_bitmap_tag = *(uint32_t *)(definition + 0x28);
                rasterizer_decal_bitmap_frame = (int16_t)frame;
                chimera__rasterizer_set_texture(rasterizer_decal_bitmap_tag, 0, 0, 1, rasterizer_decal_bitmap_frame);
            }

            // 0x51af2a..0x51afc8: fild, fmul QWORD 0x673200 (1/255 as a double), fstp float
            constants[0] = (float)((double)((color >> 16) & 0xff) * (1.0 / 255.0));
            constants[1] = (float)((double)((color >> 8) & 0xff) * (1.0 / 255.0));
            constants[2] = (float)((double)(color & 0xff) * (1.0 / 255.0));
            constants[3] = (float)((double)(uint32_t)(0xff - alpha) * (1.0 / 255.0));
            if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, constants, 1) < 0) {
                succeeded = 0;
            }
            if (((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(
                    rasterizer_device, (void *)rasterizer_vertex_declarations[_rasterizer_vertex_type_decal].declaration) < 0) {
                succeeded = 0;
            }
            if (((d3d_call1_fn)device_vtable()[0x134 / 4])(
                    rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                        rasterizer_vertex_declarations[_rasterizer_vertex_type_decal].usage) & 0x10) < 0) {
                succeeded = 0;
            }
            if (((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                                 (void *)rasterizer_vertex_shaders[2].shader) < 0) {
                succeeded = 0;
            }
            if (((d3d_set_pointer_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0) < 0) {
                succeeded = 0;
            }
            if (((d3d_call3_fn)device_vtable()[0x144 / 4])(rasterizer_device, 4, (uint32_t)first_vertex,
                                                           (uint32_t)primitive_count) < 0) {    // DrawPrimitive list
                succeeded = 0;
            }
            if (((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing) < 0) {
                succeeded = 0;
            }
        }
        decal_index = *(uint32_t *)&((struct decal *)decal)->next_decal;
    }
}

#if 0
Original Ghidra decompilation (0x51aa50):

/* WARNING: Removing unreachable block (ram,0x0051af45) */
/* WARNING: Removing unreachable block (ram,0x0051aed8) */
/* WARNING: Removing unreachable block (ram,0x0051af6c) */
/* WARNING: Removing unreachable block (ram,0x0051af8d) */

void FUN_0051aa50(short param_1)

{
  byte bVar1;
  short sVar2;
  uint uVar3;
  bool bVar4;
  bool bVar5;
  undefined4 uVar6;
  int iVar7;
  int iVar8;
  float fStack_10;
  float fStack_c;
  float fStack_8;
  float fStack_4;
  
  bVar5 = true;
  bVar4 = true;
  if ((DAT_006893f5 == '\0') && (DAT_006d98dc != 3)) {
    bVar4 = false;
  }
  if ((DAT_006893e4 == 0) && (bVar4)) {
    uVar3 = *(uint *)(DAT_006b0ad8 + (DAT_006d98dc * 0x200 + (int)param_1) * 4);
    sVar2 = DAT_006d98d8;
    while (DAT_006d98d8 = sVar2, uVar3 != 0xffffffff) {
      iVar8 = (uVar3 & 0xffff) * 0x38 + *(int *)(DAT_0087abe4 + 0x34);
      iVar7 = *(int *)((*(uint *)(iVar8 + 0x2c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      DAT_006d98d8 = *(short *)(iVar7 + 0xc0);
      if (sVar2 != DAT_006d98d8) {
        if ((DAT_006d98d8 == 1) || (DAT_006d98d8 == 2)) {
          (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,0xf);
        }
        else {
          (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
        }
        chimera__rasterizer_set_framebuffer_blend_function();
        sVar2 = DAT_006d98d8;
      }
      DAT_006d98d8 = sVar2;
      if (!bVar5) goto LAB_0051b09f;
      uVar3 = *(uint *)(iVar8 + 0x24);
      bVar1 = *(byte *)(iVar8 + 0x28);
      switch(DAT_006d98d8) {
      case 0:
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0x10);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
        break;
      case 1:
      case 5:
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
        uVar6 = 2;
        goto LAB_0051ae89;
      case 2:
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0x7f7f7f7f);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,0x19);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0x1a,0x10);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,3);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
        break;
      case 3:
      case 4:
      case 6:
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,2,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,3,0x30);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,5,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,4,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,1,1);
        break;
      case 7:
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0xffff0000);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,3);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
        uVar6 = 3;
LAB_0051ae89:
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,uVar6);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      }
      sVar2 = *(short *)(iVar8 + 0x2a);
      uVar6 = __ftol();
      iVar7 = *(int *)(iVar7 + 0xe4);
      if ((DAT_006d98e0 != iVar7) || (DAT_006d98e4 != *(char *)(iVar8 + 0x1b))) {
        DAT_006d98e4 = (short)*(char *)(iVar8 + 0x1b);
        DAT_006d98e0 = iVar7;
        chimera__rasterizer_set_texture(0,0,1,DAT_006d98e4);
      }
      fStack_10 = (float)(uVar3 >> 0x10 & 0xff) * 0.003921569;
      fStack_c = (float)(uVar3 >> 8 & 0xff) * 0.003921569;
      fStack_8 = (float)(uVar3 & 0xff) * 0.003921569;
      iVar7 = 0xff - ((uint)bVar1 * (uVar3 >> 0x18) + 0x7f >> 8);
      fStack_4 = (float)iVar7;
      if (iVar7 < 0) {
        fStack_4 = fStack_4 + 4.2949673e+09;
      }
      fStack_4 = fStack_4 * 0.003921569;
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&fStack_10,1);
      (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1b08);
      (**(code **)(*DAT_0071d174 + 0x134))
                (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1b10 & 0x10);
      (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e360);
      (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
      (**(code **)(*DAT_0071d174 + 0x144))(DAT_0071d174,4,uVar6,(int)sVar2 << 1);
      iVar7 = (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
      if (iVar7 < 0) {
        bVar5 = false;
      }
LAB_0051b09f:
      sVar2 = DAT_006d98d8;
      uVar3 = *(uint *)(iVar8 + 0x34);
    }
  }
  return;
}
#endif
