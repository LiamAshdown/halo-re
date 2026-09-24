// rasterizer_shader_model_draw_pixel_shader  (Ghidra: rasterizer_shader_environment_draw_pixelshader;
//   the earlier placeholder kept that name)
// address 0x529e00, size 4674 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: installed in 0x007c0474 by rasterizer_shader_environment_select_draw_functions
//   0x52b630 on ps_1_1 and later cards and called through it by
//   rasterizer_shader_environment_draw_dispatch 0x52b050 for every shader type but 3. The shader is
//   a ShaderModel (types/tags.h): flags +0x28, translucency +0x38, change_color_source +0x4c, more
//   flags +0x6c, color_source/animation_function/period/bounds +0x70..+0x8c, map scales +0x9c,
//   base, multipurpose, detail and reflection cube maps, detail function/mask +0xd4/+0xd6, detail
//   scales +0xd8/+0xec, texture animation +0xfc, reflection falloff/cutoff and brightness/tint
//   +0x13c..+0x160. Rebuilt from the raw disassembly: Ghidra lost the frame (ebp based, eight byte
//   aligned), the technique/vertex shader selection, the fog math and every call argument.
// What it does: picks the technique with rasterizer_shader_model_select_technique 0x527500, binds
//   base/detail/multipurpose/cube maps into it, computes the animated color, change color, the
//   planar and atmospheric fog colors and the reflection falloff, uploads them as effect vectors
//   and vertex shader constants (c10..c14), chooses vertex shader 25, 26, 28 or 29, draws every
//   effect pass, and for two sided models draws again with CULLMODE CW and a mirrored c10.w.
// register convention: all seven arguments on the stack.
// blam-cc: stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot)
// UNSURE: 0x0071d1fb (selects vertex shader 25), 0x0071cfc0 (four floats that override c13/c14
//   when any is positive) and 0x007c047c (the fog alpha scale) have no known owner or name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern rasterizer_frame_time rasterizer_time;               // 0x007c1200
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern uint8_t rasterizer_camouflage_fade_active;           // 0x0071d1fe
extern float rasterizer_camouflage_fade;                    // 0x0071d200
extern uint8_t rasterizer_fog_enabled;                      // 0x0069c6a8
extern uint8_t unknown_0071d1fb;                            // 0x0071d1fb UNSURE
extern float *unknown_0071cfc0;                             // 0x0071cfc0 UNSURE: four floats
extern float unknown_007c047c;                              // 0x007c047c UNSURE: fog alpha scale
extern const ColorRGB *global_white_color;                  // 0x00686b04
extern float rasterizer_model_effect_vector[4];             // 0x006e17e4 scratch vector for SetVector
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

// blam-cc: ECX -> shader
extern rasterizer_effect_slot *rasterizer_shader_model_select_technique(const ShaderModel *shader); // 0x527500
extern void rasterizer_apply_decal_zbias(void); // 0x5194e0
extern void rasterizer_clear_decal_zbias(void); // 0x519580
// blam-cc: EAX -> a, ECX -> b
extern real vector3d_distance(const real_point3d *a, const real_point3d *b); // 0x4088b0
extern uint32_t color_rgb_float_to_int(const ColorRGB *color); // 0x4ab5d0
extern double fabs(double x);                                  // inline x87 fabs
// blam-cc: AX -> type, stack -> time
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0
// blam-cc: EAX -> bitmap_tag_id, CX -> bitmap_type, stack -> (stage, default_index, frame, effect_slot)
extern int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
                                                      int16_t default_index, int16_t frame,
                                                      rasterizer_effect_slot *effect_slot); // 0x518860
// blam-cc: ECX -> function_source, ESI -> animation, EBX -> out_u, EDI -> out_v, stack -> the rest
extern void shader_texture_animation_evaluate(const void *function_source, const void *animation,
                                              float *out_u, float *out_v, float u_scale, float v_scale,
                                              float unused_z, float unused_w, float unused_5,
                                              float time); // 0x53fe50
// blam-cc: stack -> (index_buffer, dynamic_index_slot, vertex_buffer), EAX -> primitive_count,
//   ECX -> first_primitive, EBX -> dynamic_vertex_slot
extern void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                      rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count,
                                                      int32_t first_primitive, int32_t dynamic_vertex_slot); // 0x51c730
// blam-cc: EAX -> vertex_buffer, EDI -> index_buffer, stack -> primitive_count
extern void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                   rasterizer_index_buffer *index_buffer); // 0x51c5f0
extern void rasterizer_dynamic_vertex_draw_indexed(rasterizer_index_buffer *index_buffer, int32_t primitive_count,
                                                   int32_t dynamic_vertex_slot); // 0x51c490
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0
extern void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count,
                                                int32_t dynamic_vertex_slot); // 0x51c090

