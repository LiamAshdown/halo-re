// transparent_geometry_group_test_drawn_bit  (Ghidra: FUN_00515310, unnamed; named from its
// behaviour and its sibling _set_drawn_bit @0x515370)
// address 0x515310, size 94 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: bounds-checks the pointer against the primary pool (same test as
//   transparent_geometry_group_index_from_pointer.c) and tests bit `index` of
//   transparent_geometry_group_drawn_bits[12] (types/rasterizer.h); the constant-division
//   sequence is the compiler's optimized /0xa8, replaced here with plain division.
// register convention: group pointer in in_EAX. // blam-cc: EAX -> group

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern int32_t transparent_geometry_group_count;                // 0x0071d154
extern uint32_t transparent_geometry_group_drawn_bits[12];      // 0x006d983c

// blam-cc: EAX -> group
// Returns 1 when `group`'s drawn bit is clear (out of range counts as clear), 0 when it is set.
uint8_t transparent_geometry_group_test_drawn_bit(transparent_geometry_group *group)
{
    uint8_t *base = (uint8_t *)transparent_geometry_groups;
    uint8_t *p = (uint8_t *)group;
    int32_t index = -1;

    if (base <= p && p < base + (uint32_t)transparent_geometry_group_count * 0xa8) {
        index = (int32_t)(p - base) / 0xa8;
    }
    if (index == -1) {
        return 1;
    }
    return (uint8_t)(1 - ((transparent_geometry_group_drawn_bits[index >> 5] & (1u << (index & 0x1f))) != 0));
}

#if 0
Original Ghidra decompilation (0x515310):

char FUN_00515310(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;
  short sVar3;

  sVar3 = -1;
  if ((DAT_0071d14c <= in_EAX) && (in_EAX < DAT_0071d154 * 0xa8 + DAT_0071d14c)) {
    iVar2 = in_EAX - DAT_0071d14c;
    sVar3 = ((short)(iVar2 / 0xa8) + (short)(iVar2 >> 0x1f)) -
            (short)((longlong)iVar2 * 0x30c30c31 >> 0x3f);
  }
  cVar1 = '\x01';
  if (sVar3 != -1) {
    cVar1 = '\x01' - ((1 << ((byte)sVar3 & 0x1f) & (&DAT_006d983c)[(int)sVar3 >> 5]) != 0U);
  }
  return cVar1;
}
#endif
