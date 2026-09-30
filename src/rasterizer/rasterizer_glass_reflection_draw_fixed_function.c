// rasterizer_glass_reflection_draw_fixed_function  (Ghidra: FUN_00523b90)
// address 0x523b90, size 372 bytes
// phase 4 review: renamed from the rasterizer_decal_draw_* family. The procedure table
//   0x007c0480 holds ShaderTransparentGlass draw procedures: the shader fields these functions
//   read are glass fields (background_tint_map +0x70, diffuse_map +0x164, diffuse_detail_map
//   +0x178, reflection_map +0xb8, bump_map +0xcc; types/tags.h), not decal fields.
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase2/results/rasterizer_01.json ("Draws the lit decal variant for the pre-1.1
// capability path, delegating to the newer shaded renderer for material index 4."); selected by
// rasterizer_glass_draw_procedures_select.c. Structurally identical to
// rasterizer_glass_tint_draw_fixed_function.c's vertex-type dispatch, with a fixed literal decal color
// (0x3cffffff) instead of one packed from group->tint, and delegating to
// rasterizer_glass_reflection_draw (0x522c60) for vertex type 4.
// register convention: param_1, reflection_kind as the recognized parameters (reflection_kind's role is not
// established here; forwarded unchanged to the vertex-type-4 delegate).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern void *rasterizer_device;          // 0x0071d174

extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)


// blam-cc: ECX -> group


typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_glass_reflection_draw_fixed_function(transparent_geometry_group *group, uint32_t reflection_kind)
{
    int16_t vertex_type;
    void **vtable;
    d3d_call1_fn set_vertex_declaration;
    d3d_call1_fn set_vertex_shader;
    d3d_call1_fn set_pixel_shader;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_texture_stage_state;

    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot == -1) {
            goto fallback;
        }
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    } else {
        vertex_type = *(int16_t *)(void *)group->vertex_buffer;
    }

    if (vertex_type == 4) {
        rasterizer_glass_reflection_draw(group, (int16_t)reflection_kind);
        return;
    }

fallback:
    chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0xf8), 0, 0, 1, (int16_t)group->shader_permutation); // GlobalsRasterizerData test_1

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
    set_render_state(rasterizer_device, 0x13, 5);
    set_render_state(rasterizer_device, 0x14, 2);
    set_render_state(rasterizer_device, 0xf, 0);
    set_render_state(rasterizer_device, 0x3c, 0x3cffffff);

    vtable = *(void ***)rasterizer_device;
    set_texture_stage_state = (d3d_call3_fn)vtable[0x43]; // +0x10c
    set_texture_stage_state(rasterizer_device, 0, 1, 4);
    set_texture_stage_state(rasterizer_device, 0, 2, 2);
    set_texture_stage_state(rasterizer_device, 0, 3, 3);
    set_texture_stage_state(rasterizer_device, 0, 4, 2);
    set_texture_stage_state(rasterizer_device, 0, 5, 3);
    set_texture_stage_state(rasterizer_device, 1, 1, 1);
    set_texture_stage_state(rasterizer_device, 1, 4, 1);

    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
}

#if 0
Original Ghidra decompilation (0x523b90):

void FUN_00523b90(void *param_1,undefined4 param_2)

{
  short sVar1;
  int *piVar2;

  if (*(short **)((int)param_1 + 0x58) == (short *)0x0) {
    if (*(int *)((int)param_1 + 0x54) == -1) goto LAB_00523bc9;
    sVar1 = *(short *)(&DAT_006d99d8 + *(int *)((int)param_1 + 0x54) * 0x10);
  }
  else {
    sVar1 = **(short **)((int)param_1 + 0x58);
  }
  if (sVar1 == 4) {
    FUN_00522c60(param_1,param_2);
    return;
  }
LAB_00523bc9:
  chimera__rasterizer_set_texture(0,0,1,*(undefined2 *)((int)param_1 + 0x10));
  (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1b20);
  (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,0);
  (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,5);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0x3cffffff);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,3);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,3);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
  piVar2 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
  rasterizer_transparent_geometry_group_draw_vertices(param_1,(void *)0x0,(char)piVar2);
  return;
}
#endif
