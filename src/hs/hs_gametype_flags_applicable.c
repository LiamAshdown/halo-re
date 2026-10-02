// hs_gametype_flags_applicable  (Ghidra: FUN_00483600; renamed per types/hs.h, which already
// documents this address by this name: "Bit 0 is handled separately by
// hs_gametype_flags_applicable @0x483600, which then tests bits 1..6.")
// address 0x483600, size 140 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: bit 0 (_hs_context_default_bit) of hs_autocomplete_gametype_mask is checked directly
// (required in the low byte, forbidden in the high byte, same shape as
// hs_gametype_flag_satisfied), then bits 1..6 (ctf, slayer, oddball, king, terminator, race) are
// each checked via hs_gametype_flag_satisfied; the function/global is applicable only if every
// one passes.
// register convention: the tested capability byte is unrecognized by Ghidra (unaff_BL); by the
// blam-cc convention this is the low byte of the fourth register slot, EBX.
// reconciled: R31 hs_gametype_flags bits are console command contexts (default/host/client-forbidden/mp engine/no mp engine/unknown_20/always), not game types; comments only

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char hs_gametype_flag_satisfied(uint8_t bit_index, uint8_t flags); // 0x004835b0, this batch

extern uint16_t hs_autocomplete_gametype_mask; // 0x006b14ac

// blam-cc: capability byte in BL (EBX)
// Returns 1 if every one of the console command context bits (0 through
// _hs_context_always_bit) required or forbidden by hs_autocomplete_gametype_mask is
// satisfied by `flags`, 0 otherwise.
uint8_t hs_gametype_flags_applicable(uint8_t flags)
{
    uint8_t result;

    result = 1;
    if ((int16_t)hs_autocomplete_gametype_mask != 0) {
        if (((hs_autocomplete_gametype_mask & 1) == 0) ||
            (result = (uint8_t)(flags & 1), result != 0)) {
            if ((hs_autocomplete_gametype_mask & 0x100) != 0) {
                result = (uint8_t)(~flags & 1);
            }
            if (result != 0) {
                if ((hs_gametype_flag_satisfied(1, flags) != 0) &&
                    (hs_gametype_flag_satisfied(2, flags) != 0) &&
                    (hs_gametype_flag_satisfied(3, flags) != 0) &&
                    (hs_gametype_flag_satisfied(4, flags) != 0) &&
                    (hs_gametype_flag_satisfied(6, flags) != 0) &&
                    (hs_gametype_flag_satisfied(5, flags) != 0)) {
                    return 1;
                }
            }
        }
        result = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x483600):

byte FUN_00483600(void)

{
  byte bVar1;
  char cVar2;
  byte unaff_BL;

  bVar1 = 1;
  if ((short)DAT_006b14ac != 0) {
    if (((DAT_006b14ac & 1) == 0) || (bVar1 = unaff_BL & 1, bVar1 != 0)) {
      if ((DAT_006b14ac & 0x100) != 0) {
        bVar1 = ~unaff_BL & 1;
      }
      if (bVar1 != 0) {
        cVar2 = FUN_004835b0(1);
        if (cVar2 != '\0') {
          cVar2 = FUN_004835b0(2);
          if (cVar2 != '\0') {
            cVar2 = FUN_004835b0(3);
            if (cVar2 != '\0') {
              cVar2 = FUN_004835b0(4);
              if (cVar2 != '\0') {
                cVar2 = FUN_004835b0(6);
                if (cVar2 != '\0') {
                  cVar2 = FUN_004835b0(5);
                  if (cVar2 != '\0') {
                    return 1;
                  }
                }
              }
            }
          }
        }
      }
    }
    bVar1 = 0;
  }
  return bVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
