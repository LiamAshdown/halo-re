// rasterizer_water_update_ripple_texture  (Ghidra: FUN_00534f80, never created as a function; called by the water draw
//   procedures when byte 0x71d275 asks for a refresh)
// address 0x534f80, size 2342 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x534f80..0x5358a5. With the water toggle (0x6893fe) and
//   rasterizer_effects[104] present, renders the water tag's ripple layers into render target 8:
//   - a two-triangle fan over the target at 0x6e1d60 (4 vertices of 0x18 bytes, shifted by half a texel);
//   - fixed render and sampler state, declaration 8 and vertex shader 0;
//   - up to four layers copied from the tag's ripple block (+0x124 count, +0x128 elements of 0x4c; missing layers
//     are zero with their +0x38 frame word 1). Layer +0x04 weights become c0.w = w0/(w0+w1), c1.w = w2/(w2+w3),
//     c2.w = (w0+w1)/(w0+w1+w2+w3) (a zero pair gets its second weight 1). Vertex constants c13.. hold, per layer,
//     {frame, 0, 0, cos(+0x28) * +0x2c * time + +0x30, 0, frame, 0, sin(+0x28) * +0x2c * time + +0x34};
//   - one pass per ripple count (+0xd8, at most 4): c3 = the packed (alpha, 0x80, 0x80, 0xff) colour for
//     i / (count - 1) * +0xdc (a single layer uses (128/255, 128/255, 1, 0)), each layer's ripple bitmap (+0xd4,
//     frame word +0x3a modulo the bitmap count, or the rasterizer globals' +0xb8 bitmap 3) on stages 0-3, every
//     effect pass drawing the fan;
//   then restores software vertex processing, the window's render target and shader stage config 2.
// blam-cc: stack -> water_shader (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "cache.h"
#include "bitmaps.h"
#include <string.h>

extern uint8_t rasterizer_water_enabled;              // 0x006893fe
extern uint8_t console_debug_toggle_689409;           // 0x00689409, "use the tag's ripple bitmap"
extern void *rasterizer_device;                       // 0x0071d174
extern GlobalsRasterizerData *rasterizer_globals_data; // 0x0071d164
extern rasterizer_frame_time rasterizer_time;         // 0x007c1200
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern uint32_t renderer_unknown_6e1af8;              // 0x006e1af8
extern int16_t rasterizer_active_render_target;       // 0x0069d350
extern int16_t rasterizer_bound_bitmap_size_a[2];     // 0x006d986c
extern tag_instance *tag_instances;                   // 0x0087bc14
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern float rasterizer_water_ripple_quad[4][6];      // 0x006e1d60

extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200, AX
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); // 0x52ccc0, EAX
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550
extern double sin(double x);
extern double cos(double x);
extern long lrint(double x); // x87 fistp under the default control word

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_call4v_fn)(void *self, uint32_t start_register, const void *data, uint32_t count);

#define DEVICE_CALL(index) ((*(void ***)rasterizer_device)[(index) / 4])
#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)DEVICE_CALL(0xe4))(rasterizer_device, state, value);
}

static void set_linear_clamped_stage(uint32_t stage)
{
    d3d_call3_fn set_sampler_state = (d3d_call3_fn)DEVICE_CALL(0x114);

    set_sampler_state(rasterizer_device, stage, 1, 1);
    set_sampler_state(rasterizer_device, stage, 2, 1);
    set_sampler_state(rasterizer_device, stage, 5, 2);
    set_sampler_state(rasterizer_device, stage, 6, 2);
    set_sampler_state(rasterizer_device, stage, 7, 2);
}

// 0x535725: bind a bitmap's texture on a stage and remember its size
static void bind_ripple_bitmap(uint32_t stage, uint8_t *bitmap)
{
    texture_cache_get((BitmapData *)bitmap, 1, 1);
    ((d3d_call2_fn)DEVICE_CALL(0x104))(rasterizer_device, stage, ((struct BitmapData *)bitmap)->hardware_texture);
    rasterizer_bound_bitmap_size_a[0] = *(int16_t *)&((struct BitmapData *)bitmap)->width;
    rasterizer_bound_bitmap_size_a[1] = *(int16_t *)&((struct BitmapData *)bitmap)->height;
}

