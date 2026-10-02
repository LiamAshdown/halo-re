// rasterizer_lens_flare_occlusion_queries_create  (Ghidra: rasterizer_lens_flare_occlusion_queries_create, already named)
// address 0x536f70, size 113 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: functions.md summary ("Creates one Direct3D occlusion query per lens-flare slot (up
//   to 1024), used to fade flares based on their visibility, disabling the feature if the driver
//   does not support occlusion queries"); CreateQuery(Type=9 OCCLUSION, ppQuery) at device +0x1d8
//   matches the IDirect3DDevice9 vtable; -0x7789f796 (0x8876086a) is D3DERR_NOTAVAILABLE.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device; // 0x0071d174
extern uint8_t lens_flare_occlusion_queries_supported; // 0x006e1dc0
extern void *lens_flare_occlusion_queries[k_lens_flare_occlusion_queries]; // 0x006e1dc8

typedef int32_t (__stdcall *d3d_create_query_fn)(void *device, uint32_t type, void **out_query);

// Creates one Direct3D occlusion query per lens-flare slot (up to 1024), used to fade flares based
// on their visibility, disabling the feature if the driver does not support occlusion queries.
uint8_t rasterizer_lens_flare_occlusion_queries_create(void)
{
    void **vt = *(void ***)rasterizer_device;
    d3d_create_query_fn create_query = (d3d_create_query_fn)vt[0x1d8 / 4];
    uint8_t ok = 1;
    int i;

    lens_flare_occlusion_queries_supported = 1;
    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        lens_flare_occlusion_queries[i] = 0;
    }

    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        void *query = 0;
        int32_t hr;
        if (!ok) {
            break;
        }
        hr = create_query(rasterizer_device, 9, &query);
        if (hr < 0) {
            ok = 0;
            if (hr == (int32_t)0x8876086a) { // D3DERR_NOTAVAILABLE
                lens_flare_occlusion_queries_supported = 0;
            }
        } else {
            lens_flare_occlusion_queries[i] = query;
        }
    }
    return (lens_flare_occlusion_queries_supported != 0) == ok;
}

#if 0
Original Ghidra decompilation (0x536f70):

bool __cdecl rasterizer_lens_flare_occlusion_queries_create(void)

{
  bool bVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_4;

  bVar1 = true;
  DAT_006e1dc0 = '\x01';
  puVar3 = &DAT_006e1dc8;
  for (iVar2 = 0x400; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = &DAT_006e1dc8;
  do {
    if (!bVar1) break;
    iVar2 = (**(code **)(*DAT_0071d174 + 0x1d8))(DAT_0071d174,9,&local_4);
    if (iVar2 < 0) {
      bVar1 = false;
      if (iVar2 == -0x7789f796) {
        DAT_006e1dc0 = '\0';
      }
    }
    else {
      *puVar3 = local_4;
    }
    puVar3 = puVar3 + 1;
  } while ((int)puVar3 < 0x6e2dc8);
  return (bool)DAT_006e1dc0 == bVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
