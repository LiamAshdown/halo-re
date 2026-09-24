// rasterizer_shader_environment_self_illumination_draw  (Ghidra: FUN_0051f3e0, unnamed; the phase
// 4 rewriter called it rasterizer_light_halo_draw)
// address 0x51f3e0, size 1764 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: the pixel shader (ps_1_1 and better) procedure stored in 0x007c048c by
//   rasterizer_select_hardware_codepaths 0x516810 (the fixed function variants are the
//   undefined functions 0x51fad0 and 0x51fd80). Every field it reads is a ShaderEnvironment
//   self-illumination field: self_illumination_flags +0x180 (unfiltered), primary/secondary/
//   plasma on and off colors, animation function, period and phase (+0x19c..+0x234), map_scale
//   +0x250 and map +0x260, plus material_color +0x10c. It draws with effect 0/1 (map present)
//   or 2/3 (no map), +1 when 0x006e0a04 is set, the bump map on stage 0, the self-illumination
//   map on stage 1 (point sampled when unfiltered), the environment lightmap (0x006e0a08) or the
//   capture texture 0x0069c66c on stage 2, the vector normalization cube map on stage 3, the
//   bump transform at c10..c12 and up to six effect vectors (material color; for effect 0 also
//   the plasma value, the animated primary and secondary colors and the plasma colors). One
//   two stream draw (0x51c310) per pass; the second stream is the lightmap vertices unless
//   0x006e0a04 is set.
//   Spot-check fix (phase 4 review): the earlier file (rasterizer_light_halo_draw) was a
//   structural placeholder; rewritten in full from the raw code 0x51f3e0..0x51fac3 (8 byte
//   aligned frame, all arguments through EBP).
// register convention: __cdecl, (shader, frame, dynamic_index_slot, first_primitive,
//   primitive_count, vertex_buffer) on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern void *rasterizer_capture_surfaces[4];                        // 0x0069c66c; [0] bound as a texture here
extern uint8_t unknown_006e0a04;                                    // 0x006e0a04 UNSURE: selects the odd effects
                                                                    //   and drops the lightmap stream
extern BitmapData *rasterizer_environment_lightmap;                 // 0x006e0a08 set from EAX by 0x51f310
extern uint8_t console_debug_toggle_6893f1;                         // 0x006893f1 self-illumination enable
extern uint8_t console_debug_toggle_68941c;                         // 0x0068941c allow alpha testing
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
// blam-cc: AX -> type, stack -> input
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0
// blam-cc: ESI -> shader_environment
extern void shader_environment_texture_scrolling_evaluate(float *u, float *v, double time, const ShaderEnvironment *shader); // 0x540060
// blam-cc: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, second_stream)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                        int32_t dynamic_index_slot, int32_t first_primitive,
                                                                        rasterizer_vertex_buffer *second_stream); // 0x51c310

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

static void rasterizer_set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}

static float shader_field(const uint8_t *raw, uint32_t offset)
{
    return *(const float *)(raw + offset);
}

// periodic function of (time + phase) / period for the animation block at `offset`
// (function int16 at +0, period at +4, phase at +8)
static float self_illumination_animation(const uint8_t *raw, uint32_t offset)
{
    return periodic_function_evaluate((periodic_function_t)*(const int16_t *)(raw + offset),
                                      (shader_field(raw, offset + 8) + rasterizer_time.time) / shader_field(raw, offset + 4));
}

