// server_browser_gametype2_flags_pack  (Ghidra: FUN_00576920; named per this rewrite)
// address 0x576920, size 57 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary: "Encodes 3 boolean flags into a compact
// code tagged with type id 2, part of a family of per-game-type option packers."
// register convention: 3-byte boolean array in EAX (in_EAX).
// blam-cc: EAX -> flags

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: EAX -> flags
// Encodes 3 boolean flags into a compact code tagged with type id 2 (the low 3 bits).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t server_browser_gametype2_flags_pack(uint8_t *flags)
{
    return (((uint32_t)(flags[2] != 0) << 1 | (uint32_t)(flags[1] != 0)) << 1 |
            (uint32_t)(flags[0] != 0)) << 3 | 2;
}

#if 0
Original Ghidra decompilation (0x576920):

uint FUN_00576920(void)

{
  char *in_EAX;

  return (((uint)(in_EAX[2] != '\0') << 1 | (uint)(in_EAX[1] != '\0')) << 1 |
         (uint)(*in_EAX != '\0')) << 3 | 2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
