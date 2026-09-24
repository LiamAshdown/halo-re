// rasterizer_dynamic_vertex_draw_indexed  (Ghidra: FUN_0051c490)
// address 0x51c490, size 338 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: dynamic vertices (rasterizer_dynamic_vertex_slots[dynamic_vertex_slot] out of the
//   per type cache buffer) with a static rasterizer_index_buffer: SetIndices(index_buffer
//   hardware), DrawIndexedPrimitive with the primitive type of the buffer
//   (rasterizer_triangle_buffer_primitive_types 0x0065e034, {list, strip}), the slot as base
//   vertex and vertex count, in chunks of 10000 primitives. The start index advances by 3 per
//   primitive for lists and by 1 for strips (other types do not advance).
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51c490..0x51c5e1; the stride
//   is the vertex size of the slot type, not a live-in EBX, and the start index is a local.
// register convention: __cdecl, (index_buffer, primitive_count, dynamic_vertex_slot) on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count]; // 0x0065de00
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060
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

void rasterizer_dynamic_vertex_draw_indexed(rasterizer_index_buffer *index_buffer, int32_t primitive_count, int32_t dynamic_vertex_slot)
{
    uint32_t start_index = 0;

    while (primitive_count > 0) {
        rasterizer_dynamic_vertex_slot *vertex_slot;
        int16_t type;
        uint32_t stride;
        int32_t chunk;
        int32_t handle;
        void *buffer;

        if (index_buffer == 0 || index_buffer->hardware_buffer == 0 || dynamic_vertex_slot == -1) {
            break;
        }
        vertex_slot = &rasterizer_dynamic_vertex_slots[dynamic_vertex_slot];
        type = vertex_slot->vertex_type;
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[type].usage) & 0x10);
        handle = rasterizer_dynamic_vertex_caches[type].buffer_handle;
        buffer = handle == 0 ? 0 : (void *)rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;
        ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, buffer, 0, stride);
        ((d3d_set_pointer_fn)device_vtable()[0x1a0 / 4])(rasterizer_device, (void *)index_buffer->hardware_buffer);
        ((d3d_draw_indexed_primitive_fn)device_vtable()[0x148 / 4])(rasterizer_device,
                                                                    rasterizer_triangle_buffer_primitive_types[index_buffer->type],
                                                                    vertex_slot->first_vertex, 0, (uint32_t)vertex_slot->vertex_count,
                                                                    start_index, (uint32_t)chunk);
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
Original Ghidra decompilation (0x51c490):

void FUN_0051c490(short *param_1,int param_2,int param_3)

{
  short sVar1;
  undefined4 unaff_EBX;
  int iVar2;
  int iVar3;
  undefined4 uVar4;

  while ((((0 < param_2 && (param_1 != (short *)0x0)) && (*(int *)(param_1 + 6) != 0)) &&
         (param_3 != -1))) {
    iVar3 = param_3 * 0x10;
    sVar1 = *(short *)(&DAT_006d99d8 + iVar3);
    iVar2 = param_2;
    if (10000 < param_2) {
      iVar2 = 10000;
    }
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[sVar1 * 3] & 0x10
              );
    if ((&DAT_006d98f0)[sVar1 * 3] == 0) {
      uVar4 = 0;
    }
    else {
      uVar4 = *(undefined4 *)(&DAT_007bf04c + (&DAT_006d98f0)[sVar1 * 3] * 10);
    }
    (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,uVar4,0,unaff_EBX);
    (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,*(undefined4 *)(param_1 + 6));
    (**(code **)(*DAT_0071d174 + 0x148))
              (DAT_0071d174,*(undefined4 *)(&DAT_0065e034 + *param_1 * 4),
               *(undefined4 *)(&DAT_006d99dc + iVar3),0,*(undefined4 *)(&DAT_006d99e0 + iVar3),uVar4
               ,iVar2);
    param_2 = param_2 - iVar2;
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