void rasterizer_water_update_ripple_texture(void *water_shader)
{
    uint8_t *water = (uint8_t *)water_shader;
    void *effect;

    if (rasterizer_water_enabled == 0) {
        return;
    }
    effect = (void *)rasterizer_effects[104].effect;
    if (effect != 0) {
        static const float quad[4][6] = {
            {-1.0078125f, 1.0078125f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.9921875f, 1.0078125f, 0.0f, 0.0f, 1.0f, 0.0f},
            {0.9921875f, -0.9921875f, 0.0f, 0.0f, 1.0f, 1.0f},
            {-1.0078125f, -0.9921875f, 0.0f, 0.0f, 0.0f, 1.0f},
        };
        uint8_t layers[4][0x4c];     // esp+0xa4
        float pixel_constants[16];   // esp+0x2c: c0..c3
        float vertex_constants[32];  // esp+0x1d4: c13..c20
        int16_t ripple_count;        // esp+0x14
        int16_t pass_index;
        int32_t k;

        memcpy(rasterizer_water_ripple_quad, quad, sizeof quad);
        for (k = 0; k < 4; k++) {
            *(uint32_t *)&rasterizer_water_ripple_quad[k][3] = 0xffffffff; // the vertex colour
        }

        set_render_state(0x16, 3);
        set_render_state(0xa8, 7);
        set_render_state(0x1b, 0);
        set_render_state(0xf, 0);
        set_render_state(7, 0);
        set_render_state(0x1c, 0);
        set_linear_clamped_stage(0);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[8].declaration);
        ((d3d_call1_fn)DEVICE_CALL(0x134))(rasterizer_device,
            ((rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | renderer_unknown_6e1af8) & 0x10);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, rasterizer_vertex_shaders[0].shader);

        ripple_count = *(int16_t *)(water + 0xd8);
        if (ripple_count > 4) {
            ripple_count = 4;
        }
        memset(pixel_constants, 0, sizeof pixel_constants);
        set_linear_clamped_stage(1);
        set_linear_clamped_stage(2);
        set_linear_clamped_stage(3);

        for (k = 0; k < 4; k++) {
            if (k < *(int32_t *)(water + 0x124)) {
                memcpy(layers[k], *(uint8_t **)(water + 0x128) + k * 0x4c, 0x4c);
            } else {
                memset(layers[k], 0, 0x4c);
                *(int16_t *)(layers[k] + 0x38) = 1;
            }
        }
        if (F(layers[0], 0x4) == 0.0f && F(layers[1], 0x4) == 0.0f) {
            F(layers[1], 0x4) = 1.0f;
        }
        if (F(layers[2], 0x4) == 0.0f && F(layers[3], 0x4) == 0.0f) {
            F(layers[3], 0x4) = 1.0f;
        }

        // 0x535438: per-layer scroll constants
        for (k = 0; k < 4; k++) {
            float frame = (float)(int32_t)*(int16_t *)(layers[k] + 0x38);
            float angle = F(layers[k], 0x28);
            float *r = &vertex_constants[k * 8];

            r[0] = frame;
            r[1] = 0.0f;
            r[2] = 0.0f;
            r[3] = (float)(cos((double)angle) * F(layers[k], 0x2c) * rasterizer_time.time + F(layers[k], 0x30));
            r[4] = 0.0f;
            r[5] = frame;
            r[6] = 0.0f;
            r[7] = (float)(sin((double)angle) * F(layers[k], 0x2c) * rasterizer_time.time + F(layers[k], 0x34));
        }
        ((d3d_call4v_fn)DEVICE_CALL(0x178))(rasterizer_device, 0xd, vertex_constants, 8);

        {
            float sum01 = F(layers[1], 0x4) + F(layers[0], 0x4);
            float sum23 = F(layers[3], 0x4) + F(layers[2], 0x4);

            pixel_constants[3] = F(layers[0], 0x4) / sum01;
            pixel_constants[7] = F(layers[2], 0x4) / sum23;
            pixel_constants[11] = sum01 / (F(layers[1], 0x4) + F(layers[0], 0x4) + sum23);
        }
        rasterizer_set_shader_stage_config(0);

        for (pass_index = 0; pass_index < ripple_count; pass_index++) {
            uint32_t passes = 0;
            uint32_t pass;
            int32_t stage;

            if (*(int16_t *)(water + 0xd8) > 1) {
                float fraction = (float)(int32_t)pass_index / (float)(int32_t)(*(int16_t *)(water + 0xd8) - 1);
                float alpha = fraction * F(water, 0xdc);
                uint32_t packed = ((uint32_t)lrint((double)alpha * 255.0) << 24) | 0x8080ff; // fistp, round to nearest

                pixel_constants[12] = (float)(int32_t)((packed >> 16) & 0xff) * 0.003921569f;
                pixel_constants[13] = (float)(int32_t)((packed >> 8) & 0xff) * 0.003921569f;
                pixel_constants[14] = (float)(int32_t)(packed & 0xff) * 0.003921569f;
                pixel_constants[15] = fraction * F(water, 0xdc);
            } else {
                pixel_constants[12] = 0.5019608f;
                pixel_constants[13] = 0.5019608f;
                pixel_constants[14] = 1.0f;
                pixel_constants[15] = 0.0f;
            }

            // 0x535622: render into target 8 over its whole surface
            {
                void *surface = (void *)rasterizer_render_targets[8].surface;
                uint32_t desc[8];
                uint32_t viewport[6];

                ((d3d_call2_fn)DEVICE_CALL(0x94))(rasterizer_device, 0, (uint32_t)surface);
                rasterizer_active_render_target = 8;
                ((int32_t (__stdcall *)(void *, void *))(*(void ***)surface)[0x30 / 4])(surface, desc); // GetDesc
                viewport[0] = 0;
                viewport[1] = 0;
                viewport[2] = desc[6]; // Width
                viewport[3] = desc[7]; // Height
                *(float *)&viewport[4] = 0.0f;
                *(float *)&viewport[5] = 1.0f;
                ((d3d_call1_fn)DEVICE_CALL(0xbc))(rasterizer_device, (uint32_t)viewport);
            }

            for (stage = 0; stage < 4; stage++) {
                datum_index ripple_bitmap = (stage < *(int32_t *)(water + 0x124)) ?
                    *(datum_index *)(water + 0xd4) : k_datum_index_none;
                uint8_t bound = 0;

                if (console_debug_toggle_689409 != 0 && ripple_bitmap != k_datum_index_none) {
                    uint8_t *bitmap_tag = (uint8_t *)tag_instances[ripple_bitmap & 0xffff].data;
                    int32_t bitmap_count = *(int32_t *)(bitmap_tag + 0x60);

                    if (bitmap_count > 0) {
                        int16_t index = (int16_t)(*(int16_t *)(layers[stage] + 0x3a) % bitmap_count);
                        uint8_t *bitmap = 0;

                        if (bitmap_tag != 0 && index >= 0 && index < bitmap_count) {
                            bitmap = *(uint8_t **)(bitmap_tag + 0x64) + index * 0x30;
                        }
                        if (*(int16_t *)&((struct BitmapData *)bitmap)->type == 0) {
                            bind_ripple_bitmap((uint32_t)stage, bitmap);
                            bound = 1;
                        }
                    }
                }
                if (!bound) {
                    datum_index fallback = *(datum_index *)((uint8_t *)rasterizer_globals_data + 0xb8);

                    if (fallback != k_datum_index_none) {
                        uint8_t *bitmap_tag = (uint8_t *)tag_instances[fallback & 0xffff].data;

                        if (bitmap_tag != 0 && *(int32_t *)(bitmap_tag + 0x60) > 3) {
                            bind_ripple_bitmap((uint32_t)stage, *(uint8_t **)(bitmap_tag + 0x64) + 0x90);
                        }
                    }
                }
            }

            ((d3d_call4v_fn)DEVICE_CALL(0x1b4))(rasterizer_device, 0, pixel_constants, 4);
            {
                void **vtable = *(void ***)effect;

                ((int32_t (__stdcall *)(void *, uint32_t *, uint32_t))vtable[0x100 / 4])(effect, &passes, 3);
                for (pass = 0; pass < passes; pass++) {
                    ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x104 / 4])(effect, pass);
                    ((int32_t (__stdcall *)(void *, uint32_t, uint32_t, const void *, uint32_t))DEVICE_CALL(0x14c))(
                        rasterizer_device, 6, 2, rasterizer_water_ripple_quad, 0x18); // DrawPrimitiveUP fan
                }
                ((int32_t (__stdcall *)(void *))vtable[0x108 / 4])(effect);
            }
        }
        ((d3d_call1_fn)DEVICE_CALL(0x134))(rasterizer_device, rasterizer_software_vertex_processing);
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    rasterizer_set_shader_stage_config(2);
}
