// rasterizer_shader_environment_draw_pixel_shader  (not a Ghidra function; 0x007c0470 on ps_1_1+ cards)
// address 0x528050, size 2692 bytes
// name confidence: 0.6  rewrite confidence: 0.8
// evidence: rasterizer_shader_environment_select_draw_functions 0x52b630 stores it in 0x007c0470 when the card has
//   more than one stream and pixel shaders 1.1+; rasterizer_shader_environment_draw_dispatch 0x52b050 calls that for
//   shader_type 3 (environment shaders drawn on a model part). Same argument list, fog math and effect-slot use as
//   its model twin rasterizer_shader_model_draw_pixel_shader 0x529e00. Campaign track: first object render.
// objdump 0x528050..0x528ad2:
//   Only with 0x006893ec set (else it returns at once). Depth: off for a model context flagged 8, else ZENABLE,
//   ZWRITEENABLE, ZFUNC LESSEQUAL and the decal z bias cleared. Then CULLMODE CCW, COLORWRITEENABLE 7, no alpha
//   blend (SRCALPHA/INVSRCALPHA, ADD), ALPHATESTENABLE from shader flags bit 0 with ALPHAREF 0x7f, and FOGENABLE
//   from shader flags bit 2 on ps_1_4+ cards (off below).
//   Vertex shader: 0x1c for flags bit 2; else 0x19 with 0x0071d1fb, 0x1a for a context with +0x50 > 0, 0x1c with a
//   bump map (+0x330), else 0x1d when the context's +0x0c word is at most 1 (0x1c otherwise).
//   The environment effect slot (0x0069e290) gets the technique from 0x006e180c (ps_1_4+ and flags bit 2 clear) or
//   0x006e17f4 by the shader type word (+0xb0), then its four maps (rasterizer_resolve_and_cache_submap_b): base
//   (+0x94, stage 0), primary detail (+0xc4, stage 1), micro detail (+0x134 with flags bit 0, else none, stage 2)
//   and the bump cube map (+0x330, type 2, stage 3). Vertex constants c10..c12 hold the map scales (+0xb4 twice, 1,
//   1 / the context's +0xc4 / +0xc8) and c13..c14 the scaled tint pair: (+0x2a8.. scaled by the context's
//   +0x5c..+0x68) minus (+0x2b4..) and the second set itself, with the w's from +0x2f4/+0x2f8.
//   Fog (as in the model draw): off, or with flags bit 2 on ps_1_4+ nothing; otherwise the planar/atmospheric
//   colors from the window fog (0x007c1408..) at the model's depth, and below ps_1_4 FOGCOLOR -- the atmospheric
//   color, or for vertex shader 0x19 the pack of (keep, fog color) after subtracting 0x007c047c times the negated
//   planar remainder. Four effect vectors (white, fog + keep, negated remainder, atmospheric add) go to the slot's
//   constant handles. The vertex shader and declaration (0x006e1ac0) are set, and each effect pass draws through
//   rasterizer_dynamic_geometry_draw_dispatch. The decal z bias is cleared at the end.
// blam-cc: stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h>

extern void *rasterizer_device;                      // 0x0071d174
extern uint8_t console_debug_toggle_6893ec;          // 0x006893ec
extern uint8_t *rasterizer_active_model_context;     // 0x0071d1f0
extern uint8_t unknown_0071d1fb;                     // 0x0071d1fb
extern uint8_t rasterizer_fog_enabled;               // 0x0069c6a8
extern float unknown_007c047c;                       // 0x007c047c
extern uint32_t rasterizer_pixel_shader_version;     // 0x007c118c
extern float rasterizer_camera_position[3];          // 0x007c1228
extern float rasterizer_camera_forward[3];           // 0x007c1234
extern uint8_t rasterizer_fog_flags;                 // 0x007c1408
extern ColorRGB rasterizer_fog_atmospheric_color;    // 0x007c140c
extern float rasterizer_fog_atmospheric_max_density; // 0x007c1418
extern float rasterizer_fog_atmospheric_min_distance;// 0x007c141c
extern float rasterizer_fog_atmospheric_max_distance;// 0x007c1420
extern float rasterizer_fog_plane[4];                // 0x007c1428 (normal, d)
extern ColorRGB rasterizer_fog_planar_color;         // 0x007c1438
extern rasterizer_effect_slot environment_effect_slot; // 0x0069e290
extern uint32_t environment_techniques_ps14[];       // 0x006e180c
extern int32_t environment_techniques_no[];          // 0x006e17f4
extern rasterizer_vertex_shader rasterizer_vertex_shaders[]; // 0x0069e350
extern uint32_t rasterizer_model_vertex_declaration; // 0x006e1ac0

