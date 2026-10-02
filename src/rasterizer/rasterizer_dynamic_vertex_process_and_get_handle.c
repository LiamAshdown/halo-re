// rasterizer_dynamic_vertex_process_and_get_handle  (Ghidra: FUN_0051c790)
// address 0x51c790, size 151 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: out/phase2/results/rasterizer_01.json ("Issues a single non-indexed draw call for
// one dynamic geometry record and returns the index-cache buffer handle used."); the vtable
// call at +0x154 (index 85, two methods after the DrawPrimitiveUP at +0x14c already confirmed
// in src/networking/network_stats_overlay_draw.c) takes exactly six arguments after the
// device, matching IDirect3DDevice9::ProcessVertices(SrcStartIndex, DestIndex, VertexCount,
// pDestBuffer, pVertexDecl, Flags) rather than a draw call; the phase2 summary's "draw call" may
// simply be imprecise about which vtable method this is. `unaff_ESI` matches rasterizer_vertex_buffer
// (count at short-offset 2, hardware_buffer at short-offset 8), the same live-in pattern as
// every sibling function in this family.
// register convention: no stack parameters; ESI = vertex_buffer (live-in).
// UNSURE: DAT_006d99a4 (the vertex_buffer_slot handle used here) has no documented owner in
// types/rasterizer.h; declared as an opaque extern.

// Spot-check fix (phase 4 review): the declaration usage is read with the 0xc byte stride of
//   rasterizer_vertex_declaration (the earlier rewrite indexed a uint32 array, stride 4), and the
//   destination buffer is *(0x007bf04c + h*0x14), i.e. slot h - 1 of the 1 based handle
//   (0x51c7ed..0x51c7f0), not slot h.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern void *rasterizer_device;                                                    // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing;                               // 0x0069c680
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count];             // 0x0065de00
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call4_fn)(void *self, uint32_t a, void *b, uint32_t c, uint32_t d);
typedef int32_t (__stdcall *d3d_call6_fn)(void *self, uint32_t a, uint32_t b, uint32_t c, void *d, uint32_t e, uint32_t f);

// blam-cc: ESI = vertex_buffer (live-in)
uint32_t rasterizer_dynamic_vertex_process_and_get_handle(rasterizer_vertex_buffer *vertex_buffer)
{
    d3d_call1_fn set_software_vertex_processing;
    d3d_call4_fn set_stream_source;
    d3d_call6_fn process_vertices;
    void **vtable;
    int16_t stride;
    uint32_t handle;

    stride = rasterizer_vertex_sizes[vertex_buffer->type];

    vtable = *(void ***)rasterizer_device;
    set_software_vertex_processing = (d3d_call1_fn)vtable[0x4d]; // +0x134
    set_software_vertex_processing(rasterizer_device,
        (-(uint32_t)(rasterizer_software_vertex_processing != 0) & 0x10) |
        (rasterizer_vertex_declarations[vertex_buffer->type].usage & 0x10));

    vtable = *(void ***)rasterizer_device;
    set_stream_source = (d3d_call4_fn)vtable[0x64]; // +0x190
    set_stream_source(rasterizer_device, 0, (void *)vertex_buffer->hardware_buffer, 0,
                       (uint32_t)stride);

    handle = (uint32_t)rasterizer_vertex_buffer_slots[rasterizer_dynamic_vertex_caches[_rasterizer_vertex_type_model_processed].buffer_handle - 1].hardware_buffer;

    vtable = *(void ***)rasterizer_device;
    process_vertices = (d3d_call6_fn)vtable[0x55]; // +0x154, ProcessVertices
    process_vertices(rasterizer_device, 0, 0, (uint32_t)vertex_buffer->count, (void *)handle, 0, 1);

    vtable = *(void ***)rasterizer_device;
    set_software_vertex_processing = (d3d_call1_fn)vtable[0x4d]; // +0x134
    set_software_vertex_processing(rasterizer_device, rasterizer_software_vertex_processing);

    return handle;
}

#if 0
Original Ghidra decompilation (0x51c790):

undefined4 FUN_0051c790(void)

{
  short sVar1;
  undefined4 uVar2;
  short *unaff_ESI;

  sVar1 = *(short *)(&DAT_0065de00 + *unaff_ESI * 2);
  (**(code **)(*DAT_0071d174 + 0x134))
            (DAT_0071d174,
             -(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[*unaff_ESI * 3] & 0x10);
  (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,*(undefined4 *)(unaff_ESI + 8),0,(int)sVar1);
  uVar2 = *(undefined4 *)(&DAT_007bf04c + DAT_006d99a4 * 10);
  (**(code **)(*DAT_0071d174 + 0x154))(DAT_0071d174,0,0,*(undefined4 *)(unaff_ESI + 2),uVar2,0,1);
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
