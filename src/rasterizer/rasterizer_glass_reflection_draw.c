// rasterizer_glass_reflection_draw  (Ghidra: FUN_00522c60, unnamed; the phase 4 rewriter called it
// rasterizer_glass_reflection_draw)
// address 0x522c60, size 2596 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: slot 2 of the ShaderTransparentGlass draw procedure table 0x007c0480 (filled by
//   rasterizer_glass_draw_procedures_select 0x523ec0 for ps_1_1 and better). Every shader field
//   it reads is a ShaderTransparentGlass field (types/tags.h): flags +0x28 (bit 3
//   bump_map_is_specular_mask), perpendicular brightness/tint +0x8c/+0x90, parallel
//   brightness/tint +0x9c/+0xa0, reflection_map +0xb8, bump_map_scale +0xbc and bump_map +0xcc.
//   reflection_kind 0 is the bumped reflection (effect 106, vertex shaders 50+), 1 the flat one
//   (effect 107, 52+; kind 0 falls back to it without a usable bump map) and 2 the dynamic mirror
//   (effect 108, 54+, render target 2 on Texture3). The vertex shader variant is +1 for model
//   vertices (type 4). Without pixel shaders it sets up a fixed function additive pass with the
//   test_1 default bitmap instead. The draw goes through
//   rasterizer_transparent_geometry_group_draw_vertices 0x533660, with the bump specular mask
//   flag as the effect pass.
//   Spot-check fix (phase 4 review): the earlier file was a structural placeholder; rewritten in
//   full from the raw code 0x522c60..0x523683.
// register convention: __cdecl, (group, reflection_kind as int16) on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern uint8_t console_debug_toggle_689409;                         // 0x00689409

// blam-cc: EAX -> bitmap_tag_id, DX -> index
extern BitmapData *bitmap_group_get_bitmap_data(uint32_t bitmap_tag_id, int16_t index); // 0x43f250
// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage
extern uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot); // 0x5186c0
// blam-cc: EAX -> bitmap_tag_id, EDI -> effect_slot, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame,
                                                           rasterizer_effect_slot *effect_slot); // 0x518700
// blam-cc: EAX -> bitmap_tag_id, CX -> bitmap_type, stack -> (stage, default_index, frame, effect_slot)
extern int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
                                                      int16_t default_index, int16_t frame,
                                                      rasterizer_effect_slot *effect_slot); // 0x518860
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: ECX -> group
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x00533660

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_set_texture_fn)(void *effect, uint32_t handle, void *texture);
typedef int32_t (__stdcall *d3dx_effect_set_vector_fn)(void *effect, uint32_t handle, const float *vector);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

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

static void rasterizer_set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}

// 2 * pin(0.5 - 0.5 * x, 0, 1) - 1, i.e. -x pinned to [-1, 1]
static float real_negate_pinned(float x)
{
    float value = 0.5f - x * 0.5f;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return value + value - 1.0f;
}

