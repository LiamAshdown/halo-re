// rasterizer_glass_tint_draw  (Ghidra: FUN_00522930)
// address 0x522930, size 802 bytes
// phase 4 review: renamed from the rasterizer_decal_draw_* family. The procedure table
//   0x007c0480 holds ShaderTransparentGlass draw procedures: the shader fields these functions
//   read are glass fields (background_tint_map +0x70, diffuse_map +0x164, diffuse_detail_map
//   +0x178, reflection_map +0xb8, bump_map +0xcc; types/tags.h), not decal fields.
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase2/results/rasterizer_01.json ("Draws a decal using a material-selected
// texture and a fixed multitexture combiner setup, for the post-1.1 capability path."); reached
// from rasterizer_glass_tint_draw_fixed_function.c for vertex type 4. Same group field layout
// (shader_permutation at +0x10, parameters.mode at +0x14, vertex_buffer at +0x58,
// dynamic_vertex_slot at +0x54) and the same __ftol-based ColorARGB-to-D3DCOLOR packing of
// group->tint as rasterizer_glass_tint_draw_fixed_function.c.
// register convention: param_1 as the recognized parameter.
// UNSURE: the vertex-shader-constant upload's source data (`&stack0xffffffc0`) is not shown by
// Ghidra; passed here as group->position (a plausible per-decal constant) but not verified. The
// `else` branch of the shader-index remap casts the group pointer itself to a shader index
// (`sVar1 = (short)param_1`), which cannot be correct as written -- almost certainly another
// instance of the register-reuse corruption documented throughout this module -- but is
// preserved literally since no better source is available.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern void *rasterizer_device; // 0x0071d174
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: ECX -> group
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x00533660

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_call4v_fn)(void *self, uint32_t start_register, const void *data, uint32_t count);

void rasterizer_glass_tint_draw(transparent_geometry_group *group)
{
    int16_t vertex_type;
    int16_t shader_index;
    void **vtable;
    d3d_call1_fn set_vertex_declaration;
    d3d_call1_fn set_vertex_shader;
    d3d_call4v_fn set_vertex_shader_constant_f;
    d3d_call1_fn set_pixel_shader;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_texture_stage_state;
    uint32_t decal_color;
    uint32_t stage4_arg, stage5_arg;

    vertex_type = -1;
    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot != -1) {
            vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
        }
    } else {
        vertex_type = *(int16_t *)(void *)group->vertex_buffer;
    }

    if (vertex_type == 0 || vertex_type == 2) {
        shader_index = 0;
    } else if (vertex_type == 4) {
        shader_index = 1;
    } else {
        shader_index = (int16_t)(uint32_t)group; // UNSURE: see file header
    }

    chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)group->shader + 0x70), 0, 0, 1, (int16_t)group->shader_permutation);

    vtable = *(void ***)rasterizer_device;
    set_vertex_declaration = (d3d_call1_fn)vtable[0x57]; // +0x15c
    set_vertex_declaration(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[vertex_type].declaration);

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader = (d3d_call1_fn)vtable[0x5c]; // +0x170
    set_vertex_shader(rasterizer_device, (uint32_t)rasterizer_vertex_shaders[55 + shader_index].shader);

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader_constant_f = (d3d_call4v_fn)vtable[0x5e]; // +0x178
    set_vertex_shader_constant_f(rasterizer_device, 10, &group->position, 3); // UNSURE source, see header

    vtable = *(void ***)rasterizer_device;
    set_pixel_shader = (d3d_call1_fn)vtable[0x6b]; // +0x1ac
    set_pixel_shader(rasterizer_device, 0);

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x13, 1);
    set_render_state(rasterizer_device, 0x14, 3);
    set_render_state(rasterizer_device, 0xf, 1);

    decal_color = ((((uint32_t)(int32_t)(group->tint.alpha * 255.0f) & 0xff) << 8 |
                    ((uint32_t)(int32_t)(group->tint.red * 255.0f) & 0xff)) << 8 |
                   ((uint32_t)(int32_t)(group->tint.green * 255.0f) & 0xff)) << 8 |
                  ((uint32_t)(int32_t)(group->tint.blue * 255.0f) & 0xff);
    set_render_state(rasterizer_device, 0x3c, decal_color);

    vtable = *(void ***)rasterizer_device;
    set_texture_stage_state = (d3d_call3_fn)vtable[0x43]; // +0x10c
    set_texture_stage_state(rasterizer_device, 0, 1, 4);
    set_texture_stage_state(rasterizer_device, 0, 2, 2);
    set_texture_stage_state(rasterizer_device, 0, 3, 3);

    if (group->parameters.mode == 1) {
        set_texture_stage_state(rasterizer_device, 0, 4, 4);
        set_texture_stage_state(rasterizer_device, 0, 5, 0);
        stage4_arg = 3;
        stage5_arg = 6;
    } else {
        set_texture_stage_state(rasterizer_device, 0, 4, 2);
        stage4_arg = 0;
        stage5_arg = 5;
    }
    set_texture_stage_state(rasterizer_device, 0, stage5_arg, stage4_arg);
    set_texture_stage_state(rasterizer_device, 1, 1, 0x19);
    set_texture_stage_state(rasterizer_device, 1, 2, 1);
    set_texture_stage_state(rasterizer_device, 1, 3, 0x20);
    set_texture_stage_state(rasterizer_device, 1, 0x1a, 0x30);
    set_texture_stage_state(rasterizer_device, 1, 4, 2);
    set_texture_stage_state(rasterizer_device, 1, 5, 1);
    set_texture_stage_state(rasterizer_device, 2, 1, 1);
    set_texture_stage_state(rasterizer_device, 2, 4, 1);

    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
}

