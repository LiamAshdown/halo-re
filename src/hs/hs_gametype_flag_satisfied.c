// hs_gametype_flag_satisfied  (Ghidra: FUN_004835b0; renamed per types/hs.h, which already
// documents this address by this name: "the exact shape hs_gametype_flag_satisfied (0x4835b0)
// tests bit by bit for bits 0..6")
// address 0x4835b0, size 69 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: tests bit `bit_index` and bit `bit_index+8` of hs_autocomplete_gametype_mask (the
// low byte is "required", the high byte is "forbidden", per hs_autocomplete_gametype_mask_bits
// in types/hs.h) against the caller's console-command-context capability byte (R31: the bits
// are command contexts built by 0x4c69c0, not multiplayer game types).
// register convention: the tested capability byte is unrecognized by Ghidra (in_AL); by the
// blam-cc convention this is the low byte of the first register slot, EAX. bit_index is
// Ghidra-recognized directly.
//   // blam-cc: AL -> flags, stack -> bit_index
// FIXED (register inputs, objdump): AL carries flags (read at 0x4835c8, mov bl,al); the prose
// note already identified it but "capability byte in AL (EAX), bit_index the recognized
// parameter" has no machine-readable register mapping, so the checker saw no register at all.
// reconciled: R31 context bits are console command contexts, not game types (comments only)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern uint16_t hs_autocomplete_gametype_mask; // 0x006b14ac

// blam-cc: AL -> flags, stack -> bit_index
// Tests whether console-context bit `bit_index` (of `flags`) is satisfied by
// hs_autocomplete_gametype_mask: passes if the bit is not required or is present, and if the
// bit is not forbidden or is absent.
char hs_gametype_flag_satisfied(uint8_t bit_index, uint8_t flags)
{
    uint32_t bit_mask;
    uint8_t bit_mask_byte;
    char satisfied;

    bit_mask = 1u << (bit_index & 0x1f);
    bit_mask_byte = (uint8_t)bit_mask;
    satisfied = 1;
    if ((((bit_mask & (uint16_t)hs_autocomplete_gametype_mask) == 0) ||
         (satisfied = (char)((flags & bit_mask_byte) != 0), satisfied != 0)) &&
        (((uint16_t)hs_autocomplete_gametype_mask & (1u << ((bit_index + 8) & 0x1f))) != 0)) {
        satisfied = (char)(1 - ((flags & bit_mask_byte) != 0));
    }
    return satisfied;
}

#if 0
Original Ghidra decompilation (0x4835b0):

char FUN_004835b0(byte param_1)

{
  byte in_AL;
  char cVar1;
  byte bVar2;
  uint uVar3;

  uVar3 = 1 << (param_1 & 0x1f);
  cVar1 = true;
  bVar2 = (byte)uVar3;
  if ((((uVar3 & (ushort)DAT_006b14ac) == 0) || (cVar1 = (bVar2 & in_AL) != 0, (bool)cVar1)) &&
     (((uint)(ushort)DAT_006b14ac & 1 << (param_1 + 8 & 0x1f)) != 0)) {
    cVar1 = '\x01' - ((in_AL & bVar2) != 0);
  }
  return cVar1;
}
#endif