void rasterizer_glass_reflection_draw(transparent_geometry_group *group, int16_t reflection_kind)
{
    const uint8_t *raw = (const uint8_t *)group->shader;
    int16_t vertex_type = -1;
    int16_t shader_variant = 0;                                 // UNSURE: uninitialised for vertex types
                                                                //   other than 0, 2 and 4
    int16_t shader_base = 0;
    rasterizer_effect_slot *effect_slot;
    uint32_t specular_mask_pass;
    float vectors[16];
    float constants[12];
    int32_t width;
    int32_t height;
    int32_t i;

    if (group->vertex_buffer != 0) {
        vertex_type = ((rasterizer_vertex_buffer *)group->vertex_buffer)->type;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }

    // view, perpendicular, parallel and fade vectors
    vectors[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
    vectors[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
    vectors[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
    vectors[3] = 1.0f;
    vectors[4] = *(const float *)(raw + 0x90);                  // perpendicular tint
    vectors[5] = *(const float *)(raw + 0x94);
    vectors[6] = *(const float *)(raw + 0x98);
    vectors[7] = *(const float *)(raw + 0x8c);                  // perpendicular brightness
    vectors[8] = *(const float *)(raw + 0xa0);                  // parallel tint
    vectors[9] = *(const float *)(raw + 0xa4);
    vectors[10] = *(const float *)(raw + 0xa8);
    vectors[11] = *(const float *)(raw + 0x9c);                 // parallel brightness
    for (i = 12; i < 16; i++) {
        vectors[i] = group->parameters.mode == 1 ? 1.0f - group->parameters.blend_factor : 1.0f;
    }

    // the bumped reflection needs a bump map that is not a specular mask
    if (reflection_kind == 0 && ((raw[0x28] & 8) != 0 || *(const uint32_t *)(raw + 0xcc) == 0xffffffff)) {
        reflection_kind = 1;
    }
    switch (vertex_type) {
    case 0:
    case 2:
        shader_variant = 0;
        break;
    case 4:
        shader_variant = 1;
        break;
    default:
        break;
    }

    effect_slot = &rasterizer_effects[106];
    switch (reflection_kind) {
    case 0:
        shader_base = 0x32;
        if (effect_slot->constant_handles != 0) {
            const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;
            void *effect = (void *)effect_slot->effect;

            for (i = 0; i < 4; i++) {
                ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[i], &vectors[i * 4]);
            }
        }
        break;
    case 1:
        effect_slot = &rasterizer_effects[107];
        shader_base = 0x34;
        if (rasterizer_caps.pixel_shader_version >= 0xffff0101) {
            ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, vectors, 3);
        }
        break;
    case 2:
        effect_slot = &rasterizer_effects[108];
        shader_base = 0x36;
        if (effect_slot->constant_handles != 0) {
            const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;
            void *effect = (void *)effect_slot->effect;

            for (i = 0; i < 3; i++) {
                ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[i], &vectors[i * 4]);
            }
        }
        break;
    default:
        break;
    }

    specular_mask_pass = (raw[0x28] >> 3) & 1;
    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[vertex_type].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                     (void *)rasterizer_vertex_shaders[shader_variant + shader_base].shader);
    if (effect_slot->effect == 0) {
        return;
    }

    width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
    height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[9] = 0.0f;
    constants[8] = 0.0f;                                        // UNSURE: never written in the original
    constants[10] = 0.0f;                                       // UNSURE: never written in the original

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        // fixed function: additive pass over the test_1 default bitmap
        constants[0] = 1.0f;
        constants[1] = 1.0f;
        constants[2] = 0.0f;
        constants[3] = 0.0f;
        constants[4] = (float)width * 0.5f;
        constants[5] = (float)height * 0.5f;
        constants[11] = 1.0f;
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xa, constants, 3);
        chimera__rasterizer_set_texture(*(uint32_t *)&rasterizer_globals_data->test_1.tag_id, 0, 0, 1,
                                        (int16_t)group->shader_permutation);
        ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);  // SetPixelShader(NULL)
        rasterizer_set_render_state(0x13, 5);                   // D3DRS_SRCBLEND srcalpha
        rasterizer_set_render_state(0x14, 2);                   // D3DRS_DESTBLEND one
        rasterizer_set_render_state(0xf, 0);                    // D3DRS_ALPHATESTENABLE
        rasterizer_set_render_state(0x3c, 0x3c7f7f7f);          // D3DRS_TEXTUREFACTOR
        rasterizer_set_texture_stage_state(0, 1, 7);
        rasterizer_set_texture_stage_state(0, 2, 2);
        rasterizer_set_texture_stage_state(0, 3, 3);
        rasterizer_set_texture_stage_state(0, 4, 2);
        rasterizer_set_texture_stage_state(0, 5, 2);
        rasterizer_set_texture_stage_state(1, 1, 4);
        rasterizer_set_texture_stage_state(1, 2, 1);
        rasterizer_set_texture_stage_state(1, 3, 0x20);
        rasterizer_set_texture_stage_state(1, 4, 2);
        rasterizer_set_texture_stage_state(1, 5, 1);
        rasterizer_set_texture_stage_state(2, 1, 1);
        rasterizer_set_texture_stage_state(2, 4, 1);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        return;
    }

    {
        void *effect = (void *)effect_slot->effect;
        float bump_scale = *(const float *)(raw + 0xbc);
        uint32_t bump_map_tag = *(const uint32_t *)(raw + 0xcc);
        uint32_t normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
        BitmapData *bump_bitmap = 0;
        uint32_t pass_count;

        constants[0] = group->unknown_3c * bump_scale;
        constants[1] = group->unknown_40 * bump_scale;
        constants[2] = (float)width * 0.5f;
        constants[3] = (float)height * 0.5f;
        constants[4] = 0.0f;
        constants[9] = 1.0f;
        constants[11] = 0.0f;
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xa, constants, 3);

        // stage 0: the bump map frame, or entry 3 of the default 2D bitmap
        if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)tag_instances[bump_map_tag & 0xffff].data;
            int32_t count = (int32_t)bitmap->bitmap_data.count;

            if (count > 0) {
                bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag,
                                                           (int16_t)((int32_t)(int16_t)group->shader_permutation % count));
                if (*(int16_t *)((uint8_t *)bump_bitmap + 0xa) != 0) {
                    bump_bitmap = 0;
                }
            }
        }
        if (bump_bitmap == 0) {
            uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

            if (default_tag != 0xffffffff) {
                Bitmap *bitmap = (Bitmap *)tag_instances[default_tag & 0xffff].data;

                if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                    bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
                }
            }
        }
        if (bump_bitmap != 0) {
            rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
            rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
            rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
        }

        // stages 1 and 2: vector normalization cube map, clamped, linear/point/point
        for (i = 1; i <= 2; i++) {
            chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, (int16_t)i, 0, effect_slot);
            rasterizer_set_sampler_state((uint32_t)i, 1, 3);
            rasterizer_set_sampler_state((uint32_t)i, 2, 3);
            rasterizer_set_sampler_state((uint32_t)i, 3, 3);
            rasterizer_set_sampler_state((uint32_t)i, 5, 2);
            rasterizer_set_sampler_state((uint32_t)i, 6, 1);
            rasterizer_set_sampler_state((uint32_t)i, 7, 1);
        }

        // stage 3: the mirror image for dynamic mirrors, else the reflection cube map
        if (reflection_kind == 2) {
            ((d3dx_effect_set_texture_fn)(*(void ***)effect)[0xd0 / 4])(effect, effect_slot->texture_handles[3],
                                                                        (void *)rasterizer_render_targets[2].texture);
            rasterizer_set_sampler_state(3, 1, 3);
            rasterizer_set_sampler_state(3, 2, 3);
            rasterizer_set_sampler_state(3, 5, 2);
            rasterizer_set_sampler_state(3, 6, 2);
            rasterizer_set_sampler_state(3, 7, 1);
        } else {
            rasterizer_resolve_and_cache_submap_b(*(const uint32_t *)(raw + 0xb8), 2, 3, 0, (int16_t)group->shader_permutation,
                                                  effect_slot);
            rasterizer_set_sampler_state(3, 1, 3);
            rasterizer_set_sampler_state(3, 2, 3);
            rasterizer_set_sampler_state(3, 3, 3);
            rasterizer_set_sampler_state(3, 5, 2);
            rasterizer_set_sampler_state(3, 6, 2);
            rasterizer_set_sampler_state(3, 7, 2);
        }
        rasterizer_set_render_state(0x13, 5);                   // D3DRS_SRCBLEND srcalpha
        rasterizer_set_render_state(0x14, 2);                   // D3DRS_DESTBLEND one
        rasterizer_set_render_state(0xf, 0);                    // D3DRS_ALPHATESTENABLE

        ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, specular_mask_pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
    }
}

#if 0
Original Ghidra decompilation (0x522c60) -- see `python tools/pack.py 0x522c60` for the full
2596-byte body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
