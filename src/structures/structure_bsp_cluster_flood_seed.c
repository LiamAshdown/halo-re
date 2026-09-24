// structure_bsp_cluster_flood_seed  (Ghidra: FUN_00554cb0, still unnamed there)
// address 0x554cb0, size 94 bytes
// name confidence: 0.55 -- matches the phase4 summary ("Returns either the single starting
//   cluster (when the radius is non-positive) or the full set of clusters within the given radius
//   via a flood fill").
// rewrite confidence: 0.6 -- Ghidra's own decompile is short and clean but shows every callee call
//   with elided arguments; objdump disassembly resolves them, and also reveals that Ghidra's
//   "param_1" (typed undefined4, apparently unused) is in fact the query point pointer, forwarded
//   opaquely into cluster_flood_fill_within_radius without ever being dereferenced here.
// evidence: objdump -M intel disassembly of 0x554cb0..0x554d10, cross-checked against
//   cluster_flood_fill_within_radius.c (this batch) which resolves the same call from the other
//   side.
// register convention: 2 stack parameters (point, radius) plus 3 register-passed: in_CX ->
//   start_cluster, in_EDX -> output array, unaff_SI -> max_count.
//   // blam-cc: stack(point, radius), CX -> start_cluster, EDX -> output, SI -> max_count
// UNSURE: none left in this function's own body; cluster_flood_fill_within_radius's contract is
//   fully resolved (see that file).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern int32_t cluster_flood_stamp; // 0x006e3f04
extern uint8_t cluster_flood_in_progress; // 0x006e3f01

extern int32_t cluster_flood_fill_within_radius(int16_t cluster_index, real_point3d *point,
                                                 float tolerance, int32_t remaining_budget,
                                                 int16_t *output);

// blam-cc: stack(point, radius), CX -> start_cluster, EDX -> output, SI -> max_count
int32_t structure_bsp_cluster_flood_seed(real_point3d *point, float radius,
                                          int16_t start_cluster, int16_t *output,
                                          int16_t max_count)
{
    if (start_cluster == -1) {
        return 0;
    }
    if (radius > 0.0f) {
        cluster_flood_stamp++;
        cluster_flood_in_progress = 1;
        int32_t count = cluster_flood_fill_within_radius(start_cluster, point, radius, max_count,
                                                          output);
        cluster_flood_in_progress = 0;
        return count;
    }
    if (max_count > 0) {
        *output = start_cluster;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x554cb0):

undefined4 FUN_00554cb0(undefined4 param_1,float param_2)

{
  undefined4 uVar1;
  short in_CX;
  short *in_EDX;
  short unaff_SI;

  if (in_CX != -1) {
    if (0.0 < param_2) {
      DAT_006e3f04 = DAT_006e3f04 + 1;
      DAT_006e3f01 = 1;
      uVar1 = cluster_flood_fill_within_radius();
      DAT_006e3f01 = 0;
      return uVar1;
    }
    if (0 < unaff_SI) {
      *in_EDX = in_CX;
      return 1;
    }
  }
  return 0;
}
#endif
