// rasterizer_glass_diffuse_draw  (Ghidra: FUN_00523690)
// address 0x523690, size 743 bytes
// phase 4 review: renamed from the rasterizer_decal_draw_* family. The procedure table
//   0x007c0480 holds ShaderTransparentGlass draw procedures: the shader fields these functions
//   read are glass fields (background_tint_map +0x70, diffuse_map +0x164, diffuse_detail_map
//   +0x178, reflection_map +0xb8, bump_map +0xcc; types/tags.h), not decal fields.
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase2/results/rasterizer_01.json ("Draws a batch of decal instances through a
// dedicated draw interface, for the post-1.1 capability path."); reached from
// rasterizer_glass_diffuse_draw_fixed_function.c for vertex type 4. Unlike the sibling *_post11 draw
// functions, this one calls ID3DXEffect::BeginPass exactly once (with the lightmap-present flag
// as the pass index) rather than looping over Begin's pass count, and draws exactly once.
// register convention: param_1 as the recognized parameter.
// UNSURE: the `else` branch of the shader-index remap (vertex type neither {0,2} nor 4) casts
// the group pointer itself to a shader index, the same suspicious pattern documented in
// rasterizer_glass_tint_draw.c; preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern void *rasterizer_device; // 0x0071d174
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: ECX -> group
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x00533660

typedef int32_t (*d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (*d3d_call4v_fn)(void *self, uint32_t start_register, const void *data, uint32_t count);

void rasterizer_glass_diffuse_draw(transparent_geometry_group *group)
{
    int16_t vertex_type;
    int16_t shader_index;
    uint32_t declaration;
    int32_t has_lightmap;
    void **vtable;
    d3d_call1_fn set_vertex_declaration;
    d3d_call1_fn set_vertex_shader;
    d3d_call4v_fn set_vertex_shader_constant_f;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;
    uint32_t pass_index;

    if (rasterizer_effects[109].effect == 0) {
        return;
    }

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
        declaration = (group->lightmap_bitmap == 0) ?
            (uint32_t)rasterizer_vertex_declarations[0].declaration : rasterizer_vertex_declarations[2].declaration;
    } else if (vertex_type == 4) {
        declaration = rasterizer_vertex_declarations[4].declaration;
        shader_index = 1;
    } else {
        shader_index = (int16_t)(uint32_t)group; // UNSURE: see file header
        declaration = (uint32_t)rasterizer_vertex_declarations[0].declaration;
    }

    vtable = *(void ***)rasterizer_device;
    set_vertex_declaration = (d3d_call1_fn)vtable[0x57]; // +0x15c
    set_vertex_declaration(rasterizer_device, declaration);

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader = (d3d_call1_fn)vtable[0x5c]; // +0x170
    set_vertex_shader(rasterizer_device, (uint32_t)rasterizer_vertex_shaders[48 + shader_index].shader);

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader_constant_f = (d3d_call4v_fn)vtable[0x5e]; // +0x178
    set_vertex_shader_constant_f(rasterizer_device, 10, &group->position, 3); // UNSURE source

    chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)group->shader + 0x164), 0, 0, 1, (int16_t)group->shader_permutation);
    chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)group->shader + 0x178), 1, 0, 2, (int16_t)group->shader_permutation);

    vtable = *(void ***)rasterizer_device;
    set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
    set_sampler_state(rasterizer_device, 1, 1, 1);
    set_sampler_state(rasterizer_device, 1, 2, 1);
    set_sampler_state(rasterizer_device, 1, 5, 2);
    set_sampler_state(rasterizer_device, 1, 6, 2);
    set_sampler_state(rasterizer_device, 1, 7, 2);

    has_lightmap = group->lightmap_bitmap != 0;
    if (has_lightmap) {
        rasterizer_bind_texture_d3d9(2, (BitmapData *)group->lightmap_bitmap); // ESI = group +0x5c (0x523880)
        set_sampler_state(rasterizer_device, 2, 1, 3);
        set_sampler_state(rasterizer_device, 2, 2, 3);
        set_sampler_state(rasterizer_device, 2, 5, 2);
        set_sampler_state(rasterizer_device, 2, 6, 2);
        set_sampler_state(rasterizer_device, 2, 7, 2);
    }

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x13, 5);
    set_render_state(rasterizer_device, 0x14, 6);
    set_render_state(rasterizer_device, 0xf, 1);

    vtable = *(void ***)(void *)rasterizer_effects[109].effect;
    pass_index = 0;
    ((int32_t (*)(void *, uint32_t *, uint32_t))vtable[0x40])((void *)rasterizer_effects[109].effect, &pass_index, 3); // +0x100, Begin
    ((int32_t (*)(void *, uint32_t))vtable[0x41])((void *)rasterizer_effects[109].effect, (uint32_t)has_lightmap); // +0x104, BeginPass
    rasterizer_transparent_geometry_group_draw_vertices(group, (int32_t)has_lightmap); // ECX group, one stack flag (0x523959)
    ((int32_t (*)(void *))vtable[0x42])((void *)rasterizer_effects[109].effect); // +0x108, End
}

