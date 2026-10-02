// text_find_character  (Ghidra: FUN_00557870; renamed here)
// address 0x557870, size 71 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md: "0x556bb0 reads strings 4, 5 and 6 as
//   character sets and searches them with 0x557870" -- used purely as a membership test
//   (is target_character present in string before its terminating NUL). objdump
//   0x557870..0x5578c0 confirmed.
// register convention: string is the sole stack argument ("mov ebp,[esp+0x8]" at entry,
//   right after the single "push ebp"). target_character is EBX (unaff_BX, "cmp ax,bx"),
//   never loaded in this function -- a genuine caller-set register argument.
// UNSURE: the phase 2 summary ("returning the byte offset and whether the character was
//   found") describes Ghidra's uVar3, but that value is reconstructed from whatever
//   garbage was left in EAX's upper bits by the preceding text_char_is_double_byte call
//   and the character read -- objdump shows the success path only ever sets AL (0x1) and
//   the failure path only clears AL (0x0); EDI, which holds the real running offset, is
//   never moved into EAX. The single caller (0x556bb0) only needs the found/not-found
//   answer (test al,al at 0x556e1c / 0x556e36 / 0x556e47), so this is rewritten as a
//   boolean returned in AL (uint8_t) and the offset is not returned.

#include "tags.h"
#include "memory.h"
#include "text.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t text_char_is_double_byte(uint8_t *string); // 0x557750

// blam-cc: EBX=target_character, stack=string
// Searches string (1 or 2 bytes per character, DBCS-aware) for target_character. Returns
// true if found before the terminating NUL, false otherwise.
uint8_t text_find_character(int16_t target_character, uint8_t *string)
{
    int16_t position;

    position = 0;
    for (;;) {
        uint8_t *here;
        uint16_t character;

        here = string + position;
        if (text_char_is_double_byte(here)) {
            character = (uint16_t)((here[0] << 8) | here[1]);
            position += 2;
        } else {
            character = (uint16_t)here[0];
            position += 1;
        }
        if (character == 0) {
            return 0;
        }
        if (character == (uint16_t)target_character) {
            return 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x557870):

int FUN_00557870(int param_1)

{
  ushort uVar1;
  undefined4 uVar2;
  uint3 uVar3;
  ushort unaff_BX;
  byte *pbVar4;
  short sVar5;

  sVar5 = 0;
  while( true ) {
    pbVar4 = (byte *)(sVar5 + param_1);
    uVar2 = text_char_is_double_byte();
    if ((char)uVar2 == '\0') {
      uVar1 = (ushort)*pbVar4;
      sVar5 = sVar5 + 1;
    }
    else {
      uVar1 = CONCAT11(*pbVar4,pbVar4[1]);
      sVar5 = sVar5 + 2;
    }
    uVar3 = (uint3)(CONCAT22((short)((uint)uVar2 >> 0x10),uVar1) >> 8);
    if (uVar1 == 0) break;
    if (uVar1 == unaff_BX) {
      return CONCAT31(uVar3,1);
    }
  }
  return (uint)uVar3 << 8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
