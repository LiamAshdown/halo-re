// rasterizer_glass_tint_draw_fixed_function  (Ghidra: FUN_00523980)
// address 0x523980, size 521 bytes
// phase 4 review: renamed from the rasterizer_decal_draw_* family. The procedure table
//   0x007c0480 holds ShaderTransparentGlass draw procedures: the shader fields these functions
//   read are glass fields (background_tint_map +0x70, diffuse_map +0x164, diffuse_detail_map
//   +0x178, reflection_map +0xb8, bump_map +0xcc; types/tags.h), not decal fields.
// name confidence: 0.4   rewrite confidence: 0.9
// VERIFIED against disassembly 0x523980..0x523b89 (2026-09-30). FIXED: the draft packed group->tint (0x88); the binary packs the
//   glass shader's background_tint_color (+0x54..0x5c) with alpha = blend_factor (mode 1) or 1.0. Everything else (vertex type 4
//   delegation, set_texture args, the render/texture stage states and the draw_vertices(ECX group, 0) call) matched.
// evidence: out/phase2/results/rasterizer_01.json ("Draws a decal using a simplified combiner
// setup for the pre-1.1 capability path, falling back to the newer renderer for material index
// 4."); selected by rasterizer_glass_draw_procedures_select.c for pixel_shader_version <
// ps_1_1. `param_1` is a transparent_geometry_group* (vertex_buffer at +0x58, dynamic_vertex_slot
// at +0x54, shader_permutation at +0x10, matching "passed as the bitmap index to set_texture"
// per its own header comment). Vertex type 4 (model_uncompressed) delegates to
// rasterizer_glass_tint_draw (0x522930) regardless of capability. The four `__ftol` calls
// pack the glass shader's background tint color and the group's blend factor into the 0xAARRGGBB D3DCOLOR for render state
// 0x3c (D3DRS_TEXTUREFACTOR); see the block in the function.
// register convention: param_1 on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern void *rasterizer_device;          // 0x0071d174

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
extern void rasterizer_glass_tint_draw(transparent_geometry_group *group); // 0x522930
// blam-cc: ECX -> group
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x00533660

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_glass_tint_draw_fixed_function(transparent_geometry_group *group)
{
    int16_t vertex_type;
    void **vtable;
    d3d_call1_fn set_vertex_declaration;
    d3d_call1_fn set_vertex_shader;
    d3d_call1_fn set_pixel_shader;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_texture_stage_state;
    uint32_t decal_color;
    uint32_t stage4_arg, stage5_arg;

    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot == -1) {
            goto fallback;
        }
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    } else {
        vertex_type = *(int16_t *)(void *)group->vertex_buffer;
    }

    if (vertex_type == 4) {
        rasterizer_glass_tint_draw(group);
        return;
    }

fallback:
    chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)group->shader + 0x70), 0, 0, 1, (int16_t)group->shader_permutation);

    vtable = *(void ***)rasterizer_device;
    set_vertex_declaration = (d3d_call1_fn)vtable[0x57]; // +0x15c
    set_vertex_declaration(rasterizer_device, rasterizer_vertex_declarations[12].declaration);

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader = (d3d_call1_fn)vtable[0x5c]; // +0x170
    set_vertex_shader(rasterizer_device, 0);

    vtable = *(void ***)rasterizer_device;
    set_pixel_shader = (d3d_call1_fn)vtable[0x6b]; // +0x1ac
    set_pixel_shader(rasterizer_device, 0);

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x13, 1);
    set_render_state(rasterizer_device, 0x14, 3);
    set_render_state(rasterizer_device, 0xf, 1);

    // 0x523a3d..0x523aad: the factor is the group's blend_factor (+0x18) in mode 1, else 1.0; the color is the glass shader's
    //   background tint color (shader +0x54/+0x58/+0x5c). D3DCOLOR = (factor * 255) << 24 | r << 16 | g << 8 | b, each
    //   channel truncated by __ftol; the first ftol is masked to a byte before the alpha is or'ed in.
    {
        ShaderTransparentGlass *glass = (ShaderTransparentGlass *)(void *)group->shader;
        double factor = (group->parameters.mode == 1) ? (double)group->parameters.blend_factor : 1.0;

        decal_color = (uint32_t)(int32_t)((double)glass->background_tint_color.red * 255.0) & 0xff;
        decal_color |= (uint32_t)(int32_t)(factor * 255.0) << 8;
        decal_color <<= 8;
        decal_color |= (uint32_t)(int32_t)((double)glass->background_tint_color.green * 255.0) & 0xff;
        decal_color <<= 8;
        decal_color |= (uint32_t)(int32_t)((double)glass->background_tint_color.blue * 255.0) & 0xff;
    }
    set_render_state(rasterizer_device, 0x3c, decal_color);

    vtable = *(void ***)rasterizer_device;
    set_texture_stage_state = (d3d_call3_fn)vtable[0x43]; // +0x10c
    set_texture_stage_state(rasterizer_device, 0, 1, 4);
    set_texture_stage_state(rasterizer_device, 0, 2, 2);
    set_texture_stage_state(rasterizer_device, 0, 3, 3);

    if (group->parameters.mode == 1) {
        set_texture_stage_state(rasterizer_device, 0, 4, 4);
        set_texture_stage_state(rasterizer_device, 0, 5, 0);
        stage4_arg = 0x13;
        stage5_arg = 6;
    } else {
        set_texture_stage_state(rasterizer_device, 0, 4, 2);
        stage4_arg = 3;
        stage5_arg = 5;
    }
    set_texture_stage_state(rasterizer_device, 0, stage5_arg, stage4_arg);
    set_texture_stage_state(rasterizer_device, 1, 1, 1);
    set_texture_stage_state(rasterizer_device, 1, 4, 1);

    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
}

#if 0
Original Ghidra decompilation (0x523980):

void FUN_00523980(void *param_1)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int *piVar7;
  undefined4 uVar8;
  undefined4 uVar9;

  if (*(short **)((int)param_1 + 0x58) == (short *)0x0) {
    if (*(int *)((int)param_1 + 0x54) == -1) goto LAB_005239b4;
    sVar2 = *(short *)(&DAT_006d99d8 + *(int *)((int)param_1 + 0x54) * 0x10);
  }
  else {
    sVar2 = **(short **)((int)param_1 + 0x58);
  }
  if (sVar2 == 4) {
    FUN_00522930(param_1);
    return;
  }
LAB_005239b4:
  chimera__rasterizer_set_texture(0,0,1,*(undefined2 *)((int)param_1 + 0x10));
  (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1b20);
  (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,0);
  (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
  iVar1 = *DAT_0071d174;
  uVar3 = __ftol();
  iVar4 = __ftol();
  uVar5 = __ftol();
  uVar6 = __ftol();
  (**(code **)(iVar1 + 0xe4))
            (DAT_0071d174,0x3c,((uVar3 & 0xff | iVar4 << 8) << 8 | uVar5 & 0xff) << 8 | uVar6 & 0xff
            );
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,3);
  if (*(short *)((int)param_1 + 0x14) == 1) {
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,0);
    uVar9 = 0x13;
    uVar8 = 6;
  }
  else {
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
    uVar9 = 3;
    uVar8 = 5;
  }
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,uVar8,uVar9);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
  piVar7 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
  rasterizer_transparent_geometry_group_draw_vertices(param_1,(void *)0x0,(char)piVar7);
  return;
}
#endif