#if 0
Original Ghidra decompilation (0x523690):

void FUN_00523690(void *param_1)

{
  short sVar1;
  undefined4 uVar2;
  bool bVar3;
  int *piVar4;
  int *piStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  int *piStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  uint uStack_6c;
  int *piStack_68;
  undefined4 uStack_64;
  undefined4 uStack_60;
  uint uStack_5c;
  int *piStack_58;
  undefined4 uStack_54;
  undefined1 *puStack_50;
  undefined4 uStack_4c;
  int *piStack_48;
  undefined4 uStack_44;

  if (DAT_0069e1b0 != (int *)0x0) {
    sVar1 = -1;
    if (*(short **)((int)param_1 + 0x58) == (short *)0x0) {
      if (*(int *)((int)param_1 + 0x54) != -1) {
        sVar1 = *(short *)(&DAT_006d99d8 + *(int *)((int)param_1 + 0x54) * 0x10);
      }
    }
    else {
      sVar1 = **(short **)((int)param_1 + 0x58);
    }
    if ((sVar1 == 0) || (sVar1 == 2)) {
      sVar1 = 0;
      if (*(int *)((int)param_1 + 0x5c) == 0) {
        uStack_44 = DAT_006e1a90;
        piStack_48 = DAT_0071d174;
        uStack_4c = 0x523736;
        (**(code **)(*DAT_0071d174 + 0x15c))();
      }
      else {
        uStack_44 = DAT_006e1aa8;
        piStack_48 = DAT_0071d174;
        uStack_4c = 0x523726;
        (**(code **)(*DAT_0071d174 + 0x15c))();
      }
    }
    else if (sVar1 == 4) {
      uStack_44 = DAT_006e1ac0;
      piStack_48 = DAT_0071d174;
      sVar1 = 1;
      uStack_4c = 0x523706;
      (**(code **)(*DAT_0071d174 + 0x15c))();
    }
    else {
      sVar1 = (short)param_1;
    }
    uStack_44 = (&DAT_0069e4d0)[sVar1 * 2];
    piStack_48 = DAT_0071d174;
    uStack_4c = 0x523755;
    (**(code **)(*DAT_0071d174 + 0x170))();
    uStack_4c = 3;
    puStack_50 = &stack0xffffffc8;
    uStack_54 = 10;
    piStack_58 = DAT_0071d174;
    uStack_5c = 0x5237e0;
    (**(code **)(*DAT_0071d174 + 0x178))();
    uStack_5c = (uint)*(ushort *)((int)param_1 + 0x10);
    uStack_60 = 1;
    uStack_64 = 0;
    piStack_68 = (int *)0x0;
    uStack_6c = 0x5237f8;
    chimera__rasterizer_set_texture();
    uStack_6c = (uint)*(ushort *)((int)param_1 + 0x10);
    uStack_70 = 2;
    uStack_74 = 0;
    piStack_78 = (int *)0x1;
    uStack_7c = 0x523810;
    chimera__rasterizer_set_texture();
    uStack_5c = 1;
    uStack_60 = 1;
    uStack_64 = 1;
    piStack_68 = DAT_0071d174;
    uStack_6c = 0x523827;
    (**(code **)(*DAT_0071d174 + 0x114))();
    uStack_6c = 1;
    uStack_70 = 2;
    uStack_74 = 1;
    piStack_78 = DAT_0071d174;
    uStack_7c = 0x52383b;
    (**(code **)(*DAT_0071d174 + 0x114))();
    uStack_7c = 2;
    uStack_80 = 5;
    uStack_84 = 1;
    piStack_88 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x114))();
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
    bVar3 = *(int *)((int)param_1 + 0x5c) != 0;
    if (bVar3) {
      FUN_00518680(2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,2);
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,5);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,6);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    (**(code **)(*DAT_0069e1b0 + 0x100))(DAT_0069e1b0,&piStack_88,3);
    piVar4 = DAT_0069e1b0;
    uVar2 = (**(code **)(*DAT_0069e1b0 + 0x104))(DAT_0069e1b0,bVar3);
    rasterizer_transparent_geometry_group_draw_vertices
              (param_1,(void *)CONCAT31((int3)((uint)uVar2 >> 8),*(int *)((int)param_1 + 0x5c) != 0)
               ,(char)piVar4);
    (**(code **)(*DAT_0069e1b0 + 0x108))(DAT_0069e1b0);
  }
  return;
}
#endif
