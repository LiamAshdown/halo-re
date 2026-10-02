// rasterizer_round_up_resolution_height  (Ghidra: rasterizer_round_up_resolution_height, already
// named)
// address 0x5195f0, size 182 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: a pure lookup ladder over vertical resolution thresholds; matches its own name and
//   the functions.md summary exactly.
// register convention: input height in in_EAX. // blam-cc: EAX -> height

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> height
// Rounds an input vertical screen resolution up to the nearest value in a table of common
// display heights.
int32_t rasterizer_round_up_resolution_height(int32_t height)
{
    if (height < 0x240) return 0x1e0;
    if (height < 0x241) return 0x240;
    if (height < 0x259) return 600;
    if (height < 0x2d1) return 0x2d0;
    if (height < 0x301) return 0x300;
    if (height < 0x361) return 0x360;
    if (height < 0x385) return 900;
    if (height < 0x3c1) return 0x3c0;
    if (height < 0x401) return 0x400;
    if (height < 0x41b) return 0x41a;
    if (height < 0x439) return 0x438;
    if (height < 0x4b1) return 0x4b0;
    if (height < 0x5a1) return 0x5a0;
    if (height <= 0x640) return 0x640;
    return 0x870;
}

#if 0
Original Ghidra decompilation (0x5195f0):

undefined4 rasterizer_round_up_resolution_height(void)

{
  int in_EAX;
  undefined4 uVar1;

  if (in_EAX < 0x240) {
    return 0x1e0;
  }
  if (in_EAX < 0x241) {
    return 0x240;
  }
  if (in_EAX < 0x259) {
    return 600;
  }
  if (in_EAX < 0x2d1) {
    return 0x2d0;
  }
  if (in_EAX < 0x301) {
    return 0x300;
  }
  if (in_EAX < 0x361) {
    return 0x360;
  }
  if (in_EAX < 0x385) {
    return 900;
  }
  if (in_EAX < 0x3c1) {
    return 0x3c0;
  }
  if (in_EAX < 0x401) {
    return 0x400;
  }
  if (in_EAX < 0x41b) {
    return 0x41a;
  }
  if (in_EAX < 0x439) {
    return 0x438;
  }
  if (in_EAX < 0x4b1) {
    return 0x4b0;
  }
  if (in_EAX < 0x5a1) {
    return 0x5a0;
  }
  uVar1 = 0x640;
  if (0x640 < in_EAX) {
    uVar1 = 0x870;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
