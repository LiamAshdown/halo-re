// rasterizer_service_deferred_windowed_ops  (Ghidra: FUN_005180d0, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Services deferred windowed-mode device
// operations (an update/present and a clear) flagged by earlier code.")
// address 0x5180d0, size 89 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: gated on rasterizer_fullscreen and rasterizer_device; rasterizer_pending_clear/
//   rasterizer_device_usable-adjacent flags (0x0071d16e/0x0071d16f, types/rasterizer.h) each
//   drive one device vtable call (+0xa8, +0x44) and are then reset.
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t rasterizer_fullscreen;                               // 0x0071d16c (the header called it windowed)
extern void *rasterizer_device;     // 0x0071d174
extern uint8_t rasterizer_in_scene;                                 // 0x0071d16f set after BeginScene, cleared after EndScene
extern uint8_t rasterizer_pending_clear; // 0x0071d16e

typedef int32_t (__stdcall *d3d_call0_fn)(void *device);
typedef int32_t (__stdcall *d3d_call4_fn)(void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

// While windowed and the device exists: performs a deferred present/update (+0xa8) if flagged,
// and a deferred clear (+0x44, all zero args) if flagged, clearing each flag afterward.
void rasterizer_service_deferred_windowed_ops(void)
{
    void **vtable;

    if (rasterizer_fullscreen == 0 || rasterizer_device == (void *)0) {
        return;
    }
    if (rasterizer_in_scene != 0) {
        vtable = *(void ***)rasterizer_device;
        ((d3d_call0_fn)vtable[0xa8 / 4])(rasterizer_device);
        rasterizer_in_scene = 0;
    }
    if (rasterizer_pending_clear != 0) {
        vtable = *(void ***)rasterizer_device;
        ((d3d_call4_fn)vtable[0x44 / 4])(rasterizer_device, 0, 0, 0, 0);
        rasterizer_pending_clear = (rasterizer_pending_clear == 0);
    }
}

#if 0
Original Ghidra decompilation (0x5180d0):

void FUN_005180d0(void)

{
  if ((DAT_0071d16c != '\0') && (DAT_0071d174 != (int *)0x0)) {
    if (DAT_0071d16f != '\0') {
      (**(code **)(*DAT_0071d174 + 0xa8))(DAT_0071d174);
      DAT_0071d16f = '\0';
    }
    if (DAT_0071d16e != '\0') {
      (**(code **)(*DAT_0071d174 + 0x44))(DAT_0071d174,0,0,0,0);
      DAT_0071d16e = DAT_0071d16e == '\0';
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