#if 0
Original Ghidra decompilation (0x522930):

void FUN_00522930(void *param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int *piVar7;
  undefined4 uVar8;
  undefined4 uVar9;

  sVar1 = -1;
  if (*(short **)((int)param_1 + 0x58) == (short *)0x0) {
    if (*(int *)((int)param_1 + 0x54) != -1) {
      sVar1 = *(short *)(&DAT_006d99d8 + *(int *)((int)param_1 + 0x54) * 0x10);
    }
  }
  else {
    sVar1 = **(short **)((int)param_1 + 0x58);
  }
  iVar6 = (int)sVar1;
  if ((iVar6 == 0) || (iVar6 == 2)) {
    sVar1 = 0;
  }
  else if (iVar6 == 4) {
    sVar1 = 1;
  }
  else {
    sVar1 = (short)param_1;
  }
  chimera__rasterizer_set_texture(0,0,1,*(undefined2 *)((int)param_1 + 0x10));
  (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,(&DAT_006e1a90)[iVar6 * 3]);
  (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,*(undefined4 *)(sVar1 * 8 + 0x69e508));
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&stack0xffffffc0,3);
  (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
  iVar6 = *DAT_0071d174;
  uVar2 = __ftol();
  iVar3 = __ftol();
  uVar4 = __ftol();
  uVar5 = __ftol();
  (**(code **)(iVar6 + 0xe4))
            (DAT_0071d174,0x3c,((uVar2 & 0xff | iVar3 << 8) << 8 | uVar4 & 0xff) << 8 | uVar5 & 0xff
            );
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,3);
  if (*(short *)((int)param_1 + 0x14) == 1) {
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,0);
    uVar9 = 3;
    uVar8 = 6;
  }
  else {
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
    uVar9 = 0;
    uVar8 = 5;
  }
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,uVar8,uVar9);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,0x19);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,2,1);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,3,0x20);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,0x1a,0x30);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,2);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,5,1);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,1,1);
  piVar7 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,4,1);
  rasterizer_transparent_geometry_group_draw_vertices(param_1,(void *)0x0,(char)piVar7);
  return;
}
#endif
