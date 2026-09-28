// chimera__rasterizer_draw_dynamic_triangles_static_vertices2  (Ghidra: already named; Chimera
// name, hint only)
// address 0x51c310, size 383 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: the two stream variant of chimera__rasterizer_draw_dynamic_triangles_static_vertices
//   0x51c1c0: both buffers must have a hardware buffer; stream 1 (second_stream, stride of its
//   own vertex type) is bound only when 0x00722b60 is clear and d3d_caps9.max_streams > 1.
//   rasterizer_transparent_geometry_group_draw_vertices 0x533660 passes group.vertex_buffer + 0x14
//   (the lightmap stream of a BSP material) as second_stream; the environment reflection pass
//   0x5202f0 passes the buffer itself or its successor.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51c310..0x51c48e; the earlier
//   rewrite lost the second stream stride and the start index (Ghidra showed them through
//   unaffected EBP/ESI reads).
// register convention: EAX = primitive_count, EDI = vertex_buffer, stack = (dynamic_index_slot,
//   first_primitive, second_stream); the first_primitive stack slot is advanced in place.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern void debug_fp_draw_state_note(const char *site, int32_t hresult, uint32_t primitive_type,
    uint32_t vertex_count, uint32_t primitive_count); // TEMPORARY first-person diagnostics
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count]; // 0x0065de00
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006dd9e0
extern void *rasterizer_dynamic_index_buffer;                       // 0x006e09e8
extern int32_t config_safe_mode;                             // 0x00722b60 nonzero: one vertex stream, fixed function path

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);
typedef int32_t (__stdcall *d3d_draw_indexed_primitive_fn)(void *self, uint32_t type, int32_t base_vertex, uint32_t min_index,
                                                 uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

// blam-cc: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, second_stream)
void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                 int32_t dynamic_index_slot, int32_t first_primitive,
                                                                 rasterizer_vertex_buffer *second_stream)
{
    while (primitive_count > 0) {
        rasterizer_dynamic_index_slot *slot;
        uint32_t stride;
        uint32_t second_stride;
        int32_t chunk;

        if (dynamic_index_slot == -1 || vertex_buffer == 0 || vertex_buffer->hardware_buffer == 0 ||
            second_stream == 0 || second_stream->hardware_buffer == 0) {
            break;
        }
        slot = &rasterizer_dynamic_index_slots[dynamic_index_slot];
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[vertex_buffer->type];
        second_stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[second_stream->type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                                   ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                    rasterizer_vertex_declarations[vertex_buffer->type].usage) & 0x10);
        ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, (void *)vertex_buffer->hardware_buffer, 0, stride);
        if (config_safe_mode == 0 && rasterizer_caps.max_streams > 1) {
            ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 1, (void *)second_stream->hardware_buffer, 0,
                                                                   second_stride);
        }
        ((d3d_set_pointer_fn)device_vtable()[0x1a0 / 4])(rasterizer_device, rasterizer_dynamic_index_buffer);
        {
            int32_t debug_hr = ((d3d_draw_indexed_primitive_fn)device_vtable()[0x148 / 4])(rasterizer_device, 4, 0, 0, (uint32_t)vertex_buffer->count,
                                                                    (uint32_t)((slot->first_index + first_primitive) * 3),
                                                                    (uint32_t)chunk);
            debug_fp_draw_state_note("st2", debug_hr, 0, (uint32_t)vertex_buffer->count, (uint32_t)chunk); // TEMPORARY
        }
        first_primitive += chunk;
        primitive_count -= chunk;
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51c310):

void chimera__rasterizer_draw_dynamic_triangles_static_vertices2
               (int param_1,undefined4 param_2,int param_3)

{
  int in_EAX;
  int *unaff_EBP;
  int unaff_ESI;
  int iVar1;
  short *unaff_EDI;
  int *piVar2;

  while (((((0 < in_EAX && (param_1 != -1)) && (unaff_EDI != (short *)0x0)) &&
          ((*(int *)(unaff_EDI + 8) != 0 && (param_3 != 0)))) && (*(int *)(param_3 + 0x10) != 0))) {
    param_3 = (int)*(short *)(&DAT_0065de00 + *unaff_EDI * 2);
    iVar1 = 10000;
    if (in_EAX < 0x2711) {
      iVar1 = in_EAX;
    }
    piVar2 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,
               -(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[*unaff_EDI * 3] & 0x10);
    (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,*(undefined4 *)(unaff_EDI + 8),0);
    if ((DAT_00722b60 == 0) && (1 < DAT_007c117c)) {
      (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,1,*(undefined4 *)(param_3 + 0x10),0,piVar2);
    }
    (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,DAT_006e09e8);
    (**(code **)(*DAT_0071d174 + 0x148))
              (DAT_0071d174,4,0,0,*(undefined4 *)(unaff_EDI + 2),(*unaff_EBP + unaff_ESI) * 3,iVar1)
    ;
    in_EAX = (int)unaff_EBP - iVar1;
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
