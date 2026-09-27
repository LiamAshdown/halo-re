// hud_messaging_clear_after_load  (not a Ghidra function; game state after-load proc 12)
// address 0x4ae530, size 28 bytes
// name confidence: 0.3  rewrite confidence: 0.95
// evidence: game_state_after_load_procs[12] (0x0069e7e4) holds 0x4ae530.
// objdump 0x4ae530..0x4ae54b: clears the byte at +0x82 of each of the 4 records (0x8c apart) of the hud
//   messaging globals (0x006b3a40).
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"

extern uint8_t *hud_messaging; // 0x006b3a40

void hud_messaging_clear_after_load(void)
{
    int32_t i;

    for (i = 0; i < 4; i++) {
        hud_messaging[0x82 + i * 0x8c] = 0;
    }
}
