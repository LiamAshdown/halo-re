// rasterizer_shader_environment_reflection_draw  (Ghidra: FUN_005202f0, unnamed; the phase 4
// rewriter called it rasterizer_shader_environment_technique_extra_pass_draw)
// address 0x5202f0, size 1172 bytes
// VERIFIED against disassembly 0x5202f0..0x520784 (2026-09-30): guards, effect selection (0x20/0x21/0x22), bump/normalization/cube-map binds, c10..c12, the three effect vectors, and the second-stream draw (vertex_buffer + 0x14 when the force flag is set for bumped types)
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: every field it reads is a ShaderEnvironment field (types/tags.h): flags +0x28,
//   bump_map +0x134 (tag id), bump_map_scale_xy +0x138, u/v animation +0x150..+0x164 (through
//   shader_environment_texture_scrolling_evaluate), perpendicular/parallel color +0x2a8/+0x2b4, reflection_type +0x2d2,
//   perpendicular/parallel brightness +0x2f4/+0x2f8 and reflection_cube_map +0x330. It draws the
//   specular cube map reflection pass: effect 0x20 (bumped, types 0 and 2), 0x21 or 0x22 (flat,
//   0x22 when flags bit 1 is set), with the bump map on stage 0, the vector normalization cube
//   map on stages 1 and 2 and the reflection cube map on stage 3, three effect vectors (view
//   direction, perpendicular and parallel tint) and the bump map transform at vertex shader
//   c10..c12, then one draw per effect pass through the two stream helper 0x51c310.
//   Reached only through a draw procedure table (no direct callers).
//   Spot-check fix (phase 4 review): rewritten in full from the raw code 0x5202f0..0x520783 with the stack traced across the merged cdecl cleanups.
// register convention: __cdecl, (shader, frame, dynamic_index_slot, first_primitive,
//   primitive_count, vertex_buffer) on the stack.

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
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893fa;                         // 0x006893fa reflections enable
extern uint8_t console_debug_toggle_689409;                         // 0x00689409
extern int16_t render_force_flag;                             // 0x0069c67c (read as a word)

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
// blam-cc: ESI -> shader_environment; writes u = periodic(u function, time / u period) * u scale
//   and v likewise
extern void shader_environment_texture_scrolling_evaluate(float *u, float *v, double time, const ShaderEnvironment *shader); // 0x540060
// blam-cc: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, second_stream)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                        int32_t dynamic_index_slot, int32_t first_primitive,
                                                                        rasterizer_vertex_buffer *second_stream); // 0x51c310

typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_set_vector_fn)(void *effect, uint32_t handle, const float *vector);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
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

void rasterizer_shader_environment_reflection_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
                                                   int32_t first_primitive, int32_t primitive_count,
                                                   rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    uint32_t reflection_type;
    int16_t effect_index;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap;
    float constants[12];
    float vector[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893fa == 0 ||
        rasterizer_caps.pixel_shader_version < 0xffff0101) {
        return;
    }

    // bumped types fall back to the flat cube map without a usable bump map
    reflection_type = *(uint16_t *)&((struct ShaderEnvironment *)raw)->reflection_type;
    if (reflection_type == 0 || reflection_type == 2) {
        if ((raw[0x28] & 2) != 0) {
            reflection_type = 1;
        }
        if (*(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id == 0xffffffff) {
            reflection_type = 1;
        }
    }
    if (!(((struct ShaderEnvironment *)raw)->perpendicular_brightness > 0.0f) && !(((struct ShaderEnvironment *)raw)->parallel_brightness > 0.0f)) {
        return;
    }
    if (*(uint32_t *)&((struct ShaderEnvironment *)raw)->reflection_cube_map.tag_id == 0xffffffff) {
        return;
    }

    switch (reflection_type) {
    case 0:
    case 2:
        effect_index = 0x20;
        break;
    case 1:
        effect_index = (raw[0x28] & 2) != 0 ? 0x22 : 0x21;
        break;
    default:
        effect_index = 0;                                       // UNSURE: an uninitialised local in
        break;                                                  //   the original; types 0..2 only
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }

    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[0].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                     (void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    // stage 0: the bump map frame, or entry 3 of the default 2D bitmap
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
    chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 1, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 2, 0, effect_slot);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)&((struct ShaderEnvironment *)raw)->reflection_cube_map.tag_id, 2, 3, 0, frame, effect_slot);

    // c10: bump map scale xy and the 320x240 reference size; c11/c12: the animated bump map
    // transform rows [1 0 0 u] [0 1 0 v]
    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 320.0f;
    constants[3] = 240.0f;
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

    if (effect_slot->constant_handles != 0) {
        const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;

        vector[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
        vector[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
        vector[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
        vector[3] = 0.0f;
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[0], vector);

        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;              // perpendicular color
        vector[1] = *(const float *)(raw + 0x2ac);
        vector[2] = *(const float *)(raw + 0x2b0);
        vector[3] = ((struct ShaderEnvironment *)raw)->perpendicular_brightness;              // perpendicular brightness
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[1], vector);

        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;              // parallel color
        vector[1] = *(const float *)(raw + 0x2b8);
        vector[2] = *(const float *)(raw + 0x2bc);
        vector[3] = ((struct ShaderEnvironment *)raw)->parallel_brightness;              // parallel brightness
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[2], vector);
    }

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        int32_t lightmap_stream;

        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        lightmap_stream = (render_force_flag != 0 && (int16_t)reflection_type == 2) ? 1 : 0;
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + lightmap_stream);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x5202f0) -- see `python tools/pack.py 0x5202f0` for the full
1172-byte body; the rewrite above was compared instruction by instruction with the disassembly.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
