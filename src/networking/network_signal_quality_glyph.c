// network_signal_quality_glyph  (Ghidra: FUN_00440610, still unnamed -> renamed)
// address 0x440610, size 48 bytes
// name confidence: 0.25   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("maps a small integer enum value to
// a fixed output byte via a lookup switch, purpose unconfirmed"); `python tools/pack.py
// 0x440610`. The returned bytes (0x2b '+', 0x37 '7', 0x38 '8', 0x39 '9', 0x2e '.', 0x31 '1')
// are plain ASCII digits/punctuation, which in Blam's HUD font mapping typically select
// icon glyphs (e.g. connection-quality bar icons); this is a guess and not confirmed by any
// caller in this batch.
// register convention: lookup code in EAX (in_EAX), no stack arguments.
// UNSURE: the meaning of the input code and of each returned glyph byte; only the switch
// table itself is certain.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: lookup code in EAX (in_EAX)
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t network_signal_quality_glyph(uint32_t code)
{
    switch (code) {
    case 3: return 0x37;
    case 4: return 0x38;
    case 5: return 0x39;
    case 6: return 0x2e;
    case 8: return 0x31;
    default: return 0x2b;
    }
}

#if 0
Original Ghidra decompilation (0x440610):

undefined4 FUN_00440610(void)

{
  undefined4 in_EAX;

  switch(in_EAX) {
  default:
    return 0x2b;
  case 3:
    return 0x37;
  case 4:
    return 0x38;
  case 5:
    return 0x39;
  case 6:
    return 0x2e;
  case 8:
    return 0x31;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
