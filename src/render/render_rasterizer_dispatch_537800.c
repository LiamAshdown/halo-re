// render_rasterizer_dispatch_537800  (Ghidra: FUN_00512190; new name, evidence below)
// address 0x512190, size 15 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// REWRITTEN from objdump 0x512190..0x51219e: push [esp+4] (radius); push ecx (the sample point); call 0x537800 with
//   EDI untouched (the caller's slot index, lens_flare_update_samples' loop counter); the callee's EAX (the sample
//   count) is returned. rasterizer_lens_flare_occlusion_test_issue is blam-cc EDI slot, stack (position, radius).
//   The draft returned void and forwarded (ECX, EDI) as two opaque stack values.
// blam-cc: EDI -> slot_index, ECX -> point, stack -> radius

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t rasterizer_lens_flare_occlusion_test_issue(int32_t slot_index, const real_point3d *position,
    float radius); // 0x537800, EDI slot, stack (position, radius)

int32_t render_rasterizer_dispatch_537800(int32_t slot_index, real_point3d *point, float radius)
{
    return rasterizer_lens_flare_occlusion_test_issue(slot_index, point, radius);
}

#if 0
Original Ghidra decompilation (0x512190):

void FUN_00512190(void)

{
  FUN_00537800();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
