// rasterizer_shader_environment_lightmap_draw_two_stream  (Ghidra: LAB_0051e570, never created as a function;
//   installed in the 0x7c0490 draw slot by rasterizer_select_hardware_codepaths on multi-stream devices without
//   pixel shaders 1.1)
// address 0x51e570, size 890 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x51e570..0x51e8e9. While console toggle 0x6893f4 is set and the shader's
//   environment effect (rasterizer_effects[((type +0x2a * 3 + +0xb0) * 3 + +0xf4) + 5], only tested for presence)
//   exists: the base map (+0x94) on stage 0, vertex shader 0 with declaration 12, no pixel shader. With a detail map
//   (+0xc4) it goes on stage 1 with a texture-1 transform scaling u and v by (base size / detail size) * the detail
//   scale (+0xb4), stage 0 = texture, stage 1 = modulate2x(texture, current) with its alpha kept, stage 2 disabled;
//   after the draw the stage-1 transform is turned off and its texcoord index restored to 1. Without one: stage 0
//   modulate-by-texture, stage 1 disabled, draw.
// blam-cc: stack -> shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include <string.h>

extern void *rasterizer_device;                     // 0x0071d174
extern uint8_t console_debug_toggle_6893f4;         // 0x006893f4
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame); returns the bound bitmap's
//   {width, height} words
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
    int16_t default_index, int16_t frame); // 0x518960
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2p_fn)(void *self, uint32_t a, const void *b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)DEVICE_CALL(0x10c))(rasterizer_device, stage, type, value);
}

void rasterizer_shader_environment_lightmap_draw_two_stream(const ShaderEnvironment *shader, int16_t frame,
    int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    int16_t effect_index;
    int16_t *base_size;
    int16_t base_width, base_height;

    if (console_debug_toggle_6893f4 == 0) {
        return;
    }
    effect_index = (int16_t)((uint16_t)(((uint16_t)(*(uint16_t *)(raw + 0x2a) * 3) + *(uint16_t *)(raw + 0xb0)) * 3) +
        *(uint16_t *)(raw + 0xf4) + 5);
    if (rasterizer_effects[effect_index].effect == 0) {
        return;
    }

    base_size = chimera__rasterizer_set_texture(*(uint32_t *)(raw + 0x94), 0, 0, 1, frame);
    base_width = base_size[0];
    base_height = base_size[1];
    ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
    ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[12].declaration);
    ((d3d_call1_fn)DEVICE_CALL(0x1ac))(rasterizer_device, 0);

    if (*(int32_t *)(raw + 0xc4) != -1) {
        int16_t *detail_size = chimera__rasterizer_set_texture(*(uint32_t *)(raw + 0xc4), 1, 0, 2, frame);
        float matrix[16];

        memset(matrix, 0, sizeof matrix);
        matrix[0] = (float)(int32_t)base_width / (float)(int32_t)detail_size[0] * *(float *)(raw + 0xb4);
        matrix[5] = (float)(int32_t)base_height / (float)(int32_t)detail_size[1] * *(float *)(raw + 0xb4);
        matrix[10] = 1.0f;
        matrix[15] = 1.0f;
        set_texture_stage_state(1, 0x18, 2); // D3DTSS_TEXTURETRANSFORMFLAGS = COUNT2
        ((d3d_call2p_fn)DEVICE_CALL(0xb0))(rasterizer_device, 0x11, matrix); // SetTransform(D3DTS_TEXTURE1)
        set_texture_stage_state(1, 0xb, 0);
        set_texture_stage_state(0, 1, 2);
        set_texture_stage_state(0, 2, 2);
        set_texture_stage_state(0, 4, 2);
        set_texture_stage_state(0, 5, 1);
        set_texture_stage_state(1, 1, 4);
        set_texture_stage_state(1, 2, 2);
        set_texture_stage_state(1, 3, 1);
        set_texture_stage_state(1, 4, 2);
        set_texture_stage_state(1, 5, 2);
        set_texture_stage_state(2, 1, 1);
        set_texture_stage_state(2, 4, 1);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
        set_texture_stage_state(1, 0x18, 0);
        set_texture_stage_state(1, 0xb, 1);
        return;
    }
    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 1);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
}
