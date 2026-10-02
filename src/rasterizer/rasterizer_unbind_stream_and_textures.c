// rasterizer_unbind_stream_and_textures  (Ghidra: FUN_00518130, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Unbinds the first two texture stages and clears
// the current stream source/vertex declaration on the Direct3D device.")
// address 0x518130, size 78 bytes
// name confidence: 0.45  rewrite confidence: 0.6
// evidence: SetTexture(0/1, NULL) (+0x104), then two more device calls (+0x190 SetStreamSource,
//   +0x1a0 SetVertexDeclaration/SetFVF -- same slots rasterizer_dynamic_index_cache_draw.c's
//   comment identifies) with all-zero/NULL arguments.
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device; // 0x0071d174

typedef int32_t (__stdcall *d3d_set_texture_fn)(void *device, int32_t stage, void *texture);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *device, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);
typedef int32_t (__stdcall *d3d_call1_fn)(void *device, uint32_t a);

// Unbinds texture stages 0 and 1, clears the current stream source, and clears the current
// vertex declaration/FVF.
void rasterizer_unbind_stream_and_textures(void)
{
    void **vtable;
    int32_t stage;

    for (stage = 0; stage < 2; stage++) {
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_texture_fn)vtable[0x104 / 4])(rasterizer_device, stage, (void *)0);
    }

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_stream_source_fn)vtable[400 / 4])(rasterizer_device, 0, (void *)0, 0, 0);

    vtable = *(void ***)rasterizer_device;
    ((d3d_call1_fn)vtable[0x1a0 / 4])(rasterizer_device, 0);
}

#if 0
Original Ghidra decompilation (0x518130):

void FUN_00518130(void)

{
  int iVar1;
  int iVar2;

  iVar1 = 0;
  iVar2 = 2;
  do {
    (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,iVar1,0);
    iVar1 = iVar1 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,0,0,0);
  (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
