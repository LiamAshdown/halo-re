// unit_animation_state_allows_weapon_ik  (Ghidra: FUN_00565d00)
// address 0x565d00, size 46 bytes
// name confidence: 0.35 (its only caller, unit_update_ik_detail_nodes, asks it before
//   solving the weapon-hand IK nodes)   rewrite confidence: 0.9
// evidence: objdump -d 0x565d00..0x565d2d plus its tables: jump table 0x565d30 {0x565d2b
//   "return 0", 0x565d2d "return the flag"} and byte map 0x565d38 for animation states
//   0x10..0x29. ECX is &unit->animation_state_flags (unit + 0x298), so +0x0b is
//   animation_state (unit 0x2a3), +0x0c is overlay_animation_command and +0x1a is overlays[2].index
//   (unit 0x2b2).
// register convention: ECX = &unit->animation_state_flags, result in AL.
//   // blam-cc: ECX -> animation_block

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "fn_units.h"

// blam-cc: ECX -> animation_block
// True when the third animation overlay is unused, overlay_animation_command is clear and the current
// animation state is not one of the states 0x10..0x13, 0x17..0x23 and 0x27..0x29.
uint8_t unit_animation_state_allows_weapon_ik(uint8_t *animation_block)
{
    uint8_t result = *(int16_t *)(animation_block + 0x1a) == -1;   // overlays[2].index
    int32_t state_index;

    if (animation_block[0x0c] != 0) {                               // overlay_animation_command
        result = 0;
    }
    state_index = (int32_t)*(int8_t *)(animation_block + 0x0b) - 0x10; // animation_state
    if ((uint32_t)state_index <= 0x19) {
        // byte map 0x565d38: 1 for 0x14..0x16 and 0x24..0x26, 0 otherwise
        if (!((state_index >= 4 && state_index <= 6) || (state_index >= 0x14 && state_index <= 0x16))) {
            result = 0;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x565d00): Ghidra shows the entry only (zero visible
arguments, switch unresolved); the tables above were read from the binary directly.
#endif
