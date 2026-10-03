// cluster_sound_distance_lookup  (Ghidra: cluster_sound_distance_lookup, already named via
//   cea-pdb match)
// address 0x552210, size 68 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: unaff_EDI+0x134 is ScenarioStructureBSP.clusters.count (types/tags.h,
//   types/structures.h); unaff_EDI+0x220 lands inside ScenarioStructureBSP.sound_pas_data's own
//   TagDataOffset (0x214..0x228) at its +0x0c (pointer) sub-field -- "PAS" (potentially audible
//   set) is exactly the cluster-pair sound occlusion/distance matrix this name and summary
//   describe. The index arithmetic is the standard closed-form offset into a strictly
//   upper-triangular matrix (row `min(a,b)`, one byte per unordered cluster pair, diagonal
//   omitted): row_start(min) = (cluster_count-1)*min - min*(min+1)/2, then + (max - min - 1).
// register convention: EAX -> cluster_a, ECX -> cluster_b, EDI -> structure_bsp (unaff_EDI).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

// Looks up the cached one-byte sound-propagation distance between two clusters from the
// triangular distance matrix in the BSP tag's sound_pas_data block, or returns 0 when the two
// cluster indices are the same (no self-distance is stored).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b, ScenarioStructureBSP *structure_bsp)
    // blam-cc: EAX -> cluster_a, ECX -> cluster_b, EDI -> structure_bsp
{
    if (cluster_b != cluster_a) {
        int16_t lo = cluster_b;
        int16_t hi = cluster_a;
        int32_t index;
        uint8_t *sound_pas;

        if (hi < lo) {
            lo = cluster_a;
            hi = cluster_b;
        }

        index = (int32_t)(uint16_t)(structure_bsp->clusters.count - 1) * lo -
                ((int32_t)(lo + 1) * lo) / 2 - 1 + hi;
        sound_pas = (uint8_t *)structure_bsp->sound_pas_data.pointer;
        return sound_pas[index];
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x552210):

uint cluster_sound_distance_lookup(void)

{
  int iVar1;
  uint in_EAX;
  uint in_ECX;
  uint uVar2;
  int unaff_EDI;

  if ((short)in_ECX != (short)in_EAX) {
    uVar2 = in_ECX;
    if ((short)in_EAX < (short)in_ECX) {
      uVar2 = in_EAX;
      in_EAX = in_ECX;
    }
    iVar1 = ((ushort)(*(short *)(unaff_EDI + 0x134) - 1) * uVar2 -
            (((short)uVar2 + 1) * (int)(short)uVar2) / 2) + -1 + in_EAX;
    return CONCAT31((int3)((uint)iVar1 >> 8),
                    *(undefined1 *)((int)(short)iVar1 + *(int *)(unaff_EDI + 0x220)));
  }
  return in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
