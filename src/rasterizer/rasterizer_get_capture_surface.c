// rasterizer_get_capture_surface  (Ghidra: FUN_00515c30, unnamed; named from
// out/phase4/rasterizer_functions.md's summary)
// address 0x515c30, size 43 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: indexes rasterizer_capture_surfaces[4] (0x0069c66c, types/rasterizer.h) by a mode
//   field at object+10.
// register convention: object pointer in in_EAX, fallback value in in_ECX.
//   // blam-cc: EAX -> object, ECX -> fallback

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_capture_surfaces[4]; // 0x0069c66c

// blam-cc: EAX -> object, ECX -> fallback
// Returns the capture/blit render-target surface for the mode at object+10, or `fallback` for
// any other mode.
void *rasterizer_get_capture_surface(uint8_t *object, void *fallback)
{
    int16_t mode = *(int16_t *)(object + 10);
    switch (mode) {
    case 0:
    case 1:
        return rasterizer_capture_surfaces[1];
    case 2:
        return rasterizer_capture_surfaces[3];
    case 3:
        return rasterizer_capture_surfaces[0];
    default:
        return fallback;
    }
}

#if 0
Original Ghidra decompilation (0x515c30):

undefined4 FUN_00515c30(void)

{
  int in_EAX;
  undefined4 in_ECX;

  switch(*(undefined2 *)(in_EAX + 10)) {
  case 0:
  case 1:
    return DAT_0069c670;
  case 2:
    return DAT_0069c678;
  case 3:
    return DAT_0069c66c;
  default:
    return in_ECX;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
