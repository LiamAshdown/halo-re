// rasterizer_dx9_create_vertex_buffer  (Ghidra: rasterizer_dx9_vertex_shader_create, misnamed)
// address 0x530570, size 122 bytes
// name confidence: 0.2   rewrite confidence: 0.75
//   Phase 4 rename: the body is CreateVertexBuffer (device +0x68); Ghidra name was wrong.
// evidence: out/phase4/rasterizer_types_notes.md: "Wraps CreateVertexBuffer (device +0x68) and
//   returns the buffer (raw code 0x5305dc); CreateVertexShader is +0x16c". Ghidra's own decompile
//   of the return statement is wrong -- it returns the creation-flags register (`uVar3`), but the
//   raw disassembly at 0x5305dc reloads EAX from the stack slot that holds the freshly created
//   IDirect3DVertexBuffer9*, then masks it to 0 on failure the same way. This rewrite follows the
//   raw code, not Ghidra's C. Left named as Ghidra named it (a real rename needs its callers, done
//   in rasterizer_vertex_buffer_slot_allocate.c / rasterizer_vertex_buffer_slot_recreate_lost.c
//   next in this file range).
// register convention: EAX -> vertex_type, stack -> (length, fvf, dynamic).
// blam-cc: EAX -> vertex_type, stack -> (length, fvf, dynamic)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device; // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

typedef int32_t (__stdcall *d3d_create_vertex_buffer_fn)(void *device, uint32_t length, uint32_t usage, uint32_t fvf,
                                               uint32_t pool, void **out_buffer, void *shared_handle);

// Creates one Direct3D vertex buffer (register EAX selects the vertex format, whose declaration
// usage bits are folded into the buffer's own Usage flags) from the given length/FVF/dynamic
// request, returning the created buffer or NULL on failure.
// UNSURE: `not_dynamic` sets D3DUSAGE_DYNAMIC (0x200) when it is ZERO, i.e. the raw bool this
// takes reads as "static"/"not dynamic" rather than "dynamic"; preserved as the code computes it.
void *rasterizer_dx9_create_vertex_buffer(int32_t vertex_type, uint32_t length, uint32_t fvf, uint8_t not_dynamic)
{
    uint32_t usage;
    uint32_t sw_flag;
    uint32_t dynamic_flag;
    uint32_t pool;
    void *buffer;
    int32_t hr;
    void **vt;
    d3d_create_vertex_buffer_fn create_vertex_buffer;

    sw_flag = (rasterizer_software_vertex_processing != 0) ? 0x10u : 0u;
    dynamic_flag = (not_dynamic == 0) ? 0x200u : 0u;
    usage = sw_flag | rasterizer_vertex_declarations[vertex_type].usage | dynamic_flag;

    pool = (sw_flag != 0 || (rasterizer_vertex_declarations[vertex_type].usage & 0x10) != 0 ||
           (rasterizer_vertex_declarations[vertex_type].usage & 0x200) != 0 || dynamic_flag != 0) ? 2u : 1u;

    buffer = 0;
    vt = *(void ***)rasterizer_device;
    create_vertex_buffer = (d3d_create_vertex_buffer_fn)vt[0x68 / 4];
    hr = create_vertex_buffer(rasterizer_device, length, usage, fvf, pool, &buffer, 0);
    return (hr >= 0) ? buffer : 0;
}

#if 0
Original Ghidra decompilation (0x530570):

uint rasterizer_dx9_vertex_shader_create(undefined4 param_1,undefined4 param_2,char param_3)

{
  uint uVar1;
  int in_EAX;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  undefined4 uVar6;
  undefined4 local_4;

  uVar1 = (&DAT_006e1a98)[in_EAX * 3];
  local_4 = 0;
  uVar2 = -(uint)(DAT_0069c680 != '\0') & 0x10;
  uVar5 = (param_3 != '\0') - 1 & 0x200;
  uVar3 = uVar2 | uVar1 | uVar5;
  if ((uVar2 != 0 || (uVar1 & 0x10) != 0) || (uVar6 = 1, (uVar1 & 0x200) != 0 || uVar5 != 0)) {
    uVar6 = 2;
  }
  iVar4 = (**(code **)(*DAT_0071d174 + 0x68))(DAT_0071d174,param_1,uVar3,param_2,uVar6,&local_4,0);
  return uVar3 & (iVar4 < 0) - 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