typedef int32_t (*d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (*d3dx_effect_set_vector_fn)(void *effect, uint32_t handle, const float *vector);
typedef int32_t (*d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (*d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (*d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static int32_t set_render_state(uint32_t state, uint32_t value)
{
    return ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}
// SetVector of one of the effect constant handles from the scratch vector 0x006e17e4.
static void set_effect_vector(rasterizer_effect_slot *slot, int handle, float x, float y, float z, float w)
{
    uint32_t *handles = (uint32_t *)(uintptr_t)slot->constant_handles;
    void *effect = (void *)(uintptr_t)slot->effect;

    rasterizer_model_effect_vector[0] = x;
    rasterizer_model_effect_vector[1] = y;
    rasterizer_model_effect_vector[2] = z;
    rasterizer_model_effect_vector[3] = w;
    ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[handle], rasterizer_model_effect_vector);
}

void rasterizer_shader_model_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
                                               int32_t dynamic_index_slot, int32_t primitive_count,
                                               rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    const ShaderModel *model = (const ShaderModel *)shader;
    rasterizer_model_draw_context *context;
    rasterizer_effect_slot *slot;
    uint8_t decal = (shader[0x28] >> 3) & 1;
    uint8_t ok = 1;
    uint8_t cull = 1;
    uint16_t true_atmospheric_fog;
    float depth;
    float reflection;                     // falloff between cutoff and falloff distance
    float scale;
    ColorRGB animated;                    // animated color times color source
    ColorRGB change;                      // change color
    ColorRGB fog_add = { 0.0f, 0.0f, 0.0f };      // atmospheric color x density
    ColorRGB fog_planar = { 0.0f, 0.0f, 0.0f };   // planar remainder, clamped
    ColorRGB fog_negative = { 0.0f, 0.0f, 0.0f }; // clamped negation of the planar remainder
    float fog_keep = 1.0f;                // 1 - atmospheric density
    int16_t vertex_shader;
    float reflection_constants[2][4];
    float detail_constants[3][4];
    uint32_t passes;
    uint32_t pass;

    slot = rasterizer_shader_model_select_technique(model);
    if (slot == NULL || slot->effect == 0) {
        return;
    }
    context = rasterizer_active_model_context;
    depth = (context->center.y - rasterizer_window.camera.position.y) * rasterizer_window.camera.forward.j +
            (context->center.z - rasterizer_window.camera.position.z) * rasterizer_window.camera.forward.k +
            (context->center.x - rasterizer_window.camera.position.x) * rasterizer_window.camera.forward.i;
    if (model->reflection_cutoff_distance != 0.0f) {
        float t = (depth - model->reflection_cutoff_distance) /
                  (model->reflection_falloff_distance - model->reflection_cutoff_distance);

        reflection = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    } else {
        reflection = 1.0f;
    }

    if (context->flags & 8) {
        set_render_state(0x07, 0);
    } else {
        if (!rasterizer_camouflage_fade_active) {
            set_render_state(0x07, 1);
            set_render_state(0x17, 4);
            set_render_state(0x0e, decal ? 0 : 1);
        }
        if (decal) {
            rasterizer_apply_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }
    if (shader[0x28] & 2) {                                    // two_sided
        if (shader[0x28] & 0x20) {                             // disable_two_sided_culling
            float distance = (float)vector3d_distance(&rasterizer_active_model_context->center,
                                                      &rasterizer_window.camera.position);

            cull = 0;
            if (!(distance > 8.0f)) {
                cull = 1;
            }
        } else {
            cull = 1;
        }
    }
    set_render_state(0x16, cull ? 3 : 1);
    if (!rasterizer_camouflage_fade_active) {
        set_render_state(0xa8, 7);
    }
    set_render_state(0x1b, (decal || rasterizer_camouflage_fade_active) ? 1 : 0);
    set_render_state(0x13, 5);
    set_render_state(0x14, 6);
    set_render_state(0xab, 1);
    set_render_state(0x0f, (!rasterizer_camouflage_fade_active && !decal && !(shader[0x28] & 4)) ? 1 : 0);
    set_render_state(0x18, 0x7f);
    if (rasterizer_caps.pixel_shader_version < 0xffff0104) {
        set_render_state(0x1c, rasterizer_fog_enabled ? 1 : 0);
    } else {
        set_render_state(0x1c, (shader[0x28] >> 4) & 1);       // true_atmospheric_fog
    }
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xb0), 0, 0, 1, frame, slot);   // base_map
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xe8), 0, 1, 2, frame, slot);   // detail_map
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xc8), 0, 2, 1, frame, slot);   // multipurpose_map
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x170), 2, 3, 0, frame, slot);  // reflection_cube_map

    context = rasterizer_active_model_context;
    animated.red = 0.0f;
    animated.green = 0.0f;
    animated.blue = 0.0f;
    if (rasterizer_caps.pixel_shader_version >= 0xffff0104 || model->detail_mask == 0) {
        // animated color between the two bounds, times the color source, into effect handle 4
        float phase;
        float value;
        ColorRGB delta;

        scale = 1.0f;
        if (shader[0x6c] & 1) {
            phase = 0.0f;
        } else {
            uint32_t seed = (context->unknown_04 * 0x19660d + 0x3c6ef35f) >> 16;

            phase = (float)seed * 1.5259022e-05f;
        }
        delta.red = model->animation_color_upper_bound.red - model->animation_color_lower_bound.red;
        delta.green = model->animation_color_upper_bound.green - model->animation_color_lower_bound.green;
        delta.blue = model->animation_color_upper_bound.blue - model->animation_color_lower_bound.blue;
        value = (float)periodic_function_evaluate((periodic_function_t)model->animation_function,
                                                  phase + rasterizer_time.time / model->animation_period);
        animated.red = delta.red * value + model->animation_color_lower_bound.red;
        animated.green = delta.green * value + model->animation_color_lower_bound.green;
        animated.blue = delta.blue * value + model->animation_color_lower_bound.blue;
        if (model->color_source > 0 && model->color_source < 5) {
            const ColorRGB *colors = (const ColorRGB *)(uintptr_t)context->unknown_84[0];
            const ColorRGB *source = &colors[model->color_source - 1];

            animated.red *= source->red;
            animated.green *= source->green;
            animated.blue *= source->blue;
            if (model->detail_mask == 0 && model->color_source == 2 &&
                rasterizer_caps.pixel_shader_version < 0xffff0104) {
                float distance = (float)fabs(vector3d_distance(&context->center, &rasterizer_window.camera.position));

                if (distance < 6.0f) {
                    scale = 1.0f - distance * 0.16666667f;
                }
            }
        }
        rasterizer_model_effect_vector[3] = 1.0f;
        rasterizer_model_effect_vector[0] = animated.red * scale;
        rasterizer_model_effect_vector[1] = animated.green * scale;
        rasterizer_model_effect_vector[2] = animated.blue * scale;
        if (slot->constant_handles != 0) {
            uint32_t *handles = (uint32_t *)(uintptr_t)slot->constant_handles;
            void *effect = (void *)(uintptr_t)slot->effect;

            ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handles[4], rasterizer_model_effect_vector);
        }
    }
    context = rasterizer_active_model_context;
    if (model->change_color_source > 0 && model->change_color_source < 5) {
        change = ((const ColorRGB *)(uintptr_t)context->unknown_84[0])[model->change_color_source - 1];
    } else {
        change = *global_white_color;
    }

    // vertex shader: 25 (0x0071d1fb), 26 (point lights), 28 (general) or 29 (no extra maps)
    true_atmospheric_fog = *(uint16_t *)(shader + 0x28) & 0x10;
    if (true_atmospheric_fog) {
        vertex_shader = 0x1c;
    } else if (unknown_0071d1fb) {
        vertex_shader = 0x19;
    } else if (context->lighting.point_light_count > 0) {
        vertex_shader = 0x1a;
    } else if (context->node_count > 1) {
        vertex_shader = 0x1c;
    } else if (*(int32_t *)(shader + 0xc8) != -1 &&
               (model->detail_mask != 0 ||
                animated.red != 0.0f || animated.green != 0.0f || animated.blue != 0.0f ||
                change.red != 1.0f || change.green != 1.0f || change.blue != 1.0f)) {
        vertex_shader = 0x1c;
    } else if (*(int32_t *)(shader + 0x170) != -1 && reflection > 0.0f) {
        vertex_shader = 0x1c;
    } else {
        vertex_shader = 0x1d;
    }

    // fog
    if (!rasterizer_fog_enabled) {
        // everything zero, keep 1
    } else if (true_atmospheric_fog) {
        if (rasterizer_caps.pixel_shader_version < 0xffff0104) {
            set_render_state(0x1c, 1);
            set_render_state(0x22, color_rgb_float_to_int(&rasterizer_window.fog.atmospheric_color)); // FOGCOLOR
        } else {
            // UNSURE: the binary leaves the fog slots untouched here; the one holding fog_add still
            // holds the animated color and the planar/negative slots are uninitialised stack
            fog_add = animated;
        }
    } else if (!(context->flags & 4)) {
        const render_fog *fog = &rasterizer_window.fog;
        float height = clamp01((fog->plane.normal.i * rasterizer_window.camera.position.x +
                                fog->plane.normal.k * rasterizer_window.camera.position.z +
                                fog->plane.normal.j * rasterizer_window.camera.position.y - fog->plane.d) /
                               fog->atmospheric_maximum_distance);
        float density = clamp01((depth - fog->atmospheric_minimum_distance) /
                                (fog->atmospheric_maximum_distance - fog->atmospheric_minimum_distance)) *
                        fog->atmospheric_maximum_density;

        if (fog->flags & 2) {
            height = 1.0f;
        }
        fog_keep = 1.0f - density;
        fog_planar.red = fog->planar_color.red -
                         (fog->atmospheric_color.red * (1.0f - height) + height * fog->planar_color.red) * density;
        fog_planar.green = fog->planar_color.green -
                           (fog->planar_color.green * height + (1.0f - height) * fog->atmospheric_color.green) * density;
        fog_planar.blue = fog->planar_color.blue -
                          (fog->planar_color.blue * height + (1.0f - height) * fog->atmospheric_color.blue) * density;
        fog_negative.red = clamp01(-fog_planar.red);
        fog_negative.green = clamp01(-fog_planar.green);
        fog_negative.blue = clamp01(-fog_planar.blue);
        fog_planar.red = clamp01(fog_planar.red);
        fog_planar.green = clamp01(fog_planar.green);
        fog_planar.blue = clamp01(fog_planar.blue);
        fog_add.red = density * fog->atmospheric_color.red;
        fog_add.green = fog->atmospheric_color.green * density;
        fog_add.blue = fog->atmospheric_color.blue * density;
        if (rasterizer_caps.pixel_shader_version < 0xffff0104) {
            if (model->detail_mask != 0 && vertex_shader == 0x19) {
                ColorRGB fixed_function_fog;

                fixed_function_fog.red = clamp01(clamp01(fog_add.red - unknown_007c047c * fog_negative.red) + fog_planar.red);
                fixed_function_fog.green = clamp01(clamp01(fog_add.green - unknown_007c047c * fog_negative.green) +
                                                   fog_planar.green);
                fixed_function_fog.blue = clamp01(clamp01(fog_add.blue - unknown_007c047c * fog_negative.blue) +
                                                  fog_planar.blue);
                set_render_state(0x1c, 1);
                set_render_state(0x22, color_rgb_float_to_int(&fixed_function_fog));
            } else {
                set_render_state(0x1c, 0);
            }
        }
    }
    if (slot->constant_handles != 0) {
        set_effect_vector(slot, 0, change.red, change.green, change.blue,
                          rasterizer_camouflage_fade_active ? rasterizer_camouflage_fade : 1.0f);
        set_effect_vector(slot, 1, fog_planar.red, fog_planar.green, fog_planar.blue, fog_keep);
        set_effect_vector(slot, 2, fog_negative.red, fog_negative.green, fog_negative.blue, 1.0f);
        set_effect_vector(slot, 3, fog_add.red, fog_add.green, fog_add.blue, 1.0f);
    }

    if (((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[4].declaration) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[vertex_shader].shader) < 0) {
        ok = 0;
    }

    // c10 detail scales, c11/c12 texture animation (c12.z translucency), c13/c14 reflection
    context = rasterizer_active_model_context;
    {
        const ColorARGB *tint = &context->lighting.reflection_tint;
        float perpendicular[4], parallel[4];

        perpendicular[3] = reflection * model->perpendicular_brightness * tint->alpha;
        perpendicular[0] = model->perpendicular_tint_color.red * tint->red;
        perpendicular[1] = model->perpendicular_tint_color.green * tint->green;
        perpendicular[2] = model->perpendicular_tint_color.blue * tint->blue;
        parallel[3] = reflection * model->parallel_brightness * tint->alpha;
        parallel[0] = model->parallel_tint_color.red * tint->red;
        parallel[1] = model->parallel_tint_color.green * tint->green;
        parallel[2] = model->parallel_tint_color.blue * tint->blue;
        reflection_constants[0][0] = perpendicular[0] - parallel[0];
        reflection_constants[0][1] = perpendicular[1] - parallel[1];
        reflection_constants[0][2] = perpendicular[2] - parallel[2];
        reflection_constants[0][3] = perpendicular[3] - parallel[3];
        reflection_constants[1][0] = parallel[0];
        reflection_constants[1][1] = parallel[1];
        reflection_constants[1][2] = parallel[2];
        reflection_constants[1][3] = parallel[3];
    }
    detail_constants[0][0] = model->detail_map_scale;
    detail_constants[0][1] = model->detail_map_v_scale * model->detail_map_scale;
    detail_constants[0][2] = 1.0f;
    detail_constants[0][3] = 1.0f;
    detail_constants[1][0] = 1.0f;
    detail_constants[1][1] = 0.0f;
    detail_constants[1][2] = 0.0f;
    detail_constants[1][3] = 0.0f;
    detail_constants[2][0] = 0.0f;
    detail_constants[2][1] = 1.0f;
    detail_constants[2][2] = 0.0f;
    detail_constants[2][3] = 0.0f;
    shader_texture_animation_evaluate(&context->unknown_84[0], shader + 0xfc, detail_constants[1], detail_constants[2],
                                      context->unknown_c4 * model->map_u_scale, context->unknown_c8 * model->map_v_scale,
                                      0.0f, 0.0f, 0.0f, (float)rasterizer_time.time);
    detail_constants[2][2] = model->translucency;
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, &detail_constants[0][0], 3) < 0) {
        ok = 0;
    }
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &reflection_constants[0][0], 2) < 0) {
        ok = 0;
    }
    if (unknown_0071cfc0 != NULL &&
        (unknown_0071cfc0[0] > 0.0f || unknown_0071cfc0[1] > 0.0f ||
         unknown_0071cfc0[2] > 0.0f || unknown_0071cfc0[3] > 0.0f)) {
        float override_constants[2][4];

        override_constants[0][0] = 0.0f;
        override_constants[0][1] = 0.0f;
        override_constants[0][2] = 0.0f;
        override_constants[0][3] = 0.0f;
        override_constants[1][0] = unknown_0071cfc0[0];
        override_constants[1][1] = unknown_0071cfc0[1];
        override_constants[1][2] = unknown_0071cfc0[2];
        override_constants[1][3] = unknown_0071cfc0[3];
        if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &override_constants[0][0], 2) < 0) {
            ok = 0;
            goto done;
        }
    }
    if (!ok) {
        goto done;
    }

    ((d3dx_effect_begin_fn)(*(void ***)(uintptr_t)slot->effect)[0x100 / 4])((void *)(uintptr_t)slot->effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)(uintptr_t)slot->effect)[0x104 / 4])((void *)(uintptr_t)slot->effect, pass);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
    }
    ((d3dx_effect_end_fn)(*(void ***)(uintptr_t)slot->effect)[0x108 / 4])((void *)(uintptr_t)slot->effect);

    if ((shader[0x28] & 2) && cull) {
        // two sided: back faces with c10.w = -1
        context = rasterizer_active_model_context;
        detail_constants[0][0] = model->detail_map_scale;
        detail_constants[0][1] = model->detail_map_v_scale * model->detail_map_scale;
        detail_constants[0][2] = 1.0f;
        detail_constants[0][3] = -1.0f;
        detail_constants[1][0] = 1.0f;
        detail_constants[1][1] = 0.0f;
        detail_constants[1][2] = 0.0f;
        detail_constants[1][3] = 0.0f;
        detail_constants[2][0] = 0.0f;
        detail_constants[2][1] = 1.0f;
        detail_constants[2][2] = 0.0f;
        detail_constants[2][3] = 0.0f;
        shader_texture_animation_evaluate(&context->unknown_84[0], shader + 0xfc, detail_constants[1], detail_constants[2],
                                          context->unknown_c4 * model->map_u_scale,
                                          context->unknown_c8 * model->map_v_scale, 0.0f, 0.0f, 0.0f,
                                          (float)rasterizer_time.time);
        detail_constants[2][2] = model->translucency;
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, &detail_constants[0][0], 3);
        ((d3dx_effect_begin_fn)(*(void ***)(uintptr_t)slot->effect)[0x100 / 4])((void *)(uintptr_t)slot->effect, &passes, 3);
        for (pass = 0; pass < passes; pass++) {
            ((d3dx_effect_pass_fn)(*(void ***)(uintptr_t)slot->effect)[0x104 / 4])((void *)(uintptr_t)slot->effect, pass);
            set_render_state(0x16, 2);   // CULLMODE CW
            // the draw dispatch 0x51c730 inlined, first primitive 0
            if (index_buffer != NULL) {
                if (vertex_buffer != NULL) {
                    rasterizer_dynamic_geometry_chain_draw(primitive_count, vertex_buffer, index_buffer);
                } else {
                    rasterizer_dynamic_vertex_draw_indexed(index_buffer, primitive_count, dynamic_vertex_slot);
                }
            } else if (vertex_buffer != NULL) {
                chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer,
                                                                           dynamic_index_slot, 0);
            } else {
                rasterizer_dynamic_index_cache_draw(dynamic_index_slot, 0, primitive_count, dynamic_vertex_slot);
            }
        }
        ((d3dx_effect_end_fn)(*(void ***)(uintptr_t)slot->effect)[0x108 / 4])((void *)(uintptr_t)slot->effect);
    }

