// rasterizer_dynamic_geometry_draw_dispatch  (Ghidra: already named, __cdecl)
// address 0x51c730, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: picks one of the four indexed draw paths from which of the static index and vertex
//   buffers exist: static/static rasterizer_dynamic_geometry_chain_draw 0x51c5f0, static
//   indices with dynamic vertices rasterizer_dynamic_vertex_draw_indexed 0x51c490, dynamic
//   indices with static vertices chimera__rasterizer_draw_dynamic_triangles_static_vertices
//   0x51c1c0, dynamic/dynamic rasterizer_dynamic_index_cache_draw 0x51c090. The primitive
//   count, first primitive and dynamic vertex slot are never touched here: they arrive in EAX,
//   ECX and EBX and are forwarded to whichever callee needs them (these are the
//   transparent_geometry_group primitive_count, first_index and dynamic_vertex_slot fields).
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51c730..0x51c781; the earlier
//   rewrite read the three stack arguments in the wrong roles and dropped the register inputs.
// register convention: stack = (index_buffer, dynamic_index_slot, vertex_buffer); EAX =
//   primitive_count, ECX = first_primitive, EBX = dynamic_vertex_slot (all live-in).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

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

// blam-cc: stack -> (index_buffer, dynamic_index_slot, vertex_buffer), EAX -> primitive_count,
//   ECX -> first_primitive, EBX -> dynamic_vertex_slot
void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                               rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count,
                                               int32_t first_primitive, int32_t dynamic_vertex_slot)
{
    if (index_buffer != 0) {
        if (vertex_buffer != 0) {
            rasterizer_dynamic_geometry_chain_draw(primitive_count, vertex_buffer, index_buffer);
        } else {
            rasterizer_dynamic_vertex_draw_indexed(index_buffer, primitive_count, dynamic_vertex_slot);
        }
    } else if (vertex_buffer != 0) {
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
    } else {
        rasterizer_dynamic_index_cache_draw(dynamic_index_slot, first_primitive, primitive_count, dynamic_vertex_slot);
    }
}

#if 0
Original Ghidra decompilation (0x51c730):

void __cdecl
rasterizer_dynamic_geometry_draw_dispatch(int source_kind,undefined4 param_2,int is_static)

{
  if (source_kind == 0) {
    if (is_static != 0) {
      chimera__rasterizer_draw_dynamic_triangles_static_vertices(param_2);
      return;
    }
    FUN_0051c090(param_2);
    return;
  }
  if (is_static != 0) {
    FUN_0051c5f0();
    return;
  }
  FUN_0051c490(source_kind);
  return;
}
#endif
