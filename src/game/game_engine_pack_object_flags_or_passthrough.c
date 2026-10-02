// game_engine_pack_object_flags_or_passthrough  (Ghidra: FUN_00462bd0; renamed per its summary)
// address 0x462bd0, size 24 bytes
// name confidence: 0.25   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Low-confidence: builds a packed flags/color byte for
// an object from the game engine option bitfield when not in the DAT_0087aa00-bit2 mode,
// otherwise passes the input through unchanged in its low byte"); types/game.h
// game_variant::flags (+0x38, aliased 0x006f1cc0), game_engine_unknown_aa00 (0x0087aa00).
// register convention: an input value in EAX (in_EAX), returned unchanged (low byte only) when
// game_engine_unknown_aa00 bit 2 is set.
//   // blam-cc: EAX -> input
// UNSURE: the packed result (bit 0 of game_variant::flags >> 2, combined with the upper 24 bits
// of flags >> 10 via Ghidra's CONCAT31) has no established meaning; transcribed as literally as
// C bit operations allow.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t game_engine_unknown_aa00; // 0x0087aa00
extern game_variant game_engine_variant; // 0x006f1c88 (flags aliased 0x006f1cc0)

// blam-cc: EAX -> input
uint32_t game_engine_pack_object_flags_or_passthrough(uint32_t input)
{
    uint32_t result = input & 0xffffff00u;

    if ((game_engine_unknown_aa00 & 4) == 0) {
        result = (((game_engine_variant.flags >> 10) << 8) |
                   (uint8_t)(game_engine_variant.flags >> 2)) & 0xffffff01u;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x462bd0), from tools/pack.py 0x462bd0:

uint FUN_00462bd0(void)

{
  uint in_EAX;
  uint uVar1;

  uVar1 = in_EAX & 0xffffff00;
  if ((DAT_0087aa00 & 4) == 0) {
    uVar1 = CONCAT31((uint3)(DAT_006f1cc0 >> 10),(char)(DAT_006f1cc0 >> 2)) & 0xffffff01;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
