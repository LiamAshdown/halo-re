// rasterizer_detail_object_vertex_buffer_create  (Ghidra: rasterizer_decal_dynamic_vertex_buffer_create)
// address 0x51b370, size 117 bytes
// name confidence: 0.7   rewrite confidence: 0.95
// evidence: CreateVertexBuffer (device +0x68) of 0x78000 bytes with fvf 0 and the usage of the
//   detail_object vertex declaration (0x006e1b1c = rasterizer_vertex_declarations[11].usage) plus
//   D3DUSAGE_WRITEONLY and, with software vertex processing, D3DUSAGE_SOFTWAREPROCESSING. The
//   buffer lands in 0x0071d1c8, which rasterizer_detail_objects_begin 0x51b3f0 binds with stride
//   0x14 and rasterizer_detail_objects_vertex_buffer_fill 0x51b6f0 locks whole.
//   Spot-check fix (phase 4 review): the earlier rewrite stored the usage mask in the global
//   ("alias analysis failure"); the raw code (0x51b3c3: mov edx,[esp+8]) stores the out pointer
//   CreateVertexBuffer wrote, or NULL on failure. It also computes the pool from the usage.
// register convention: none, __cdecl with no parameters; returns a bool in AL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                                     // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern void *rasterizer_detail_object_vertex_buffer;                // 0x0071d1c8

typedef int32_t (__stdcall *d3d_create_vertex_buffer_fn)(void *device, uint32_t length, uint32_t usage, uint32_t fvf,
                                               uint32_t pool, void **out_buffer, void *shared_handle);

uint8_t rasterizer_detail_object_vertex_buffer_create(void)
{
    void *buffer = 0;
    uint32_t usage = (rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                     rasterizer_vertex_declarations[_rasterizer_vertex_type_detail_object].usage | 0x200;
    uint32_t pool = (usage & 0x10) != 0 || (usage & 0x200) != 0 ? 2 : 1;
    void **vtable = *(void ***)rasterizer_device;
    int32_t hr = ((d3d_create_vertex_buffer_fn)vtable[0x68 / 4])(rasterizer_device, 0x78000, usage, 0, pool,
                                                                  &buffer, 0);

    rasterizer_detail_object_vertex_buffer = hr < 0 ? 0 : buffer;
    return (uint8_t)(rasterizer_detail_object_vertex_buffer != 0);
}

#if 0
Original Ghidra decompilation (0x51b370):

undefined4 __cdecl rasterizer_decal_dynamic_vertex_buffer_create(void)

{
  uint uVar1;
  int iVar2;
  undefined4 local_4;

  local_4 = 0;
  uVar1 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1b1c | 0x200;
  iVar2 = (**(code **)(*DAT_0071d174 + 0x68))(DAT_0071d174,0x78000,uVar1,0,2,&local_4,0);
  DAT_0071d1c8 = (iVar2 < 0) - 1 & uVar1;
  return CONCAT31((int3)(DAT_0071d1c8 >> 8),DAT_0071d1c8 != 0);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