done:
    if (rasterizer_caps.raster_caps & 0x04000000) {
        set_render_state(0xc3, 0);
    }
    if (rasterizer_caps.raster_caps & 0x02000000) {
        set_render_state(0xaf, 0);
    }
}

#if 0
Original Ghidra decompilation (0x529e00):

/* WARNING: Removing unreachable block (ram,0x0052a18f) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void rasterizer_shader_environment_draw_pixelshader
               (int param_1,int *param_2,int param_3,undefined4 param_4,undefined4 param_5,
               int param_6,undefined4 param_7)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  short sVar6;
  int *piVar7;
  byte bVar8;
  int *piVar9;
  int iVar10;
  ushort uVar11;
  byte *pbVar12;
  uint uVar13;
  undefined4 unaff_EDI;
  uint uVar14;
  float10 fVar15;
  int *piStack_160;
  uint uStack_15c;
  float *pfStack_158;
  undefined4 uStack_154;
  int *piStack_150;
  undefined *puStack_14c;
  int *piStack_148;
  int *piStack_144;
  uint uStack_128;
  int *piStack_124;
  float fStack_120;
  float fStack_11c;
  int *piStack_118;
  float fStack_114;
  float fStack_110;
  int *piStack_10c;
  float fStack_108;
  float fStack_104;
  undefined4 uStack_100;
  float fStack_fc;
  float fStack_f8;
  int *piStack_f4;
  float fStack_f0;
  float fStack_ec;
  int *piStack_e8;
  float fStack_e4;
  undefined4 uStack_e0;
  undefined4 uStack_dc;
  float fStack_d8;
  float fStack_d4;
  float fVar16;
  float fVar17;
  undefined4 uStack_c4;
  float local_ac;
  int *local_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  float fStack_9c;
  float fStack_98;
  float fStack_94;
  float fStack_90;
  float fStack_8c;
  float fStack_88;
  float fStack_84;
  float fStack_80;
  uint uStack_6c;
  
  uStack_c4._3_1_ = *(byte *)(param_1 + 0x28) >> 3 & 1;
  fStack_d4 = 7.587249e-39;
  piVar9 = (int *)rasterizer_shader_environment_select_technique_texture();
  if (piVar9 == (int *)0x0) {
    return;
  }
  if (*piVar9 == 0) {
    return;
  }
  bVar8 = 1;
  fVar16 = (*(float *)(DAT_0071d1f0 + 0xb4) - DAT_007c1228) * DAT_007c1234 +
           (*(float *)(DAT_0071d1f0 + 0xbc) - DAT_007c1230) * DAT_007c123c +
           (*(float *)(DAT_0071d1f0 + 0xb8) - DAT_007c122c) * DAT_007c1238;
  if (*(float *)(param_1 + 0x140) == 0.0) {
LAB_00529f17:
    local_ac = 1.0;
  }
  else if (0.0 <= (fVar16 - *(float *)(param_1 + 0x140)) /
                  (*(float *)(param_1 + 0x13c) - *(float *)(param_1 + 0x140))) {
    if (1.0 < (fVar16 - *(float *)(param_1 + 0x140)) /
              (*(float *)(param_1 + 0x13c) - *(float *)(param_1 + 0x140))) goto LAB_00529f17;
    local_ac = (fVar16 - *(float *)(param_1 + 0x140)) /
               (*(float *)(param_1 + 0x13c) - *(float *)(param_1 + 0x140));
  }
  else {
    local_ac = 0.0;
  }
  local_a8 = piVar9;
  if ((*DAT_0071d1f0 & 8) == 0) {
    if (DAT_0071d1fe == '\0') {
      fStack_d4 = 1.4013e-45;
      fStack_d8 = 9.80909e-45;
      uStack_dc = DAT_0071d174;
      uStack_e0 = 0x529f53;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_e0 = 4;
      fStack_e4 = 3.22299e-44;
      piStack_e8 = DAT_0071d174;
      fStack_ec = 7.58769e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_ec = (float)(uint)(uStack_dc._3_1_ == '\0');
      fStack_f0 = 1.96182e-44;
      piStack_f4 = DAT_0071d174;
      fStack_f8 = 7.587727e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
    }
    if (uStack_c4._3_1_ == 0) {
      fStack_d4 = 7.587755e-39;
      rasterizer_clear_decal_zbias();
    }
    else {
      fStack_d4 = 7.587745e-39;
      FUN_005194e0();
    }
  }
  else {
    fStack_d4 = 0.0;
    fStack_d8 = 9.80909e-45;
    uStack_dc = DAT_0071d174;
    uStack_e0 = 0x529f36;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  if ((*(ushort *)(param_1 + 0x28) & 2) != 0) {
    if ((*(ushort *)(param_1 + 0x28) & 0x20) != 0) {
      fStack_d4 = 7.5878e-39;
      fVar15 = (float10)vector3d_distance();
      bVar8 = 0;
      if ((float10)8.0 < fVar15) goto LAB_00529fca;
    }
    bVar8 = 1;
  }
LAB_00529fca:
  fStack_d4 = (float)((uint)bVar8 * 2 + 1);
  fStack_d8 = 3.08286e-44;
  uStack_dc = DAT_0071d174;
  uStack_e0 = 0x529fe8;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if (DAT_0071d1fe == '\0') {
    uStack_e0 = 7;
    fStack_e4 = 2.35418e-43;
    piStack_e8 = DAT_0071d174;
    fStack_ec = 7.587916e-39;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  if (((char)((uint)unaff_EDI >> 0x18) == '\0') && (DAT_0071d1fe == '\0')) {
    uStack_e0 = 0;
  }
  else {
    uStack_e0 = 1;
  }
  fStack_e4 = 3.78351e-44;
  piStack_e8 = DAT_0071d174;
  fStack_ec = 7.587977e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_ec = 7.00649e-45;
  fStack_f0 = 2.66247e-44;
  piStack_f4 = DAT_0071d174;
  fStack_f8 = 7.588002e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_f8 = 8.40779e-45;
  fStack_fc = 2.8026e-44;
  uStack_100 = DAT_0071d174;
  fStack_104 = 7.588027e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_104 = 1.4013e-45;
  fStack_108 = 2.39622e-43;
  piStack_10c = DAT_0071d174;
  fStack_110 = 7.588056e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if (((DAT_0071d1fe == '\0') && (uStack_100._3_1_ == '\0')) &&
     ((*(byte *)(param_1 + 0x28) & 4) == 0)) {
    fStack_110 = 1.4013e-45;
  }
  else {
    fStack_110 = 0.0;
  }
  fStack_114 = 2.10195e-44;
  piStack_118 = DAT_0071d174;
  fStack_11c = 7.588125e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_11c = 1.77965e-43;
  fStack_120 = 3.36312e-44;
  piStack_124 = DAT_0071d174;
  uStack_128 = 0x52a0ad;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if (DAT_007c118c < 0xffff0104) {
    uStack_128 = (uint)(DAT_0069c6a8 != '\0');
  }
  else {
    uStack_128 = (*(byte *)(param_1 + 0x28) & 0x10) >> 4;
  }
  (**(code **)(*DAT_0071d174 + 0xe4))();
  piStack_144 = (int *)0x52a0f6;
  FUN_00518860();
  piStack_148 = param_2;
  puStack_14c = (undefined *)0x2;
  piStack_150 = (int *)0x1;
  uStack_154 = (int *)0x52a109;
  piStack_144 = piVar9;
  FUN_00518860();
  pfStack_158 = (float *)param_2;
  uStack_15c = 1;
  piStack_160 = (int *)0x2;
  uStack_154 = piVar9;
  FUN_00518860();
  FUN_00518860(3,0,param_2,piVar9);
  pbVar12 = DAT_0071d1f0;
  if ((0xffff0103 < DAT_007c118c) || (*(short *)(param_1 + 0xd6) == 0)) {
    fStack_120 = 1.0;
    if ((*(byte *)(param_1 + 0x6c) & 1) == 0) {
      uStack_6c = *(int *)(DAT_0071d1f0 + 4) * 0x19660d + 0x3c6ef35fU >> 0x10;
    }
    fVar16 = *(float *)(param_1 + 0x84);
    fVar17 = *(float *)(param_1 + 0x78);
    fVar2 = *(float *)(param_1 + 0x88);
    fVar3 = *(float *)(param_1 + 0x7c);
    fVar4 = *(float *)(param_1 + 0x8c);
    fVar5 = *(float *)(param_1 + 0x80);
    fVar15 = (float10)periodic_function_evaluate();
    sVar6 = *(short *)(param_1 + 0x70);
    piStack_118 = (int *)(float)((float10)(fVar16 - fVar17) * fVar15 +
                                (float10)*(float *)(param_1 + 0x78));
    fStack_114 = (float)((float10)(fVar2 - fVar3) * fVar15 + (float10)*(float *)(param_1 + 0x7c));
    fStack_110 = (float)((float10)(fVar4 - fVar5) * fVar15 + (float10)*(float *)(param_1 + 0x80));
    if ((0 < sVar6) && (sVar6 < 5)) {
      pfVar1 = (float *)(*(int *)(pbVar12 + 0x84) + -0xc + sVar6 * 0xc);
      piStack_118 = (int *)((float)piStack_118 * *pfVar1);
      fStack_114 = fStack_114 * pfVar1[1];
      fStack_110 = fStack_110 * pfVar1[2];
      if ((*(short *)(param_1 + 0xd6) == 0) && ((sVar6 == 2 && (DAT_007c118c < 0xffff0104)))) {
        fVar15 = (float10)vector3d_distance();
        if (ABS(fVar15) < (float10)6.0) {
          fStack_120 = (float)((float10)1.0 - ABS(fVar15) * (float10)0.16666667);
        }
      }
    }
    _DAT_006e17f0 = 1.0;
    _DAT_006e17e4 = (int *)((float)piStack_118 * fStack_120);
    _DAT_006e17e8 = (int *)(fStack_114 * fStack_120);
    _DAT_006e17ec = (undefined *)(fStack_110 * fStack_120);
    if (piVar9[6] != 0) {
      (**(code **)(*(int *)*piVar9 + 0x88))();
      pbVar12 = DAT_0071d1f0;
    }
  }
  sVar6 = *(short *)(param_1 + 0x4c);
  if ((sVar6 < 1) || (4 < sVar6)) {
    fVar16 = *(float *)PTR_DAT_00686b04;
    fVar17 = *(float *)(PTR_DAT_00686b04 + 4);
    uStack_c4 = *(float *)(PTR_DAT_00686b04 + 8);
  }
  else {
    pfVar1 = (float *)(*(int *)(pbVar12 + 0x84) + -0xc + sVar6 * 0xc);
    fVar16 = *pfVar1;
    fVar17 = pfVar1[1];
    uStack_c4 = pfVar1[2];
  }
  uVar11 = *(ushort *)(param_1 + 0x28) & 0x10;
  if (uVar11 == 0) {
    if (DAT_0071d1fb == '\0') {
      if (*(short *)(pbVar12 + 0x50) < 1) {
        if ((1 < *(short *)(pbVar12 + 0xc)) ||
           (((*(int *)(param_1 + 200) != -1 &&
             (((((*(short *)(param_1 + 0xd6) != 0 || ((float)piStack_118 != 0.0)) ||
                (fStack_114 != 0.0)) || ((fStack_110 != 0.0 || (fVar16 != 1.0)))) ||
              ((fVar17 != 1.0 || (uStack_c4 != 1.0)))))) ||
            ((*(int *)(param_1 + 0x170) != -1 && (0.0 < (float)piStack_10c)))))) goto LAB_0052a432;
        fStack_120 = 4.06377e-44;
      }
      else {
        fStack_120 = 3.64338e-44;
      }
    }
    else {
      fStack_120 = 3.50325e-44;
    }
  }
  else {
LAB_0052a432:
    fStack_120 = 3.92364e-44;
  }
  if (DAT_0069c6a8 == '\0') {
LAB_0052a967:
    piStack_118 = (int *)0x0;
    fStack_104 = 0.0;
    piStack_f4 = (int *)0x0;
    fStack_114 = 0.0;
    uStack_100 = (int *)0x0;
    fStack_f0 = 0.0;
    fStack_110 = 0.0;
    fStack_fc = 0.0;
    fStack_ec = 0.0;
    fStack_f8 = 1.0;
  }
  else if (uVar11 == 0) {
    if ((*pbVar12 & 4) != 0) goto LAB_0052a967;
    fVar2 = ((_DAT_007c142c * DAT_007c122c +
             _DAT_007c1430 * DAT_007c1230 + _DAT_007c1428 * DAT_007c1228) - _DAT_007c1434) /
            DAT_007c1420;
    if (0.0 <= fVar2) {
      if (1.0 < fVar2) {
        fVar2 = 1.0;
      }
    }
    else {
      fVar2 = 0.0;
    }
    fStack_11c = (fStack_11c - DAT_007c141c) / (DAT_007c1420 - DAT_007c141c);
    if (0.0 <= fStack_11c) {
      if (1.0 < fStack_11c) {
        fStack_11c = 1.0;
      }
    }
    else {
      fStack_11c = 0.0;
    }
    fStack_11c = fStack_11c * DAT_007c1418;
    if ((DAT_007c1408 & 2) != 0) {
      fVar2 = 1.0;
    }
    fStack_f8 = 1.0 - fStack_11c;
    fVar3 = 1.0 - fVar2;
    piStack_f4 = (int *)(DAT_007c1438 - (fVar2 * DAT_007c1438 + DAT_007c140c * fVar3) * fStack_11c);
    fStack_f0 = DAT_007c143c - (fVar3 * DAT_007c1410 + DAT_007c143c * fVar2) * fStack_11c;
    fStack_ec = DAT_007c1440 - (fVar3 * DAT_007c1414 + DAT_007c1440 * fVar2) * fStack_11c;
    fStack_104 = -(float)piStack_f4;
    if (0.0 <= fStack_104) {
      if (1.0 < fStack_104) {
        fStack_104 = 1.0;
      }
    }
    else {
      fStack_104 = 0.0;
    }
    uStack_100 = (int *)-fStack_f0;
    if (0.0 <= (float)uStack_100) {
      if (1.0 < (float)uStack_100) {
        uStack_100 = (int *)0x3f800000;
      }
    }
    else {
      uStack_100 = (int *)0x0;
    }
    fStack_fc = -fStack_ec;
    if (0.0 <= fStack_fc) {
      if (1.0 < fStack_fc) {
        fStack_fc = 1.0;
      }
    }
    else {
      fStack_fc = 0.0;
    }
    if (0.0 <= (float)piStack_f4) {
      if (1.0 < (float)piStack_f4) {
        piStack_f4 = (int *)0x3f800000;
      }
    }
    else {
      piStack_f4 = (int *)0x0;
    }
    if (0.0 <= fStack_f0) {
      if (1.0 < fStack_f0) {
        fStack_f0 = 1.0;
      }
    }
    else {
      fStack_f0 = 0.0;
    }
    if (0.0 <= fStack_ec) {
      if (1.0 < fStack_ec) {
        fStack_ec = 1.0;
      }
    }
    else {
      fStack_ec = 0.0;
    }
    piStack_118 = (int *)(fStack_11c * DAT_007c140c);
    fStack_114 = DAT_007c1410 * fStack_11c;
    fStack_110 = DAT_007c1414 * fStack_11c;
    if (DAT_007c118c < 0xffff0104) {
      if ((*(short *)(param_1 + 0xd6) == 0) || (SUB42(fStack_120,0) != 0x19)) {
        (**(code **)(*DAT_0071d174 + 0xe4))();
      }
      else {
        fStack_d8 = (float)piStack_118 - _DAT_007c047c * fStack_104;
        if (0.0 <= fStack_d8) {
          if (1.0 < fStack_d8) {
            fStack_d8 = 1.0;
          }
        }
        else {
          fStack_d8 = 0.0;
        }
        fStack_d4 = fStack_114 - _DAT_007c047c * (float)uStack_100;
        if (0.0 <= fStack_d4) {
          if (1.0 < fStack_d4) {
            fStack_d4 = 1.0;
          }
        }
        else {
          fStack_d4 = 0.0;
        }
        fStack_d8 = fStack_d8 + (float)piStack_f4;
        if (0.0 <= fStack_d8) {
          if (1.0 < fStack_d8) {
            fStack_d8 = 1.0;
          }
        }
        else {
          fStack_d8 = 0.0;
        }
        fStack_d4 = fStack_d4 + fStack_f0;
        if (0.0 <= fStack_d4) {
          if (1.0 < fStack_d4) {
            fStack_d4 = 1.0;
          }
        }
        else {
          fStack_d4 = 0.0;
        }
        (**(code **)(*DAT_0071d174 + 0xe4))();
        piStack_144 = (int *)0x52a950;
        color_rgb_float_to_int(&fStack_e4);
        piStack_144 = (int *)0x22;
        piStack_148 = DAT_0071d174;
        puStack_14c = (undefined *)0x52a965;
        (**(code **)(*DAT_0071d174 + 0xe4))();
      }
    }
  }
  else if (DAT_007c118c < 0xffff0104) {
    (**(code **)(*DAT_0071d174 + 0xe4))();
    piStack_124 = (int *)0x0;
    uStack_100 = (int *)0x0;
    fStack_120 = 0.0;
    fStack_fc = 0.0;
    fStack_11c = 0.0;
    fStack_f8 = 0.0;
    fStack_104 = 1.0;
    piStack_144 = (int *)0x52a4b1;
    color_rgb_float_to_int(&DAT_007c140c);
    piStack_144 = (int *)0x22;
    piStack_148 = DAT_0071d174;
    puStack_14c = (undefined *)0x52a4c6;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  if (piVar9[6] != 0) {
    _DAT_006e17ec = (undefined *)uStack_c4;
    if (DAT_0071d1fe == '\0') {
      _DAT_006e17f0 = 1.0;
    }
    else {
      _DAT_006e17f0 = (float)DAT_0071d200;
    }
    fVar2 = *(float *)piVar9[6];
    piVar7 = (int *)*piVar9;
    _DAT_006e17e4 = (int *)fVar16;
    _DAT_006e17e8 = (int *)fVar17;
    (**(code **)(*piVar7 + 0x88))();
    _DAT_006e17e4 = uStack_100;
    _DAT_006e17e8 = (int *)fStack_fc;
    _DAT_006e17ec = (undefined *)fStack_f8;
    _DAT_006e17f0 = fStack_104;
    piStack_144 = *(int **)(piVar9[6] + 4);
    piStack_148 = (int *)*piVar9;
    puStack_14c = (undefined *)0x52aa53;
    (**(code **)(*piStack_148 + 0x88))();
    _DAT_006e17e4 = (int *)fStack_11c;
    _DAT_006e17e8 = piStack_118;
    _DAT_006e17ec = (undefined *)fStack_114;
    _DAT_006e17f0 = 1.0;
    piStack_150 = *(int **)(piVar9[6] + 8);
    uStack_154 = (int *)*piVar9;
    puStack_14c = &DAT_006e17e4;
    pfStack_158 = (float *)0x52aa91;
    (**(code **)(*uStack_154 + 0x88))();
    _DAT_006e17ec = &DAT_006e17e4;
    _DAT_006e17f0 = 1.0;
    uStack_15c = *(undefined4 *)(piVar9[6] + 0xc);
    piStack_160 = (int *)*piVar9;
    pfStack_158 = (float *)&DAT_006e17e4;
    _DAT_006e17e4 = piVar7;
    _DAT_006e17e8 = (int *)fVar2;
    (**(code **)(*piStack_160 + 0x88))();
  }
  piVar9 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x15c))();
  piStack_144 = (int *)0x52ab09;
  (**(code **)(*DAT_0071d174 + 0x170))();
  fStack_108 = fStack_11c * *(float *)(param_1 + 0x144) * *(float *)(DAT_0071d1f0 + 0x5c);
  fStack_104 = *(float *)(param_1 + 0x148) * *(float *)(DAT_0071d1f0 + 0x60);
  local_ac = 0.0;
  uStack_100 = (int *)(*(float *)(param_1 + 0x14c) * *(float *)(DAT_0071d1f0 + 100));
  local_a8 = (int *)0x3f800000;
  uStack_a4 = 0;
  uStack_a0 = 0;
  fStack_fc = *(float *)(param_1 + 0x150) * *(float *)(DAT_0071d1f0 + 0x68);
  fStack_80 = fStack_11c * *(float *)(param_1 + 0x154) * *(float *)(DAT_0071d1f0 + 0x5c);
  fStack_8c = *(float *)(param_1 + 0x158) * *(float *)(DAT_0071d1f0 + 0x60);
  fStack_88 = *(float *)(param_1 + 0x15c) * *(float *)(DAT_0071d1f0 + 100);
  fStack_84 = *(float *)(param_1 + 0x160) * *(float *)(DAT_0071d1f0 + 0x68);
  fStack_9c = fStack_104 - fStack_8c;
  fStack_98 = (float)uStack_100 - fStack_88;
  fStack_94 = fStack_fc - fStack_84;
  fStack_90 = fStack_108 - fStack_80;
  piStack_144 = (int *)(float)_DAT_007c1200;
  piStack_148 = (int *)0x0;
  puStack_14c = (undefined *)0x0;
  uStack_154 = (int *)(*(float *)(DAT_0071d1f0 + 200) * *(float *)(param_1 + 0xa0));
  piStack_150 = (int *)0x0;
  pfStack_158 = (float *)(*(float *)(DAT_0071d1f0 + 0xc4) * *(float *)(param_1 + 0x9c));
  uStack_15c = 0x52acb0;
  fStack_11c = (float)(param_1 + 0xfc);
  shader_texture_animation_evaluate();
  uStack_a4 = *(undefined4 *)(param_1 + 0x38);
  piStack_144 = (undefined4 *)0x3;
  piStack_148 = (int *)&stack0xffffff34;
  puStack_14c = (undefined *)0xa;
  piStack_150 = DAT_0071d174;
  uStack_154 = (int *)0x52acd7;
  iVar10 = (**(code **)(*DAT_0071d174 + 0x178))();
  if (iVar10 < 0) {
    piStack_144._0_3_ = (uint3)(ushort)piStack_144;
  }
  uStack_154 = (int *)0x2;
  pfStack_158 = &local_ac;
  uStack_15c = 0xd;
  piStack_160 = DAT_0071d174;
  iVar10 = (**(code **)(*DAT_0071d174 + 0x178))();
  if (iVar10 < 0) {
    uStack_154._0_3_ = (uint3)(ushort)uStack_154;
  }
  if ((DAT_0071cfc0 != (float *)0x0) &&
     ((((0.0 < *DAT_0071cfc0 || (0.0 < DAT_0071cfc0[1])) || (0.0 < DAT_0071cfc0[2])) ||
      (0.0 < DAT_0071cfc0[3])))) {
    piStack_118 = (int *)*DAT_0071cfc0;
    fStack_114 = DAT_0071cfc0[1];
    fStack_110 = DAT_0071cfc0[2];
    piStack_10c = (int *)DAT_0071cfc0[3];
    uStack_128 = 0;
    piStack_124 = (int *)0x0;
    fStack_120 = 0.0;
    fStack_11c = 0.0;
    iVar10 = (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&uStack_128,2);
    if (iVar10 < 0) goto LAB_0052aff9;
  }
  if (uStack_154._2_1_ != '\0') {
    (**(code **)(*(int *)*piVar9 + 0x100))((int *)*piVar9,&piStack_150,3);
    uVar14 = 0;
    if (uStack_15c != 0) {
      do {
        (**(code **)(*(int *)*piStack_144 + 0x104))((int *)*piStack_144,uVar14);
        rasterizer_dynamic_geometry_draw_dispatch(param_3,param_4,param_6);
        uVar14 = uVar14 + 1;
      } while (uVar14 < uStack_15c);
    }
    (**(code **)(*(int *)*piStack_144 + 0x108))((int *)*piStack_144);
    if (((*(byte *)(param_1 + 0x28) & 2) != 0) && (uStack_154._1_1_ != '\0')) {
      piStack_e8 = (int *)(*(float *)(param_1 + 0xec) * *(float *)(param_1 + 0xd8));
      fStack_ec = *(float *)(param_1 + 0xd8);
      fStack_e4 = 1.0;
      uStack_e0 = 0xbf800000;
      uStack_dc = (int *)0x3f800000;
      fStack_d8 = 0.0;
      fStack_d4 = 0.0;
      shader_texture_animation_evaluate
                (*(float *)(DAT_0071d1f0 + 0xc4) * *(float *)(param_1 + 0x9c),
                 *(float *)(DAT_0071d1f0 + 200) * *(float *)(param_1 + 0xa0),0,0,0,
                 (float)_DAT_007c1200);
      uVar14 = 10;
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&fStack_ec,3);
      piVar9 = piStack_148;
      (**(code **)(*(int *)*piStack_148 + 0x100))((int *)*piStack_148,&piStack_160,3);
      uStack_154 = (int *)0x0;
      if (uVar14 != 0) {
        do {
          uVar13 = (uint)uStack_154;
          piVar7 = (int *)*piVar9;
          (**(code **)(*piVar7 + 0x104))(piVar7,uStack_154);
          (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,2);
          if (param_3 == 0) {
            if (param_6 == 0) {
              FUN_0051c090(param_4,0,param_5,param_7);
            }
            else {
              chimera__rasterizer_draw_dynamic_triangles_static_vertices(param_4,0);
              uVar13 = (uint)uStack_154;
            }
          }
          else if (param_6 == 0) {
            FUN_0051c490(param_3,param_5,param_7);
          }
          else {
            FUN_0051c5f0(param_5);
          }
          uStack_154 = (int *)(uVar13 + 1);
        } while (uStack_154 < uVar14);
      }
      piVar9 = (int *)*piVar9;
      (**(code **)(*piVar9 + 0x108))(piVar9);
    }
  }
LAB_0052aff9:
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
  }
  return;
}
#endif
