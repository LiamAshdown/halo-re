// rasterizer_shader_environment_lightmap_draw  (not a Ghidra function; 0x007c0490 on ps_1_1+ cards with 2+ streams)
// address 0x51e2a0, size 718 bytes
// name confidence: 0.4  rewrite confidence: 0.8
// evidence: rasterizer_select_hardware_codepaths 0x51f8d0 stores it in 0x007c0490 (immediate 0x51e2a0) for cards with
//   more than one stream and pixel shaders above 1.0; render_window's structure pass reaches it through the thunk
//   0x511f70 (jmp [0x007c0490]) with the material callback arguments. Only referenced by that store (int3 padding
//   around it). Campaign track: the first level geometry pass.
// objdump 0x51e2a0..0x51e56d:
//   With 0x006893f4 set: the pixel shader effect slot (rasterizer_effects 0x0069d410, 0x20 each) is picked by
//   ((type +0x2a * 3 + detail function +0xb0) * 3 + micro detail function +0xf4) + 5 (word arithmetic); a slot
//   without an effect draws nothing. Its four maps are bound (rasterizer_resolve_and_cache_submap_b, frame, slot):
//   base (+0x94, stage 0, default 1), primary detail (+0xc4, 1, 2), secondary detail (+0xd8, 2, 2) and micro detail
//   (+0x108, 3, 2); each returns the bitmap's width/height words. With flags +0x6c bit 0 (rescale detail maps) the
//   detail scales are the base size over each detail size, else 1. c10..c12 get (primary scale +0xb4 x the first
//   pair, secondary scale +0xc8 x the second), (1, 0, micro scale +0xf8 x u, scroll u), (0, 1, +0xf8 x v, scroll v)
//   with the scrolls from shader_environment_texture_scrolling_evaluate (ESI shader, stack &u, &v, the frame time
//   0x007c1200). The environment vertex declaration (0x006e1a90) and the slot's vertex shader are set and every
//   effect pass draws chimera__rasterizer_draw_dynamic_triangles_static_vertices (EAX count, ESI vertex buffer,
//   stack dynamic index slot, first primitive).
// blam-cc: stack -> (shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h>

extern void *rasterizer_device;                   // 0x0071d174
extern uint8_t console_debug_toggle_6893f4;       // 0x006893f4
extern rasterizer_effect_slot rasterizer_effects[]; // 0x0069d410
extern uint32_t rasterizer_environment_vertex_declaration; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[]; // 0x0069e350
extern double rasterizer_frame_time_seconds;      // 0x007c1200

extern int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
    int16_t default_index, int16_t frame, rasterizer_effect_slot *effect_slot); // 0x518860
extern void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time,
    void *environment); // 0x540060, blam-cc: ESI -> environment, stack -> (u_out, v_out, time)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, void *vertex_buffer,
    int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0, EAX, ESI, stack

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

void rasterizer_shader_environment_lightmap_draw(uint8_t *shader, int16_t frame, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count, void *vertex_buffer)
{
    rasterizer_effect_slot *slot;
    int16_t index;
    int16_t size[4][2];
    float su1 = 1.0f, sv1 = 1.0f, su2 = 1.0f, sv2 = 1.0f, su3 = 1.0f, sv3 = 1.0f;
    float constants[12];
    void **device_vtable;
    void *effect;
    uint32_t passes;
    uint32_t pass;

    if (!console_debug_toggle_6893f4) {
        return;
    }
    index = (int16_t)(*(uint16_t *)(shader + 0x2a) * 3 + *(uint16_t *)(shader + 0xb0));
    index = (int16_t)((uint16_t)(index * 3) + *(uint16_t *)(shader + 0xf4) + 5);
    slot = &rasterizer_effects[index];
    if (slot->effect == 0) {
        return;
    }
    *(uint32_t *)size[0] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame, slot);
    *(uint32_t *)size[1] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xc4), 0, 1, 2, frame, slot);
    *(uint32_t *)size[2] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xd8), 0, 2, 2, frame, slot);
    *(uint32_t *)size[3] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x108), 0, 3, 2, frame, slot);
    if (shader[0x6c] & 1) {
        float base_width = (float)size[0][0];
        float base_height = (float)size[0][1];

        su1 = base_width / (float)size[1][0];
        sv1 = base_height / (float)size[1][1];
        su2 = base_width / (float)size[2][0];
        sv2 = base_height / (float)size[2][1];
        su3 = base_width / (float)size[3][0];
        sv3 = base_height / (float)size[3][1];
    }
    constants[0] = su1 * *(float *)(shader + 0xb4);
    constants[1] = sv1 * *(float *)(shader + 0xb4);
    constants[2] = su2 * *(float *)(shader + 0xc8);
    constants[3] = sv2 * *(float *)(shader + 0xc8);
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = su3 * *(float *)(shader + 0xf8);
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = sv3 * *(float *)(shader + 0xf8);
    constants[11] = 0.0f;
    shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_frame_time_seconds, shader);

    device_vtable = *(void ***)rasterizer_device;
    ((d3d_set_constant_f_fn)device_vtable[0x178 / 4])(rasterizer_device, 10, constants, 3);
    ((d3d_call1_fn)device_vtable[0x15c / 4])(rasterizer_device, rasterizer_environment_vertex_declaration);
    ((d3d_call1_fn)device_vtable[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[slot->vertex_shader_index].shader);

    effect = (void *)(uintptr_t)slot->effect;
    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        ((d3d_call1_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot,
            first_primitive);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}
