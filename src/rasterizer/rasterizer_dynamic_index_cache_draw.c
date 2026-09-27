// rasterizer_dynamic_index_cache_draw  (Ghidra: FUN_0051c090)
// address 0x51c090, size 295 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: dynamic vertices with dynamic indices: the vertex range of
//   rasterizer_dynamic_vertex_slots[dynamic_vertex_slot] (0x006d99d8, 0x10 byte records) out of
//   the per type cache buffer (rasterizer_dynamic_vertex_caches[type].buffer_handle, a 1 based
//   rasterizer_vertex_buffer_slots handle, 0x007bf04c + h*0x14), indexed from the shared
//   dynamic index buffer at rasterizer_dynamic_index_slots[dynamic_index_slot].first_index +
//   first_primitive, as triangle lists in chunks of at most 10000 primitives. The stride is
//   parked in the count argument slot (0x51c0e9) once the count is in EBP.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51c090..0x51c1b6; the earlier
//   rewrite had the parameters in the wrong roles (stride, unused, count, slot) and no start
//   index or base vertex.
// register convention: __cdecl, (dynamic_index_slot, first_primitive, primitive_count,
//   dynamic_vertex_slot) on the stack.

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
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006dd9e0
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060
extern void *rasterizer_dynamic_index_buffer;                       // 0x006e09e8

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);
typedef int32_t (__stdcall *d3d_draw_indexed_primitive_fn)(void *self, uint32_t type, int32_t base_vertex, uint32_t min_index,
                                                 uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count,
                                         int32_t dynamic_vertex_slot)
{
    while (primitive_count > 0) {
        rasterizer_dynamic_vertex_slot *vertex_slot;
        rasterizer_dynamic_index_slot *index_slot;
        int16_t type;
        uint32_t stride;
        int32_t chunk;
        int32_t handle;
        void *buffer;

        if (dynamic_index_slot == -1 || dynamic_vertex_slot == -1) {
            break;
        }
        vertex_slot = &rasterizer_dynamic_vertex_slots[dynamic_vertex_slot];
        index_slot = &rasterizer_dynamic_index_slots[dynamic_index_slot];
        type = vertex_slot->vertex_type;
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[type].usage) & 0x10);
        handle = rasterizer_dynamic_vertex_caches[type].buffer_handle;
        buffer = handle == 0 ? 0 : (void *)rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;
        ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, buffer, 0, stride);
        ((d3d_set_pointer_fn)device_vtable()[0x1a0 / 4])(rasterizer_device, rasterizer_dynamic_index_buffer);
        {
            int32_t debug_hr = ((d3d_draw_indexed_primitive_fn)device_vtable()[0x148 / 4])(rasterizer_device, 4, vertex_slot->first_vertex, 0,
                                                                    (uint32_t)vertex_slot->vertex_count,
                                                                    (uint32_t)((index_slot->first_index + first_primitive) * 3),
                                                                    (uint32_t)chunk);
            debug_fp_draw_state_note("idc", debug_hr, 0, (uint32_t)vertex_slot->vertex_count, (uint32_t)chunk); // TEMPORARY
        }
        first_primitive += chunk;
        primitive_count -= chunk;
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51c090):

void FUN_0051c090(int param_1,undefined4 param_2,int param_3,int param_4)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;

  while (((0 < param_3 && (param_1 != -1)) && (param_4 != -1))) {
    iVar3 = param_4 * 0x10;
    sVar1 = *(short *)(&DAT_006d99d8 + iVar3);
    iVar2 = 10000;
    if (param_3 < 0x2711) {
      iVar2 = param_3;
    }
    piVar5 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[sVar1 * 3] & 0x10
              );
    if ((&DAT_006d98f0)[sVar1 * 3] == 0) {
      piVar4 = (int *)0x0;
    }
    else {
      piVar4 = *(int **)(&DAT_007bf04c + (&DAT_006d98f0)[sVar1 * 3] * 10);
    }
    (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,piVar4,0,param_1);
    (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,DAT_006e09e8);
    (**(code **)(*DAT_0071d174 + 0x148))
              (DAT_0071d174,4,*(undefined4 *)(&DAT_006d99dc + iVar3),0,
               *(undefined4 *)(&DAT_006d99e0 + iVar3),(*piVar4 + (int)piVar5) * 3,iVar2);
    param_3 = param_3 - iVar2;
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