void rasterizer_shader_environment_self_illumination_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
                                                          int32_t first_primitive, int32_t primitive_count,
                                                          rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    int16_t effect_index;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    uint32_t map_tag;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap;
    float constants[12];
    float primary, secondary, plasma, a, b;
    float primary_color[3], secondary_color[3];
    float material[4];
    float vectors[20];
    uint32_t pass_count;
    uint32_t pass;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    // D3DRS_ALPHATESTENABLE for alpha tested shaders
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, 0xf,
                                              (raw[0x28] & 1) != 0 && console_debug_toggle_68941c != 0 ? 1 : 0);

    map_tag = *(const uint32_t *)(raw + 0x260);
    if (map_tag == 0xffffffff) {
        effect_index = (int16_t)(2 + (unknown_006e0a04 != 0 ? 1 : 0));
    } else {
        effect_index = (int16_t)(unknown_006e0a04 != 0 ? 1 : 0);
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }

    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[2].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device, (void *)rasterizer_vertex_shaders[13].shader);

    // stage 0: the bump map (none for specular masks), or entry 3 of the default 2D bitmap
    bump_map_tag = (raw[0x28] & 2) != 0 ? 0xffffffff : *(const uint32_t *)(raw + 0x134);
    bump_bitmap = 0;
    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
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

    // stage 1: the self-illumination map
    rasterizer_resolve_and_cache_submap_b(map_tag, 0, 1, 0, frame, effect_slot);
    if ((raw[0x180] & 1) != 0) {                                // unfiltered
        rasterizer_set_sampler_state(1, 5, 1);
        rasterizer_set_sampler_state(1, 6, 1);
        rasterizer_set_sampler_state(1, 7, 1);
    } else {
        rasterizer_set_sampler_state(1, 5, 2);
        rasterizer_set_sampler_state(1, 6, 2);
        rasterizer_set_sampler_state(1, 7, 2);
    }
    // stage 2: the environment lightmap, else the capture texture straight into Texture2
    if (rasterizer_environment_lightmap != 0) {
        rasterizer_bind_texture_d3dx(2, rasterizer_environment_lightmap, effect_slot);
    } else {
        ((d3dx_effect_set_texture_fn)(*(void ***)effect)[0xd0 / 4])(effect, effect_slot->texture_handles[2],
                                                                    rasterizer_capture_surfaces[0]);
    }
    chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 3, 0,
                                                effect_slot);

    // c10: bump scale xy and map scale; c11/c12: the animated bump transform
    constants[0] = shader_field(raw, 0x138);
    constants[1] = shader_field(raw, 0x13c);
    constants[2] = shader_field(raw, 0x250);
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, shader);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xa, constants, 3);
    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[2].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device, (void *)rasterizer_vertex_shaders[13].shader);

    // animated colors: off * (1 - f) + on * f
    primary = self_illumination_animation(raw, 0x1b4);
    secondary = self_illumination_animation(raw, 0x1f0);
    plasma = self_illumination_animation(raw, 0x22c);
    a = 1.0f - primary;
    b = 1.0f - secondary;
    primary_color[0] = primary * shader_field(raw, 0x19c) + a * shader_field(raw, 0x1a8);
    primary_color[1] = primary * shader_field(raw, 0x1a0) + a * shader_field(raw, 0x1ac);
    primary_color[2] = primary * shader_field(raw, 0x1a4) + a * shader_field(raw, 0x1b0);
    secondary_color[0] = secondary * shader_field(raw, 0x1d8) + b * shader_field(raw, 0x1e4);
    secondary_color[1] = secondary * shader_field(raw, 0x1dc) + b * shader_field(raw, 0x1e8);
    secondary_color[2] = secondary * shader_field(raw, 0x1e0) + b * shader_field(raw, 0x1ec);

    material[0] = shader_field(raw, 0x10c);                     // material_color
    material[1] = shader_field(raw, 0x110);
    material[2] = shader_field(raw, 0x114);
    material[3] = 1.0f;
    if (effect_slot->constant_handles != 0) {
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, ((const uint32_t *)effect_slot->constant_handles)[0],
                                                                   material);
    }

    if (effect_index == 0) {
        vectors[0] = plasma;
        vectors[1] = plasma;
        vectors[2] = plasma;
        vectors[3] = 0.5f - plasma;
        vectors[4] = primary_color[0];
        vectors[5] = primary_color[1];
        vectors[6] = primary_color[2];
        vectors[7] = 1.0f;
        vectors[8] = secondary_color[0];
        vectors[9] = secondary_color[1];
        vectors[10] = secondary_color[2];
        vectors[11] = 1.0f;
        vectors[12] = shader_field(raw, 0x214);                 // plasma on color
        vectors[13] = shader_field(raw, 0x218);
        vectors[14] = shader_field(raw, 0x21c);
        vectors[15] = 1.0f;
        vectors[16] = shader_field(raw, 0x220);                 // plasma off color
        vectors[17] = shader_field(raw, 0x224);
        vectors[18] = shader_field(raw, 0x228);
        vectors[19] = 1.0f;
        if (effect_slot->constant_handles != 0) {
            const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;
            int32_t i;

            for (i = 0; i < 5; i++) {
                ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[1 + i], &vectors[i * 4]);
            }
        }
    }

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + (unknown_006e0a04 == 0 ? 1 : 0));
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x51f3e0) -- see `python tools/pack.py 0x51f3e0` for the full
1764-byte body; this rewrite is a low-confidence structural sketch, see file header.
#endif
