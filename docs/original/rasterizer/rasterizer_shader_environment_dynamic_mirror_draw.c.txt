// rasterizer_shader_environment_dynamic_mirror_draw  (Ghidra: FUN_00520e50, unnamed; the phase 4
// rewriter called it rasterizer_shader_environment_self_illumination_extra_pass_draw)
// address 0x520e50, size 1144 bytes
// VERIFIED against disassembly 0x520e50..0x5212c8 (2026-09-30): guards, effect selection (0x25/0x26/0x27), texture binds, c10..c12 constants, effect vectors and the pass loop
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: the sibling of rasterizer_shader_environment_reflection_draw 0x5202f0 for shaders with
//   ShaderEnvironment.reflection_flags dynamic_mirror (+0x2d0 bit 0), drawn only in the main
//   window (type 1) when window byte +4 (0x007c1224, the mirror pass flag) is set: effects
//   0x25 (bumped) / 0x26 / 0x27 (flat, the latter with bump_map_is_specular_mask), the bump map
//   on stage 0 and the vector normalization cube map on stage 1 (or only the normalization map
//   on stage 2 when there is no bump map), the mirror image rendered into
//   rasterizer_render_targets[2] bound as the effect Texture3, the same c10..c12 bump transform,
//   and a view vector whose w is -1 for specular mask shaders. One single stream draw
//   (0x51c1c0) per effect pass.
//   Spot-check fix (phase 4 review): rewritten in full from the raw code 0x520e50..0x5212c7. The shader arrives in EAX, not on the stack.
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
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f9;                         // 0x006893f9 dynamic mirrors enable
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
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

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

// blam-cc: EAX -> shader
void rasterizer_shader_environment_dynamic_mirror_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
                                                       int32_t first_primitive, int32_t primitive_count,
                                                       rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    uint32_t reflection_type;
    int16_t effect_index;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    uint32_t bump_map_tag;
    uint32_t normalization_tag;
    float constants[12];
    float vectors[12];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f9 == 0 ||
        rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1) {
        return;
    }

    reflection_type = *(uint16_t *)&((struct ShaderEnvironment *)raw)->reflection_type;
    if (reflection_type == 0 || reflection_type == 2) {
        if ((raw[0x28] & 2) != 0) {
            reflection_type = 1;
        }
        if (*(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id == 0xffffffff) {
            reflection_type = 1;
        }
    }
    if ((raw[0x2d0] & 1) == 0) {                                // not a dynamic mirror
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->perpendicular_brightness > 0.0f) && !(((struct ShaderEnvironment *)raw)->parallel_brightness > 0.0f)) {
        return;
    }

    switch ((int16_t)reflection_type) {
    case 0:
    case 2:
        effect_index = 0x25;
        break;
    case 1:
        effect_index = (raw[0x28] & 2) != 0 ? 0x27 : 0x26;
        break;
    default:
        effect_index = 0;                                       // UNSURE: uninitialised in the original
        break;
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }

    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[0].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                     (void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
    bump_map_tag = *(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    if (bump_map_tag == 0xffffffff) {
        chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    } else {
        BitmapData *bump_bitmap = 0;

        if (console_debug_toggle_689409 != 0) {
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
        chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 1, 0, effect_slot);
    }

    // the mirror image
    ((d3dx_effect_set_texture_fn)(*(void ***)effect)[0xd0 / 4])(effect, effect_slot->texture_handles[3],
                                                                (void *)rasterizer_render_targets[2].texture);

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

    vectors[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
    vectors[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
    vectors[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
    vectors[3] = (raw[0x28] & 2) != 0 ? -1.0f : 0.0f;
    vectors[4] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;                 // perpendicular color, brightness
    vectors[5] = *(const float *)(raw + 0x2ac);
    vectors[6] = *(const float *)(raw + 0x2b0);
    vectors[7] = ((struct ShaderEnvironment *)raw)->perpendicular_brightness;
    vectors[8] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;                 // parallel color, brightness
    vectors[9] = *(const float *)(raw + 0x2b8);
    vectors[10] = *(const float *)(raw + 0x2bc);
    vectors[11] = ((struct ShaderEnvironment *)raw)->parallel_brightness;
    if (effect_slot->constant_handles != 0) {
        const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;

        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[0], &vectors[0]);
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[1], &vectors[4]);
        ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[2], &vectors[8]);
    }

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x520e50) -- see `python tools/pack.py 0x520e50` for the full
1144-byte body; the rewrite above was compared instruction by instruction with the disassembly.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
