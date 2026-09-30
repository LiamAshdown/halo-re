// rasterizer_shader_environment_draw_single_stream  (Ghidra: FUN_005276c0, never created as a function; installed as
//   shader_environment_draw_simple (0x7c0470) by rasterizer_shader_environment_select_draw_functions on devices with
//   one vertex stream)
// address 0x5276c0, size 1054 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5276c0..0x527add. While console toggle 0x6893ec is set: z from the model draw
//   context flags (flag 8 -> ZENABLE off, else z on with ZWRITE and a LESSEQUAL test after clearing the decal z
//   bias), then an opaque multiplicative blend state (src DESTCOLOR, dest SRCCOLOR), alpha test from tag flag 1 at
//   reference 0x7f, fog from the latched fog flag, and a texture-0 scale of (context +0xc4, context +0xc8, 0)
//   (D3DXMatrixScaling through the 0x5810a7 dispatch, built directly here). Stage 0 = texture x current, stage 1 off.
//   Context flag 0x200: one fixed-function pass of the base map (+0x94) with declaration 14. Otherwise the vertices
//   are first processed by vertex shader 27 (declaration 4) into a dynamic buffer, and the pass draws a copy of the
//   vertex buffer re-typed 0xf with that handle, using declaration 15. The decal z bias is cleared afterwards.
// blam-cc: stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include <string.h>

extern void *rasterizer_device;                     // 0x0071d174
extern uint8_t console_debug_toggle_6893ec;         // 0x006893ec
extern uint8_t *rasterizer_active_model_context;    // 0x0071d1f0
extern uint8_t rasterizer_fog_enabled;              // 0x0069c6a8
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90


// blam-cc: stack -> (index_buffer, dynamic_index_slot, vertex_buffer), EAX -> primitive_count, ECX -> first_primitive,
//   EBX -> dynamic_vertex_slot


typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call2p_fn)(void *self, uint32_t a, const void *b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)DEVICE_CALL(0xe4))(rasterizer_device, state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)DEVICE_CALL(0x10c))(rasterizer_device, stage, type, value);
}

void rasterizer_shader_environment_draw_single_stream(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
    int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_vertex_slot)
{
    float matrix[16];

    if (console_debug_toggle_6893ec == 0) {
        return;
    }
    if (rasterizer_active_model_context[0] & 8) {
        set_render_state(7, 0);
    } else {
        set_render_state(7, 1);
        set_render_state(0xe, 1);
        set_render_state(0x17, 4);
        rasterizer_clear_decal_zbias();
    }
    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 0);
    set_render_state(0x13, 5);
    set_render_state(0x14, 6);
    set_render_state(0xab, 1);
    set_render_state(0xf, shader[0x28] & 1);
    set_render_state(0x18, 0x7f);
    set_render_state(0x1c, rasterizer_fog_enabled != 0);

    memset(matrix, 0, sizeof matrix); // D3DXMatrixScaling(sx, sy, 0)
    matrix[0] = *(float *)(rasterizer_active_model_context + 0xc4);
    matrix[5] = *(float *)(rasterizer_active_model_context + 0xc8);
    matrix[10] = 0.0f;
    matrix[15] = 1.0f;

    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);

    if (*(uint32_t *)rasterizer_active_model_context & 0x200) {
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[14].declaration);
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame);
        ((d3d_call2p_fn)DEVICE_CALL(0xb0))(rasterizer_device, 0x10, matrix); // SetTransform(D3DTS_TEXTURE0)
        set_texture_stage_state(0, 0x18, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
        rasterizer_clear_decal_zbias();
        return;
    }
    {
        rasterizer_vertex_buffer processed = *vertex_buffer; // esp+0xc
        uint32_t handle = 0;

        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, rasterizer_vertex_shaders[27].shader);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[4].declaration);
        if (index_buffer != 0) {
            handle = rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer);
        }
        processed.hardware_buffer = handle;
        processed.type = 0xf;
        set_texture_stage_state(0, 0x18, 2);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[15].declaration);
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame);
        ((d3d_call2p_fn)DEVICE_CALL(0xb0))(rasterizer_device, 0x10, matrix);
        set_texture_stage_state(0, 0x18, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
        rasterizer_clear_decal_zbias();
    }
}
