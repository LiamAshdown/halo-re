// text_find_character_boundary  (Ghidra: FUN_005576d0; renamed here)
// address 0x5576d0, size 70 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase2/results/text_00.json: "Scans a string to find the last valid
//   character boundary at or before a given byte length, so a string can be truncated
//   without splitting a double-byte character; returns the character code at that
//   boundary." Steps forward one character at a time via text_char_is_double_byte
//   (0x557750), stopping once the running offset would reach or pass *length_inout, then
//   writes back the last complete-character offset.
// register convention: both parameters are plain stack arguments (objdump 0x5576d0:
//   "mov ebp,[esp+0xc]" for string, "mov ecx,[esp+0x18]" for length_inout); no
//   register-only arguments.

#include "tags.h"
#include "memory.h"
#include "text.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t text_char_is_double_byte(uint8_t *string); // 0x557750

// Walks string one character at a time (1 or 2 bytes, DBCS-aware) until the running
// offset would reach or exceed *length_inout, then rewrites *length_inout with the
// offset of the last character boundary at or before that length and returns the
// character code at that boundary.
uint16_t text_find_character_boundary(uint8_t *string, int16_t *length_inout)
{
    int16_t position;
    int16_t boundary;
    uint16_t character;

    position = 0;
    do {
        boundary = position;
        if (text_char_is_double_byte(string + boundary)) {
            position = boundary + 2;
            character = (uint16_t)((string[boundary] << 8) | string[boundary + 1]);
        } else {
            character = (uint16_t)string[boundary];
            position = boundary + 1;
        }
    } while (position < *length_inout);

    *length_inout = boundary;
    return character;
}

#if 0
Original Ghidra decompilation (0x5576d0):

ushort FUN_005576d0(int param_1,short *param_2)

{
  char cVar1;
  ushort uVar2;
  byte *pbVar3;
  short sVar4;
  short sVar5;

  sVar5 = 0;
  do {
    sVar4 = sVar5;
    pbVar3 = (byte *)(sVar4 + param_1);
    cVar1 = text_char_is_double_byte();
    if (cVar1 == '\0') {
      uVar2 = (ushort)*pbVar3;
      sVar5 = sVar4 + 1;
    }
    else {
      sVar5 = sVar4 + 2;
      uVar2 = CONCAT11(*pbVar3,pbVar3[1]);
    }
  } while (sVar5 < *param_2);
  *param_2 = sVar4;
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
