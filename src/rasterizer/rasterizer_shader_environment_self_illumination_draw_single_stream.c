// rasterizer_shader_environment_self_illumination_draw_single_stream  (Ghidra: LAB_0051fd80, never created as a function; installed in the 0x7c048c slot by rasterizer_select_hardware_codepaths on single-stream devices)
// address 0x51fd80, size 661 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump (0x51fad0 and 0x51fd80 differ only in the declaration and the draw). While
//   console toggle 0x6893f1 is set: alpha test from tag flag 1 and toggle 0x68941c, fixed function, texture
//   factor from the primary self-illumination colour, the self-illumination map on stage 0 and the environment
//   lightmap on stage 1 (stage 0 = texture x diffuse, stage 1 = texture blended by current alpha), declaration
//   19, then the single-stream draw (0x51c1c0).
// blam-cc: stack -> shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "bitmaps.h"
#include "rasterizer.h"

extern void *rasterizer_device;                     // 0x0071d174
extern uint8_t console_debug_toggle_6893f1;         // 0x006893f1
extern uint8_t console_debug_toggle_68941c;         // 0x0068941c
extern uint8_t console_debug_toggle_689409;         // 0x00689409
extern tag_instance *tag_instances;                 // 0x0087bc14
extern GlobalsRasterizerData *rasterizer_globals_data; // 0x0071d164
extern BitmapData *rasterizer_environment_lightmap; // 0x006e0a08
extern void *rasterizer_capture_surfaces[4];        // 0x0069c66c
extern int16_t rasterizer_bound_bitmap_size_a[2];   // 0x006d986c
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern BitmapData *bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index); // 0x43f250, EAX, DX
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680, ESI bitmap, stack stage
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550, EAX
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)DEVICE_CALL(0x10c))(rasterizer_device, stage, type, value);
}

void rasterizer_shader_environment_self_illumination_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    datum_index self_illumination;
    BitmapData *bitmap = 0;
    uint32_t colour;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    ((d3d_call2_fn)DEVICE_CALL(0xe4))(rasterizer_device, 0xf, (raw[0x28] & 1) != 0 && console_debug_toggle_68941c != 0);
    ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
    // 0x51fb1d: texture factor 0xffRRGGBB from the primary self-illumination colour (+0x10c), __ftol truncating
    colour = 0xffffff00u | (uint32_t)(int32_t)(*(float *)(raw + 0x10c) * 255.0f);
    colour = (colour << 8) | ((uint32_t)(int32_t)(*(float *)(raw + 0x110) * 255.0f) & 0xff);
    colour = (colour << 8) | ((uint32_t)(int32_t)(*(float *)(raw + 0x114) * 255.0f) & 0xff);
    ((d3d_call2_fn)DEVICE_CALL(0xe4))(rasterizer_device, 0x3c, colour);

    // stage 0: the self-illumination map (+0x134, frame modulo its bitmap count) unless tag flag 2, else the
    // rasterizer globals' +0xb8 bitmap 3
    self_illumination = (raw[0x28] & 2) ? k_datum_index_none : *(datum_index *)(raw + 0x134);
    if (console_debug_toggle_689409 != 0 && self_illumination != k_datum_index_none) {
        int32_t count = *(int32_t *)((uint8_t *)tag_instances[self_illumination & 0xffff].data + 0x60);

        if (count > 0) {
            bitmap = bitmap_group_get_bitmap_data(self_illumination, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)((uint8_t *)bitmap + 0xa) != 0) {
                bitmap = 0;
            }
        }
    }
    if (bitmap == 0) {
        datum_index fallback = *(datum_index *)((uint8_t *)rasterizer_globals_data + 0xb8);

        if (fallback != k_datum_index_none) {
            uint8_t *tag = (uint8_t *)tag_instances[fallback & 0xffff].data;

            if (tag != 0 && *(int32_t *)(tag + 0x60) > 3) {
                bitmap = (BitmapData *)(*(uint8_t **)(tag + 0x64) + 0x90);
            }
        }
    }
    if (bitmap != 0) {
        rasterizer_bind_texture_d3d9(0, bitmap);
        rasterizer_bound_bitmap_size_a[0] = *(int16_t *)((uint8_t *)bitmap + 0x4);
        rasterizer_bound_bitmap_size_a[1] = *(int16_t *)((uint8_t *)bitmap + 0x6);
    }

    // stage 1: the environment lightmap, or capture surface 0
    {
        uint32_t texture;

        if (rasterizer_environment_lightmap != 0) {
            texture_cache_get(rasterizer_environment_lightmap, 1, 1);
            texture = *(uint32_t *)((uint8_t *)rasterizer_environment_lightmap + 0x28);
        } else {
            texture = (uint32_t)rasterizer_capture_surfaces[0];
        }
        ((d3d_call2_fn)DEVICE_CALL(0x104))(rasterizer_device, 1, texture);
    }
    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 0);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(1, 1, 7);
    set_texture_stage_state(1, 2, 2);
    set_texture_stage_state(1, 3, 1);
    set_texture_stage_state(1, 4, 2);
    set_texture_stage_state(1, 5, 1);
    set_texture_stage_state(2, 1, 1);
    set_texture_stage_state(2, 4, 1);
    ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[19].declaration);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
}
