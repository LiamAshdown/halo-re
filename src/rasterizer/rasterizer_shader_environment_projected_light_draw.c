// rasterizer_shader_environment_projected_light_draw  (Ghidra: FUN_00521900, unnamed; the phase 4
// rewriter called it rasterizer_dynamic_light_draw)
// address 0x521900, size 946 bytes
// VERIFIED against disassembly 0x521900..0x521cb2 (2026-09-30): guards, effect 40/41, vertex shader variant, c13..c17, bump/cube/normalization binds, c10..c12, the four effect vectors, the draw
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: draws one shader_environment surface lit by the current projected light (ps_1_4 and
//   better, 0x006893f6 enabled, shader brightness +0x290 and rasterizer_projected_light_luminance
//   0x0071d1d8 both positive): effect 40 (bump map is specular mask) or 41, vertex shader
//   effect.vertex_shader_index + 0x0069c6fc (the falloff variant chosen by
//   rasterizer_projected_light_constants_build 0x521750), the five projected light vectors at
//   c13 (0x006e0a10), the bump map on stage 0, the light cube map (through
//   rasterizer_resolve_and_cache_submap_b) or plain light bitmap (direct) on stage 1, the vector
//   normalization cube map on stages 2 and 3, the bump transform at c10..c12 and four effect
//   vectors (luminance * brightness, perpendicular and parallel colors, the specular exponent
//   4 for overbright else 2). One single stream draw (0x51c1c0) per effect pass.
//   Spot-check fix (phase 4 review): rewritten in full from the raw code 0x521900..0x521cb1.
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
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern rasterizer_projected_light_constants rasterizer_projected_light; // 0x006e0a10
extern uint8_t rasterizer_projected_light_has_cube_map;             // 0x006e0a60
extern uint32_t rasterizer_projected_light_cube_map;                // 0x006e0a64 bitmap tag id
extern int16_t rasterizer_projected_light_shader_variant;           // 0x0069c6fc 0 or 1, set by 0x521750
extern float rasterizer_projected_light_luminance;                  // 0x0071d1d8
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f6;                         // 0x006893f6 projected lights enable
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
// blam-cc: ESI -> shader_environment
extern void shader_environment_texture_scrolling_evaluate(float *u, float *v, double time, const ShaderEnvironment *shader); // 0x540060
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

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

// The bump map frame on stage 0, or entry 3 of the default 2D bitmap (inlined in every
// shader_environment pass of this family).
static void rasterizer_bind_bump_map(uint32_t bump_map_tag, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    BitmapData *bump_bitmap = 0;

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

        if (default_tag == 0xffffffff) {
            return;
        }
        {
            Bitmap *bitmap = (Bitmap *)tag_instances[default_tag & 0xffff].data;

            if (bitmap == 0 || (int32_t)bitmap->bitmap_data.count <= 3) {
                return;
            }
            bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
        }
    }
    rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
    rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
    rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
}

void rasterizer_shader_environment_projected_light_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
                                                        int32_t first_primitive, int32_t primitive_count,
                                                        rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float specular_exponent;
    uint32_t normalization_tag;
    float constants[12];
    float vector[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f6 == 0 ||
        rasterizer_caps.pixel_shader_version < 0xffff0104) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->brightness > 0.0f) || !(rasterizer_projected_light_luminance > 0.0f)) {
        return;
    }
    effect_slot = (raw[0x28] & 2) != 0 ? &rasterizer_effects[40] : &rasterizer_effects[41];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }
    specular_exponent = (raw[0x27c] & 1) != 0 ? 4.0f : 2.0f;   // specular_flags overbright

    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[0].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(
        rasterizer_device,
        (void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index + rasterizer_projected_light_shader_variant].shader);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, (const float *)&rasterizer_projected_light, 5);

    rasterizer_bind_bump_map(*(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id, frame, effect_slot);
    if (rasterizer_projected_light_has_cube_map == 1) {
        rasterizer_resolve_and_cache_submap_b(rasterizer_projected_light_cube_map, 2, 1, 1, 0, effect_slot);
    } else {
        chimera__rasterizer_set_texture_direct_d3dx(rasterizer_projected_light_cube_map, 1, 0, effect_slot);
    }
    normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 3, 0, effect_slot);

    // c10: bump map scale xy; c11/c12: the animated bump transform [1 0 0 u] [0 1 0 v]
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

    if (effect_slot->constant_handles != 0) {
        const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;
        float light = rasterizer_projected_light_luminance * ((struct ShaderEnvironment *)raw)->brightness;

        vector[0] = light;
        vector[1] = light;
        vector[2] = light;
        vector[3] = light;
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[0], vector);
        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;              // perpendicular color
        vector[1] = *(const float *)(raw + 0x2ac);
        vector[2] = *(const float *)(raw + 0x2b0);
        vector[3] = 1.0f;
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[1], vector);
        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;              // parallel color
        vector[1] = *(const float *)(raw + 0x2b8);
        vector[2] = *(const float *)(raw + 0x2bc);
        vector[3] = 1.0f;
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[2], vector);
        vector[0] = specular_exponent;
        vector[1] = specular_exponent;
        vector[2] = specular_exponent;
        vector[3] = specular_exponent;
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[3], vector);
    }

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x521900) -- see `python tools/pack.py 0x521900` for the full
946-byte body; this rewrite is a the rewrite above was compared instruction by instruction with the disassembly.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
