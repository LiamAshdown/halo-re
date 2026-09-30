// rasterizer_dynamic_geometry_chain_draw  (Ghidra: FUN_0051c5f0)
// address 0x51c5f0, size 312 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: static vertices with static indices: SetStreamSource(0, vertex_buffer hardware, 0,
//   rasterizer_vertex_sizes[type]), SetIndices(index_buffer hardware) and DrawIndexedPrimitive
//   with the index buffer primitive type, base vertex 0 and vertex_buffer.count vertices, in
//   chunks of 10000 primitives with the same start index advance as
//   rasterizer_dynamic_vertex_draw_indexed 0x51c490 (3 per primitive for lists, 1 for strips).
//   Despite the phase 2 name nothing is chained: it is the static/static member of the four
//   draw paths chosen by rasterizer_dynamic_geometry_draw_dispatch 0x51c730.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51c5f0..0x51c727.
// register convention: EAX = vertex_buffer, EDI = index_buffer, stack = primitive_count.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include "fn_interface.h"

extern void *rasterizer_device;                                     // 0x0071d174


extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count]; // 0x0065de00
extern uint32_t rasterizer_triangle_buffer_primitive_types[2];      // 0x0065e034 {4, 5}

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);
typedef int32_t (__stdcall *d3d_draw_indexed_primitive_fn)(void *self, uint32_t type, int32_t base_vertex, uint32_t min_index,
                                                 uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

// blam-cc: EAX -> vertex_buffer, EDI -> index_buffer, stack -> primitive_count
void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                            rasterizer_index_buffer *index_buffer)
{
    uint32_t start_index = 0;

    while (primitive_count > 0) {
        uint32_t stride;
        int32_t chunk;

        if (index_buffer == 0 || index_buffer->hardware_buffer == 0 || vertex_buffer == 0 || vertex_buffer->hardware_buffer == 0) {
            break;
        }
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[vertex_buffer->type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[vertex_buffer->type].usage) & 0x10);
        ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, (void *)vertex_buffer->hardware_buffer, 0, stride);
        ((d3d_set_pointer_fn)device_vtable()[0x1a0 / 4])(rasterizer_device, (void *)index_buffer->hardware_buffer);
        {
            debug_fp_pre_draw(); // TEMPORARY: forces alpha test off for first-person draws
            int32_t debug_hr = ((d3d_draw_indexed_primitive_fn)device_vtable()[0x148 / 4])(rasterizer_device,
                                                                    rasterizer_triangle_buffer_primitive_types[index_buffer->type],
                                                                    0, 0, (uint32_t)vertex_buffer->count, start_index, (uint32_t)chunk);
            debug_fp_draw_state_note("chn", debug_hr, 0, (uint32_t)vertex_buffer->count, (uint32_t)chunk); // TEMPORARY
        }
        primitive_count -= chunk;
        switch (index_buffer->type) {
        case 0:                                                 // triangle list
            start_index += (uint32_t)chunk * 3;
            break;
        case 1:                                                 // triangle strip
            start_index += (uint32_t)chunk;
            break;
        default:
            break;
        }
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51c5f0):

void FUN_0051c5f0(int param_1)

{
  short *in_EAX;
  undefined4 unaff_EBX;
  int iVar1;
  short *unaff_EDI;
  int iVar2;
  short *local_c;

  local_c = (short *)0x0;
  while ((((0 < param_1 && (unaff_EDI != (short *)0x0)) && (*(int *)(unaff_EDI + 6) != 0)) &&
         ((in_EAX != (short *)0x0 && (*(int *)(in_EAX + 8) != 0))))) {
    iVar1 = 10000;
    if (param_1 < 0x2711) {
      iVar1 = param_1;
    }
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,
               -(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[*in_EAX * 3] & 0x10);
    iVar2 = 0;
    (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,*(undefined4 *)(local_c + 8),0,unaff_EBX);
    (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,*(undefined4 *)(unaff_EDI + 6));
    (**(code **)(*DAT_0071d174 + 0x148))
              (DAT_0071d174,*(undefined4 *)(&DAT_0065e034 + *unaff_EDI * 4),0,0,
               *(undefined4 *)(local_c + 2),iVar2,iVar1);
    param_1 = param_1 - iVar1;
    in_EAX = local_c;
    if (*unaff_EDI == 0) {
      local_c = (short *)(iVar1 * 3 + iVar2);
    }
    else if (*unaff_EDI == 1) {
      local_c = (short *)(iVar2 + iVar1);
    }
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
