// transparent_geometry_group_set_drawn_bit  (Ghidra: FUN_00515370, unnamed; named from its
// behaviour and its sibling _test_drawn_bit @0x515310)
// address 0x515370, size 129 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: same pool bounds check and bit index as transparent_geometry_group_test_drawn_bit.c;
//   param_1 == 0 sets the bit, nonzero clears it.
// register convention: group pointer in in_EAX, clear-flag in the recognized stack parameter.
//   // blam-cc: EAX -> group, stack -> clear

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern int32_t transparent_geometry_group_count;                // 0x0071d154
extern uint32_t transparent_geometry_group_drawn_bits[12];      // 0x006d983c

// blam-cc: EAX -> group, stack -> clear
// Sets (clear == 0) or clears (clear != 0) `group`'s drawn bit; a no-op if `group` is out of
// range.
void transparent_geometry_group_set_drawn_bit(transparent_geometry_group *group, uint8_t clear)
{
    uint8_t *base = (uint8_t *)transparent_geometry_groups;
    uint8_t *p = (uint8_t *)group;
    int32_t index;

    if (!(base <= p && p < base + (uint32_t)transparent_geometry_group_count * 0xa8)) {
        return;
    }
    index = (int32_t)(p - base) / 0xa8;
    if (index == -1) {
        return;
    }
    if (clear == 0) {
        transparent_geometry_group_drawn_bits[index >> 5] |= (1u << (index & 0x1f));
    } else {
        transparent_geometry_group_drawn_bits[index >> 5] &= ~(1u << (index & 0x1f));
    }
}

#if 0
Original Ghidra decompilation (0x515370):

void FUN_00515370(char param_1)

{
  short sVar1;
  uint in_EAX;
  int iVar2;

  if ((DAT_0071d14c <= in_EAX) && (in_EAX < DAT_0071d154 * 0xa8 + DAT_0071d14c)) {
    iVar2 = in_EAX - DAT_0071d14c;
    sVar1 = ((short)(iVar2 / 0xa8) + (short)(iVar2 >> 0x1f)) -
            (short)((longlong)iVar2 * 0x30c30c31 >> 0x3f);
    if (sVar1 != -1) {
      iVar2 = (int)sVar1 >> 5;
      if (param_1 == '\0') {
        (&DAT_006d983c)[iVar2] = (&DAT_006d983c)[iVar2] | 1 << ((byte)sVar1 & 0x1f);
        return;
      }
      (&DAT_006d983c)[iVar2] = (&DAT_006d983c)[iVar2] & ~(1 << ((byte)sVar1 & 0x1f));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
