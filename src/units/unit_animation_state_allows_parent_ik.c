// unit_animation_state_allows_parent_ik  (Ghidra: FUN_00565d60)
// address 0x565d60, size 31 bytes
// name confidence: 0.35 (its only caller, unit_update_ik_detail_nodes, asks it before
//   attaching the unit's IK nodes to its parent object)   rewrite confidence: 0.9
// evidence: objdump -d 0x565d60..0x565d7e plus its tables: jump table 0x565d80 {0x565d7c
//   "return 0", 0x565d7e "return 1"} and byte map 0x565d88 for animation states 0x17..0x23.
//   ECX is &unit->animation_state_flags (unit + 0x298), so +0x0b is animation_state
//   (unit 0x2a3).
// register convention: ECX = &unit->animation_state_flags, result in AL.
//   // blam-cc: ECX -> animation_block

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ECX -> animation_block
// False for the animation states 0x17..0x1b, 0x1d and 0x22..0x23; true for every other state.
uint8_t unit_animation_state_allows_parent_ik(uint8_t *animation_block)
{
    int32_t state_index = (int32_t)*(int8_t *)(animation_block + 0x0b) - 0x17; // animation_state

    if ((uint32_t)state_index > 0x0c) {
        return 1;
    }
    // byte map 0x565d88: {0,0,0,0,0,1,0,1,1,1,1,0,0}
    return (state_index == 5 || (state_index >= 7 && state_index <= 10)) ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x565d60): Ghidra shows the entry only (zero visible
arguments, switch unresolved); the tables above were read from the binary directly.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
