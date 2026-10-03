// rasterizer_shader_environment_lightmap_specular_draw  (Ghidra: FUN_00521f90, unnamed; the phase
// 4 rewriter called it rasterizer_dynamic_light_draw_extra_pass)
// address 0x521f90, size 866 bytes
// VERIFIED against disassembly 0x521f90..0x5222f2 (2026-09-30): guards, effect 42/43, vertex declaration 2, stage binds, c10..c12, the 16 pixel constants (3 uploaded), the two-stream draw
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: runs for shaders with ShaderEnvironment specular_flags lightmap_is_specular (+0x27c
//   bit 2) when the current BSP lightmap bitmap is known (0x006e0a68 clear; both globals are set
//   by 0x511f90 in the render module), 0x006893f7 is enabled, 0x0069c67c is clear and pixel
//   shaders are ps_1_4 or better: effect 42 (bump map is specular mask) or 43, the declaration of
//   vertex type 2 (lightmap stream), the bump map on stage 0, the lightmap (0x006e0a6c) on stage
//   1, the vector normalization cube map on stages 2 and 3, the bump transform at c10..c12 and
//   pixel shader c0..c2 = (brightness x4, perpendicular color | 1, parallel color | 1). A fourth
//   vector with the overbright scale (4 for overbright, else 2) is built but not uploaded.
//   One two stream draw (0x51c310, second stream = the lightmap vertices at vertex_buffer + 1)
//   per effect pass.
//   Spot-check fix (phase 4 review): rewritten in full from the raw code 0x521f90..0x5222f1. The shader arrives in EAX.
// register convention: EAX = shader, stack = (frame, dynamic_index_slot, first_primitive,
//   primitive_count, vertex_buffer).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                                     // 0x0071d174
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern uint8_t rasterizer_lightmap_bitmap_missing;                  // 0x006e0a68 set by 0x511f90
extern BitmapData *rasterizer_lightmap_bitmap;                      // 0x006e0a6c set by 0x511f90
extern int16_t render_force_flag;                             // 0x0069c67c (read as a word)
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f7;                         // 0x006893f7 lightmap specular enable
extern uint8_t console_debug_toggle_689409;                         // 0x00689409

// blam-cc: EAX -> bitmap_tag_id, DX -> index
extern BitmapData *bitmap_group_get_bitmap_data(uint32_t bitmap_tag_id, int16_t index); // 0x43f250
// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage
extern uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot); // 0x5186c0
// blam-cc: EAX -> bitmap_tag_id, EDI -> effect_slot, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame,
                                                           rasterizer_effect_slot *effect_slot); // 0x518700
// blam-cc: ESI -> shader_environment
extern void shader_environment_texture_scrolling_evaluate(float *u, float *v, double time, const ShaderEnvironment *shader); // 0x540060
// blam-cc: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, second_stream)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                        int32_t dynamic_index_slot, int32_t first_primitive,
                                                                        rasterizer_vertex_buffer *second_stream); // 0x51c310

typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_texture_fn)(void *self, uint32_t stage, void *texture);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

// blam-cc: EAX -> shader
void rasterizer_shader_environment_lightmap_specular_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
                                                          int32_t first_primitive, int32_t primitive_count,
                                                          rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    uint16_t specular_flags;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float specular_exponent;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap;
    uint32_t normalization_tag;
    float constants[12];
    float pixel_constants[16];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f7 == 0 || render_force_flag != 0 ||
        rasterizer_lightmap_bitmap_missing != 0 || rasterizer_caps.pixel_shader_version < 0xffff0104) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->brightness > 0.0f)) {
        return;
    }
    specular_flags = *(uint16_t *)&((struct ShaderEnvironment *)raw)->specular_flags;
    if ((specular_flags & 4) == 0) {                            // lightmap_is_specular
        return;
    }
    effect_slot = (raw[0x28] & 2) != 0 ? &rasterizer_effects[42] : &rasterizer_effects[43];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }
    specular_exponent = (specular_flags & 1) != 0 ? 4.0f : 2.0f;

    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[2].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                     (void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    // stage 0: bump map frame, or entry 3 of the default 2D bitmap
    bump_map_tag = *(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    bump_bitmap = 0;
    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bump_bitmap)->type != 0) {
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

    // stage 1: the current lightmap
    if (rasterizer_lightmap_bitmap_missing == 0) {
        rasterizer_bind_texture_d3dx(1, rasterizer_lightmap_bitmap, effect_slot);
    } else {
        ((d3d_set_texture_fn)device_vtable()[0x104 / 4])(rasterizer_device, 1, 0);
    }
    normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 3, 0, effect_slot);

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 1.0f;
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

    // pixel shader c0..c2; the fourth vector (the specular exponent) is filled but not uploaded
    pixel_constants[0] = ((struct ShaderEnvironment *)raw)->brightness;         // brightness
    pixel_constants[1] = pixel_constants[0];
    pixel_constants[2] = pixel_constants[0];
    pixel_constants[3] = pixel_constants[0];
    pixel_constants[4] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;         // perpendicular color
    pixel_constants[5] = *(const float *)(raw + 0x2ac);
    pixel_constants[6] = *(const float *)(raw + 0x2b0);
    pixel_constants[7] = 1.0f;
    pixel_constants[8] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;         // parallel color
    pixel_constants[9] = *(const float *)(raw + 0x2b8);
    pixel_constants[10] = *(const float *)(raw + 0x2bc);
    pixel_constants[11] = 1.0f;
    pixel_constants[12] = specular_exponent;
    pixel_constants[13] = specular_exponent;
    pixel_constants[14] = specular_exponent;
    pixel_constants[15] = specular_exponent;
    ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, pixel_constants, 3);

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + 1);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x521f90) -- see `python tools/pack.py 0x521f90` for the full
866-byte body; the rewrite above was compared instruction by instruction with the disassembly.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
