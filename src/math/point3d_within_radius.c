// point3d_within_radius  (Ghidra: FUN_0043c340, renamed)
// address 0x43c340, size 64 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase2/ai/08.md; caller ai_search_gather_obstacles @0x43c510 (confirmed via
//   objdump at the 0x43c62b call site: EAX = &obstacle.position (ebx+0xa0), ECX = &self_position
//   ([ebp+0xc]), stack float = [ebp+0x10] + ebx+0xac, a summed radius) passes two 3D point
//   pointers and a combined radius, matching a plain squared-distance-vs-radius^2 boolean test.
//   out/phase4/ai_types_notes.md lists this address among the "plain math helpers that belong
//   with types/math.h" living inside the ai address range.
// register convention (confirmed via objdump at 0x43c340 and the 0x43c62b call site): point a
//   in EAX (in_EAX), point b in ECX (in_ECX); radius as the recognized stack parameter
//   (param_1). Both return paths write the full 32-bit EAX (`mov eax,1` / `xor eax,eax`), so the
//   result is a plain int, not a truncated byte.
//   // blam-cc: EAX -> a, ECX -> b, stack -> radius

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Returns whether point `a` is within `radius` of point `b` (squared-distance comparison, no
// sqrt).
int point3d_within_radius(const real_point3d *a, const real_point3d *b, real radius)
{
    real dx = a->x - b->x;
    real dy = a->y - b->y;
    real dz = a->z - b->z;

    // Summation order from the FPU trace: (dx*dx + dz*dz) + dy*dy. `fcompp` of radius^2 against
    // the sum, then `test ah,0x1 / jne`: returns 0 when radius^2 < sum or unordered.
    if (dx * dx + dz * dz + dy * dy <= radius * radius) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x43c340):

undefined4 FUN_0043c340(float param_1)

{
  float *in_EAX;
  float *in_ECX;

  if ((in_EAX[1] - in_ECX[1]) * (in_EAX[1] - in_ECX[1]) +
      (in_EAX[2] - in_ECX[2]) * (in_EAX[2] - in_ECX[2]) + (*in_EAX - *in_ECX) * (*in_EAX - *in_ECX)
      <= param_1 * param_1) {
    return 1;
  }
  return 0;
}
#endif
