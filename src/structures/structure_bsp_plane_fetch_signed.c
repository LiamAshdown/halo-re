// structure_bsp_plane_fetch_signed  (orphan pass 4: FUN_0044dad0, no Ghidra name)
// address 0x44dad0, size 86 bytes
// name confidence: 0.4 (src/effects/README.md: "Indexed BSP plane fetch, negated when the
//   index's sign bit is set. Called with every register argument elided ... from
//   decal_flood_surfaces ... and from src/items/item_accelerate.c")
// rewrite confidence: 0.45 (control flow confirmed against the decompilation; the caller-side
//   struct layout and the meaning of a negative index are not independently re-verified here,
//   matching the README's own note that callers elide every register argument)
// evidence: out/phase4/effects_types_notes.md "0x44dad0 | structures | fetches an indexed BSP
//   plane, negating it for a negative index". types/math.h real_plane3d (size 0x10).
// register convention: EAX = real_plane3d *out, EDX = int32_t signed_index, and the one stack
//   argument (Ghidra's `param_1`, `mov esi,[esp+0x8]` at 0x44dad1) a ModelCollisionGeometryBSP-shaped
//   record whose +0x10 field is the real_plane3d plane array (the collision BSP's planes pointer).
// blam-cc: structure_bsp_plane_fetch_signed(real_plane3d *out /*EAX*/, void *planes_owner /*param_1*/,
//   int32_t signed_index /*EDX*/)
// Orphan pass 4 review (objdump 0x44dad0..0x44db25): the plane is planes[signed_index &
//   0x7fffffff] (`and ecx,0x7fffffff` at 0x44dad7) -- bit 31 is the collision-BSP "flipped plane"
//   flag, not a sign -- and is negated when that bit is set (`test edx,edx; jns`). The draft indexed
//   with the raw value, reading far outside the array for a flipped plane.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index)
{
    real_plane3d *plane = (real_plane3d *)((uint8_t *)*(void **)((uint8_t *)planes_owner + 0x10) +
        (signed_index & 0x7fffffff) * 0x10);

    if (signed_index < 0) {
        out->normal.i = -plane->normal.i;
        out->normal.j = -plane->normal.j;
        out->normal.k = -plane->normal.k;
        out->d = -plane->d;
    } else {
        out->normal.i = plane->normal.i;
        out->normal.j = plane->normal.j;
        out->normal.k = plane->normal.k;
        out->d = plane->d;
    }
}

#if 0
Original Ghidra decompilation (0x44dad0):

void FUN_0044dad0(int param_1)

{
  float *in_EAX;
  float *pfVar1;
  int in_EDX;

  pfVar1 = (float *)(in_EDX * 0x10 + *(int *)(param_1 + 0x10));
  if (in_EDX < 0) {
    *in_EAX = -*pfVar1;
    in_EAX[1] = -pfVar1[1];
    in_EAX[2] = -pfVar1[2];
    in_EAX[3] = -pfVar1[3];
    return;
  }
  *in_EAX = *pfVar1;
  in_EAX[1] = pfVar1[1];
  in_EAX[2] = pfVar1[2];
  in_EAX[3] = pfVar1[3];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
