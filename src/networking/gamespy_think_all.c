// gamespy_think_all  (Ghidra: FUN_006154f0; statically linked GameSpy SDK)
// address 0x6154f0, size 50 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: network_update 0x4e2b10 calls it every frame. objdump 0x6154f0..0x615521: when the GameSpy array at
//   0x006a26c8 exists, each of its elements, last to first, is passed to NegotiateThink (the per-element think).
//   First-boot track: reached from the first main-loop frame.
// blam-cc: none
#include "tags.h"
#include "memory.h"
#include "math.h"

extern void *gamespy_think_list; // 0x006a26c8
extern int32_t gamespy_array_length(void *array); // 0x6175f0
extern void *gamespy_array_nth(void *array, int32_t index); // 0x61dc00
extern void NegotiateThink(void *element); // 0x615360, GameSpy per-element think

void gamespy_think_all(void)
{
    int32_t i;

    if (gamespy_think_list == 0) {
        return;
    }
    for (i = gamespy_array_length(gamespy_think_list) - 1; i >= 0; i--) {
        NegotiateThink(gamespy_array_nth(gamespy_think_list, i));
    }
}
