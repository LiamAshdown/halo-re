// chimera__rasterizer_draw_dynamic_triangles_static_vertices  (Ghidra: already named; Chimera name,
// hint only)
// address 0x51c1c0, size 324 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: draws primitive_count triangles of a static rasterizer_vertex_buffer (ESI) indexed out
//   of the shared dynamic index buffer (0x006e09e8) at rasterizer_dynamic_index_slots[slot]
//   (0x006dd9e0, 0xc byte records) plus first_primitive, in chunks of at most 10000 primitives:
//   SetSoftwareVertexProcessing from the declaration usage of the buffer vertex type,
//   SetStreamSource(0, buffer, 0, rasterizer_vertex_sizes[type]), SetIndices, then
//   DrawIndexedPrimitive(TRIANGLELIST, 0, 0, buffer.count, (slot.first_index + first) * 3, n).
//   It also calls GetDesc (+0x34) on the vertex and index buffers into locals it never reads.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51c1c0..0x51c303; the
//   earlier rewrite modelled Ghidra's unaffected-register reads as opaque placeholders.
// register convention: EAX = primitive_count, ESI = vertex_buffer, stack = (dynamic_index_slot,
//   first_primitive); the first_primitive stack slot is advanced in place per chunk.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern void debug_fp_draw_state_note(const char *site, int32_t hresult, uint32_t primitive_type,
    uint32_t vertex_count, uint32_t primitive_count); // TEMPORARY first-person diagnostics
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count]; // 0x0065de00
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006dd9e0
extern void *rasterizer_dynamic_index_buffer;                       // 0x006e09e8

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *self, void *desc);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);
typedef int32_t (__stdcall *d3d_draw_indexed_primitive_fn)(void *self, uint32_t type, int32_t base_vertex, uint32_t min_index,
                                                 uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                int32_t dynamic_index_slot, int32_t first_primitive)
{
    while (primitive_count > 0) {
        rasterizer_dynamic_index_slot *slot;
        uint32_t stride;
        int32_t chunk;
        void *hardware_buffer;
        uint32_t vertex_desc[6];                                // D3DVERTEXBUFFER_DESC, unread
        uint32_t index_desc[5];                                 // D3DINDEXBUFFER_DESC, unread

        if (dynamic_index_slot == -1 || vertex_buffer == 0 || vertex_buffer->hardware_buffer == 0) {
            break;
        }
        slot = &rasterizer_dynamic_index_slots[dynamic_index_slot];
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[vertex_buffer->type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        hardware_buffer = (void *)vertex_buffer->hardware_buffer;
        ((d3d_get_desc_fn)(*(void ***)hardware_buffer)[0x34 / 4])(hardware_buffer, vertex_desc);
        ((d3d_get_desc_fn)(*(void ***)rasterizer_dynamic_index_buffer)[0x34 / 4])(rasterizer_dynamic_index_buffer, index_desc);

        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                                   ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                    rasterizer_vertex_declarations[vertex_buffer->type].usage) & 0x10);
        ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, hardware_buffer, 0, stride);
        ((d3d_set_pointer_fn)device_vtable()[0x1a0 / 4])(rasterizer_device, rasterizer_dynamic_index_buffer);
        {
            int32_t debug_hr = ((d3d_draw_indexed_primitive_fn)device_vtable()[0x148 / 4])(rasterizer_device, 4, 0, 0, (uint32_t)vertex_buffer->count,
                                                                    (uint32_t)((slot->first_index + first_primitive) * 3),
                                                                    (uint32_t)chunk);
            debug_fp_draw_state_note("st1", debug_hr, 0, (uint32_t)vertex_buffer->count, (uint32_t)chunk); // TEMPORARY
        }
        first_primitive += chunk;
        primitive_count -= chunk;
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51c1c0):

void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int param_1)

{
  int in_EAX;
  short *unaff_ESI;
  int iVar1;
  int *piVar2;
  undefined **ppuVar3;
  undefined *local_34;
  int iStack_30;
  int iStack_2c;
  undefined1 local_18 [24];

  while ((((0 < in_EAX && (param_1 != -1)) && (unaff_ESI != (short *)0x0)) &&
         (*(int *)(unaff_ESI + 8) != 0))) {
    local_34 = &DAT_006dd9e0 + param_1 * 0xc;
    iVar1 = 10000;
    if (in_EAX < 0x2711) {
      iVar1 = in_EAX;
    }
    (**(code **)(**(int **)(unaff_ESI + 8) + 0x34))(*(int **)(unaff_ESI + 8),local_18);
    ppuVar3 = &local_34;
    (**(code **)(*DAT_006e09e8 + 0x34))(DAT_006e09e8,ppuVar3);
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,
               -(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[*unaff_ESI * 3] & 0x10);
    piVar2 = *(int **)(unaff_ESI + 8);
    (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,piVar2,0,ppuVar3);
    (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,DAT_006e09e8);
    (**(code **)(*DAT_0071d174 + 0x148))
              (DAT_0071d174,4,0,0,*(undefined4 *)(unaff_ESI + 2),(*piVar2 + iStack_2c) * 3,iVar1);
    in_EAX = iStack_30 - iVar1;
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
