// breakable_surface_is_intact  (was zone_light_table_test_bit; renamed in reconciliation R79)
// address 0x4ffda0, size 52 bytes
// name confidence: 0.75 (types/objects.h names and cites this exact address in its
//   object_zone_light_table struct comment: "queried by zone_light_table_test_bit")
// rewrite confidence: 0.6
// evidence: types/objects.h object_zone_light_table (membership[16][8] 0x0001); global
//   0x0069e8d8 current_local_player_index (established in
//   scenario_objects_place_for_structure_bsp.c).
// register convention: Ghidra shows a single unresolved `in_AX`, a 16-bit value; by the shift
//   arithmetic (`>> 5` then `& 0x1f` to pick a bit within one of the eight dwords of a group)
//   this is the bit index into the current local player's 256-bit membership set.
// blam-cc: AX -> bit_index
// reconciled: R79 0x006b8d78 is types/physics.h breakable_surface_globals and 0x0069e8d8 the structure BSP index: the test is 'is breakable surface bit_index of the current BSP still intact' (current_local_player_index -> global_structure_bsp_index, membership -> active)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, types/physics.h breakable_surface_globals
extern int16_t global_structure_bsp_index; // 0x0069e8d8, types/physics.h

int8_t breakable_surface_is_intact(int16_t bit_index /*AX*/) // blam-cc: AX -> bit_index
{
    if (bit_index != -1) {
        uint32_t word = breakable_surface_state->active[global_structure_bsp_index][bit_index >> 5];
        if ((word & (1 << (bit_index & 0x1f))) == 0) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ffda0):

undefined4 FUN_004ffda0(void)

{
  short in_AX;

  if ((in_AX != -1) &&
     ((*(uint *)(DAT_006b8d78 + 1 + (((int)in_AX >> 5) + DAT_0069e8d8 * 8) * 4) &
      1 << ((byte)in_AX & 0x1f)) == 0)) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
