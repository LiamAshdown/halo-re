// game_engine_ctf_unknown_84  (not a Ghidra function; the ctf game engine definition's +0x84 slot (unknown_84); no C existed, so that
//   stored pointer trapped as unlisted_4699e0)
// address 0x4699e0, size 13 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4699e0..0x4699ec: true for kind 0.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>



uint8_t game_engine_ctf_unknown_84(int32_t kind)
{
    return (uint8_t)(kind == 0);
}
