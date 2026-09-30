// rasterizer_shader_environment_lightmap_draw_single_stream  (Ghidra: LAB_0051e8f0, never created as a function; installed in the
//   0x7c0490 (environment lightmap) draw slot by rasterizer_select_hardware_codepaths on devices with fewer than two vertex streams)
// address 0x51e8f0, size 244 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x51e8f0..0x51e9e3. While console toggle 0x6893f4 is set: the shader's +0x94
//   bitmap on stage 0 (chimera__rasterizer_set_texture, default index 1, the given frame), vertex shader 0 with
//   declaration 19, no pixel shader, stage 0 colour/alpha modulate-by-texture state (1,2,4 = 2; 5 = 1), stage 1
//   disabled (1,4 = 1), then one single-stream draw of the primitives.
// blam-cc: stack -> shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern void *rasterizer_device;                     // 0x0071d174
extern uint8_t console_debug_toggle_6893f4;         // 0x006893f4
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)

// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

void rasterizer_shader_environment_lightmap_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    d3d_call3_fn set_texture_stage_state;

    if (console_debug_toggle_6893f4 == 0) {
        return;
    }
    chimera__rasterizer_set_texture(*(uint32_t *)&((struct ShaderEnvironment *)shader)->base_map.tag_id, 0, 0, 1, frame);
    ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
    ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, (uint32_t)rasterizer_vertex_declarations[19].declaration);
    ((d3d_call1_fn)DEVICE_CALL(0x1ac))(rasterizer_device, 0);
    set_texture_stage_state = (d3d_call3_fn)DEVICE_CALL(0x10c);
    set_texture_stage_state(rasterizer_device, 0, 1, 2);
    set_texture_stage_state(rasterizer_device, 0, 2, 2);
    set_texture_stage_state(rasterizer_device, 0, 4, 2);
    set_texture_stage_state(rasterizer_device, 0, 5, 1);
    set_texture_stage_state(rasterizer_device, 1, 1, 1);
    set_texture_stage_state(rasterizer_device, 1, 4, 1);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
}