extern void rasterizer_clear_decal_zbias(void); // 0x519580
extern uint32_t color_rgb_float_to_int(const ColorRGB *color); // 0x4ab5d0
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900
extern int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
    int16_t default_index, int16_t frame, rasterizer_effect_slot *effect_slot); // 0x518860
extern void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
    rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count, int32_t first_primitive,
    int32_t dynamic_vertex_slot); // 0x51c730

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_set_vector_fn)(void *effect, uint32_t handle, const float *vector);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **environment_device_vtable(void) { return *(void ***)rasterizer_device; }

static void environment_set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)environment_device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static float environment_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static void environment_set_vector(void *effect, uint32_t handle, float x, float y, float z, float w)
{
    float vector[4];

    vector[0] = x;
    vector[1] = y;
    vector[2] = z;
    vector[3] = w;
    ((d3dx_effect_set_vector_fn)(*(void ***)effect)[0x88 / 4])(effect, handle, vector);
}

void rasterizer_shader_environment_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
    int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_vertex_slot)
{
    uint8_t *context;
    float relative[3];
    uint16_t pixel_shader_fog = *(uint16_t *)(shader + 0x28) & 4;
    int16_t vertex_shader;
    uint8_t draw_ok = 1;
    void *effect;
    float c10_c12[12];
    float c13_c14[8];
    float a[4];
    float b[4];
    float fog[4];      // keep, planar remainder r g b (clamped)
    float negative[3]; // clamped negation of the planar remainder
    float add[3];      // atmospheric color x density
    uint32_t passes;
    uint32_t pass;

    if (!console_debug_toggle_6893ec) {
        return;
    }
    context = rasterizer_active_model_context;
    relative[0] = *(float *)(context + 0xb4) - rasterizer_camera_position[0];
    relative[1] = *(float *)(context + 0xb8) - rasterizer_camera_position[1];
    relative[2] = *(float *)(context + 0xbc) - rasterizer_camera_position[2];
    if (context[0] & 8) {
        environment_set_render_state(0x07, 0);
    } else {
        environment_set_render_state(0x07, 1);
        environment_set_render_state(0x0e, 1);
        environment_set_render_state(0x17, 4);
        rasterizer_clear_decal_zbias();
    }
    environment_set_render_state(0x16, 3);
    environment_set_render_state(0xa8, 7);
    environment_set_render_state(0x1b, 0);
    environment_set_render_state(0x13, 5);
    environment_set_render_state(0x14, 6);
    environment_set_render_state(0xab, 1);
    environment_set_render_state(0x0f, shader[0x28] & 1);
    environment_set_render_state(0x18, 0x7f);
    if (rasterizer_pixel_shader_version < 0xffff0104) {
        environment_set_render_state(0x1c, 0);
    } else {
        environment_set_render_state(0x1c, (shader[0x28] >> 2) & 1);
    }

    if (pixel_shader_fog) {
        vertex_shader = 0x1c;
    } else if (unknown_0071d1fb) {
        vertex_shader = 0x19;
    } else if (*(int16_t *)(context + 0x50) > 0) {
        vertex_shader = 0x1a;
    } else if (*(datum_index *)(shader + 0x330) != k_datum_index_none) {
        vertex_shader = 0x1c;
    } else if (*(int16_t *)(context + 0xc) <= 1) {
        vertex_shader = 0x1d;
    } else {
        vertex_shader = 0x1c;
    }

    effect = (void *)(uintptr_t)environment_effect_slot.effect;
    if (effect == 0) {
        rasterizer_clear_decal_zbias();
        return;
    }
    ((d3d_call1_fn)(*(void ***)effect)[0xec / 4])(effect,
        (rasterizer_pixel_shader_version >= 0xffff0104 && !pixel_shader_fog) ?
            environment_techniques_ps14[*(int16_t *)(shader + 0xb0)] :
            (uint32_t)environment_techniques_no[*(int16_t *)(shader + 0xb0)]);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame, &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xc4), 0, 1, 2, frame, &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b((shader[0x28] & 1) ? *(uint32_t *)(shader + 0x134) : 0xffffffff, 0, 2, 1, frame,
        &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x330), 2, 3, 0, frame, &environment_effect_slot);

    a[0] = *(float *)(shader + 0x2f4) * *(float *)(context + 0x5c);
    a[1] = *(float *)(shader + 0x2a8) * *(float *)(context + 0x60);
    a[2] = *(float *)(shader + 0x2ac) * *(float *)(context + 0x64);
    a[3] = *(float *)(shader + 0x2b0) * *(float *)(context + 0x68);
    b[0] = *(float *)(shader + 0x2f8) * *(float *)(context + 0x5c);
    b[1] = *(float *)(shader + 0x2b4) * *(float *)(context + 0x60);
    b[2] = *(float *)(shader + 0x2b8) * *(float *)(context + 0x64);
    b[3] = *(float *)(shader + 0x2bc) * *(float *)(context + 0x68);
    *(uint32_t *)&c10_c12[0] = *(uint32_t *)(shader + 0xb4);
    *(uint32_t *)&c10_c12[1] = *(uint32_t *)(shader + 0xb4);
    c10_c12[2] = 1.0f;
    c10_c12[3] = 1.0f;
    *(uint32_t *)&c10_c12[4] = *(uint32_t *)(context + 0xc4);
    c10_c12[5] = 0.0f;
    c10_c12[6] = 0.0f;
    c10_c12[7] = 0.0f;
    c10_c12[8] = 0.0f;
    *(uint32_t *)&c10_c12[9] = *(uint32_t *)(context + 0xc8);
    c10_c12[10] = 0.0f;
    c10_c12[11] = 0.0f;
    c13_c14[0] = a[1] - b[1];
    c13_c14[1] = a[2] - b[2];
    c13_c14[2] = a[3] - b[3];
    c13_c14[3] = a[0] - b[0];
    c13_c14[4] = b[1];
    c13_c14[5] = b[2];
    c13_c14[6] = b[3];
    c13_c14[7] = b[0];
    if (((d3d_set_constant_f_fn)environment_device_vtable()[0x178 / 4])(rasterizer_device, 10, c10_c12, 3) < 0) {
        draw_ok = 0;
    }
    if (((d3d_set_constant_f_fn)environment_device_vtable()[0x178 / 4])(rasterizer_device, 13, c13_c14, 2) < 0) {
        draw_ok = 0;
    }

    // The four fog locals share the stack with earlier values, which some paths leave in place: fog starts as
    // the tint pair a[] ([esp+0x10..0x1c]), negative as the camera-relative position ([esp+0x24..0x2c]); add is
    // never written on the ps_1_4 pixel-shader-fog path (uninitialized stack in the binary, 0 here).
    fog[0] = a[0];
    fog[1] = a[1];
    fog[2] = a[2];
    fog[3] = a[3];
    negative[0] = relative[0];
    negative[1] = relative[1];
    negative[2] = relative[2];
    add[0] = add[1] = add[2] = 0.0f;
    if (!rasterizer_fog_enabled || (!pixel_shader_fog && (context[0] & 4))) {
        fog[0] = 1.0f;
        fog[1] = fog[2] = fog[3] = 0.0f;
        negative[0] = negative[1] = negative[2] = 0.0f;
    } else if (pixel_shader_fog) {
        if (rasterizer_pixel_shader_version < 0xffff0104) {
            fog[0] = 1.0f;
            fog[1] = fog[2] = fog[3] = 0.0f;
            environment_set_render_state(0x22, color_rgb_float_to_int(&rasterizer_fog_atmospheric_color));
        }
    } else {
        {
            float height = environment_clamp01((rasterizer_fog_plane[2] * rasterizer_camera_position[2] +
                rasterizer_fog_plane[1] * rasterizer_camera_position[1] +
                rasterizer_fog_plane[0] * rasterizer_camera_position[0] - rasterizer_fog_plane[3]) /
                rasterizer_fog_atmospheric_max_distance);
            float depth = relative[2] * rasterizer_camera_forward[2] + relative[1] * rasterizer_camera_forward[1] +
                relative[0] * rasterizer_camera_forward[0];
            float density = environment_clamp01((depth - rasterizer_fog_atmospheric_min_distance) /
                (rasterizer_fog_atmospheric_max_distance - rasterizer_fog_atmospheric_min_distance)) *
                rasterizer_fog_atmospheric_max_density;
            float remainder[3];
            int32_t i;

            if (rasterizer_fog_flags & 2) {
                height = 1.0f;
            }
            fog[0] = 1.0f - density;
            remainder[0] = rasterizer_fog_planar_color.red - (height * rasterizer_fog_planar_color.red +
                (1.0f - height) * rasterizer_fog_atmospheric_color.red) * density;
            remainder[1] = rasterizer_fog_planar_color.green - (rasterizer_fog_atmospheric_color.green * (1.0f - height) +
                height * rasterizer_fog_planar_color.green) * density;
            remainder[2] = rasterizer_fog_planar_color.blue - (rasterizer_fog_planar_color.blue * height +
                (1.0f - height) * rasterizer_fog_atmospheric_color.blue) * density;
            for (i = 0; i < 3; i++) {
                negative[i] = environment_clamp01(-remainder[i]);
                fog[1 + i] = environment_clamp01(remainder[i]);
            }
            add[0] = density * rasterizer_fog_atmospheric_color.red;
            add[1] = rasterizer_fog_atmospheric_color.green * density;
            add[2] = rasterizer_fog_atmospheric_color.blue * density;
            if (rasterizer_pixel_shader_version < 0xffff0104) {
                if (vertex_shader != 0x19) {
                    environment_set_render_state(0x22, color_rgb_float_to_int(&rasterizer_fog_atmospheric_color));
                } else {
                    for (i = 0; i < 3; i++) {
                        add[i] = environment_clamp01(add[i] - unknown_007c047c * negative[i]);
                    }
                    environment_set_render_state(0x22, color_pack_argb_from_real((ColorARGB *)fog));
                }
            }
        }
    }

    if (environment_effect_slot.constant_handles != 0) {
        uint32_t *handles = (uint32_t *)(uintptr_t)environment_effect_slot.constant_handles;

        environment_set_vector(effect, handles[0], 1.0f, 1.0f, 1.0f, 1.0f);
        environment_set_vector(effect, handles[1], fog[1], fog[2], fog[3], fog[0]);
        environment_set_vector(effect, handles[2], negative[0], negative[1], negative[2], 1.0f);
        environment_set_vector(effect, handles[3], add[0], add[1], add[2], 1.0f);
    }

    if (((d3d_call1_fn)environment_device_vtable()[0x170 / 4])(rasterizer_device,
            rasterizer_vertex_shaders[vertex_shader].shader) < 0) {
        draw_ok = 0;
    }
    if (((d3d_call1_fn)environment_device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_model_vertex_declaration) >= 0 &&
        draw_ok) {
        ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
        for (pass = 0; pass < passes; pass++) {
            ((d3d_call1_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                dynamic_vertex_slot);
        }
        ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
    }
    rasterizer_clear_decal_zbias();
}
