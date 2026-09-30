// rasterizer_glass_diffuse_draw_fixed_function  (Ghidra: FUN_00523d10)
// address 0x523d10, size 428 bytes
// phase 4 review: renamed from the rasterizer_decal_draw_* family. The procedure table
//   0x007c0480 holds ShaderTransparentGlass draw procedures: the shader fields these functions
//   read are glass fields (background_tint_map +0x70, diffuse_map +0x164, diffuse_detail_map
//   +0x178, reflection_map +0xb8, bump_map +0xcc; types/tags.h), not decal fields.
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase2/results/rasterizer_01.json ("Draws a batch of decal instances for the
// pre-1.1 capability path, delegating to the newer multi-instance renderer for material index
// 4."); selected by rasterizer_glass_draw_procedures_select.c. Like several other functions in
// this module, Ghidra's decompile mixes real argument values with fabricated return-address
// locals; the vertex-type dispatch, the declaration selection (lightmap_bitmap == 0 picks
// DAT_006e1b20, otherwise DAT_006e1b2c) and the shape of the SetTexture/SetSamplerState/
// SetRenderState/ID3DXEffect Begin-BeginPass-End sequence are preserved, but several individual
// SetSamplerState (stage, type, value) triples in the `lightmap_bitmap != 0` block could not be
// reliably reconstructed and are marked UNSURE at each site.
// register convention: param_1 as the recognized parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern void *rasterizer_device;          // 0x0071d174

// blam-cc: ESI -> bitmap, stack -> stage

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)


// blam-cc: ECX -> group


typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_glass_diffuse_draw_fixed_function(transparent_geometry_group *group)
{
    int16_t vertex_type;
    void **vtable;
    d3d_call1_fn set_vertex_declaration;
    d3d_call1_fn set_vertex_shader;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;
    uint32_t declaration;
    uint32_t pass_count;
    uint32_t pass;

    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot == -1) {
            goto fallback;
        }
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    } else {
        vertex_type = *(int16_t *)(void *)group->vertex_buffer;
    }

    if (vertex_type == 4) {
        rasterizer_glass_diffuse_draw(group);
        return;
    }

fallback:
    if (rasterizer_effects[109].effect == 0) {
        return;
    }

    declaration = rasterizer_vertex_declarations[13].declaration;
    if (group->lightmap_bitmap == 0) {
        declaration = rasterizer_vertex_declarations[12].declaration;
    }

    vtable = *(void ***)rasterizer_device;
    set_vertex_declaration = (d3d_call1_fn)vtable[0x57]; // +0x15c
    set_vertex_declaration(rasterizer_device, declaration);

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader = (d3d_call1_fn)vtable[0x5c]; // +0x170
    set_vertex_shader(rasterizer_device, 0);

    chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)group->shader + 0x164), 0, 0, 1, (int16_t)group->shader_permutation);

    if (group->lightmap_bitmap != 0) {
        rasterizer_bind_texture_d3d9(1, (BitmapData *)group->lightmap_bitmap); // ESI = group +0x5c (0x523db8)
        vtable = *(void ***)rasterizer_device;
        set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
        set_sampler_state(rasterizer_device, 1, 3, 1); // UNSURE: exact (type, value), see header
        set_sampler_state(rasterizer_device, 1, 5, 2);
        set_sampler_state(rasterizer_device, 1, 6, 2);
        set_sampler_state(rasterizer_device, 1, 7, 2);
    }

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x13, 5);
    set_render_state(rasterizer_device, 0x14, 6);
    set_render_state(rasterizer_device, 0xf, 1);

    vtable = *(void ***)(void *)rasterizer_effects[109].effect;
    ((int32_t (__stdcall *)(void *, uint32_t *, uint32_t))vtable[0x40])((void *)rasterizer_effects[109].effect, &pass_count, 3); // +0x100, Begin
    for (pass = 0; pass < pass_count; pass++) {
        ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x41])((void *)rasterizer_effects[109].effect, pass); // +0x104, BeginPass
        rasterizer_transparent_geometry_group_draw_vertices(group, group->lightmap_bitmap != 0); // ECX group, one stack flag (0x523e9a)
    }
    ((int32_t (__stdcall *)(void *))vtable[0x42])((void *)rasterizer_effects[109].effect); // +0x108, End
}

#if 0
Original Ghidra decompilation (0x523d10):

void FUN_00523d10(void *param_1)

{
  short sVar1;
  undefined4 extraout_EDX;
  int *piVar2;
  int *piVar3;
  int *piVar4;
  int *piStack_30;
  undefined4 uStack_2c;
  int *piStack_28;
  int *piStack_24;
  undefined4 uStack_20;
  uint uStack_1c;
  int *piStack_18;
  undefined4 uStack_14;
  int *piStack_10;
  undefined4 uStack_c;

  if (*(short **)((int)param_1 + 0x58) == (short *)0x0) {
    if (*(int *)((int)param_1 + 0x54) == -1) goto code_r0x00523d55;
    sVar1 = *(short *)(&DAT_006d99d8 + *(int *)((int)param_1 + 0x54) * 0x10);
  }
  else {
    sVar1 = **(short **)((int)param_1 + 0x58);
  }
  if (sVar1 == 4) {
    uStack_c = 0x523d3f;
    FUN_00523690();
    return;
  }
code_r0x00523d55:
  if (DAT_0069e1b0 != (int *)0x0) {
    uStack_c = DAT_006e1b2c;
    if (*(int *)((int)param_1 + 0x5c) == 0) {
      uStack_c = DAT_006e1b20;
    }
    piStack_10 = DAT_0071d174;
    uStack_14 = 0x523d84;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_14 = 0;
    piStack_18 = DAT_0071d174;
    uStack_1c = 0x523d94;
    (**(code **)(*DAT_0071d174 + 0x170))();
    uStack_1c = (uint)*(ushort *)((int)param_1 + 0x10);
    uStack_20 = 1;
    piStack_24 = (int *)0x0;
    piStack_28 = (int *)0x0;
    uStack_2c = 0x523dac;
    chimera__rasterizer_set_texture();
    if (*(int *)((int)param_1 + 0x5c) != 0) {
      uStack_1c = 1;
      uStack_20 = 0x523dbd;
      FUN_00518680();
      uStack_1c = 3;
      uStack_20 = 1;
      piStack_24 = (int *)0x1;
      piStack_28 = DAT_0071d174;
      uStack_2c = 0x523dd4;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_2c = 3;
      piStack_30 = (int *)0x2;
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
    }
    uStack_1c = 5;
    uStack_20 = 0x13;
    piStack_24 = DAT_0071d174;
    piStack_28 = (int *)0x523e36;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    piStack_28 = (int *)0x6;
    uStack_2c = 0x14;
    piStack_30 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    piVar4 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    piVar3 = DAT_0069e1b0;
    (**(code **)(*DAT_0069e1b0 + 0x100))(DAT_0069e1b0,&piStack_30,3);
    piVar2 = (int *)0x0;
    if (piVar4 != (int *)0x0) {
      do {
        (**(code **)(*DAT_0069e1b0 + 0x104))(DAT_0069e1b0,piVar2);
        rasterizer_transparent_geometry_group_draw_vertices
                  (param_1,(void *)CONCAT31((int3)((uint)extraout_EDX >> 8),
                                            *(int *)((int)param_1 + 0x5c) != 0),(char)piVar3);
        piVar2 = (int *)((int)piVar2 + 1);
      } while (piVar2 < piVar4);
    }
    (**(code **)(*DAT_0069e1b0 + 0x108))(DAT_0069e1b0);
  }
  return;
}
#endif
