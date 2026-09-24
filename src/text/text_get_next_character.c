// text_get_next_character  (Ghidra: FUN_005576a0; renamed here)
// address 0x5576a0, size 41 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase2/results/text_00.json: "Reads and returns the next character (1 or
//   2 bytes, DBCS-aware) from a string and advances the cursor offset." Uses
//   text_char_is_double_byte (0x557750) at string+*cursor to decide whether to consume
//   one or two bytes.
// register convention (objdump 0x5576a0..0x5576d0): EAX = string, ESI = cursor
//   (int16_t* byte offset into string, unaff_ESI). No stack arguments.

#include "tags.h"
#include "memory.h"
#include "text.h"

extern uint8_t text_char_is_double_byte(uint8_t *string); // 0x557750

// blam-cc: EAX=string, ESI=cursor
// Reads the character at string + *cursor: two bytes (lead << 8 | trail) if
// text_char_is_double_byte says the pair must be consumed together, otherwise one byte
// zero-extended. Advances *cursor by however many bytes were consumed.
uint16_t text_get_next_character(uint8_t *string, int16_t *cursor)
{
    uint8_t *here;

    here = string + *cursor;
    if (text_char_is_double_byte(here)) {
        *cursor += 2;
        return (uint16_t)((here[0] << 8) | here[1]);
    }
    *cursor += 1;
    return (uint16_t)here[0];
}

#if 0
Original Ghidra decompilation (0x5576a0):

ushort FUN_005576a0(void)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  int in_EAX;
  short *unaff_ESI;
  byte *pbVar4;

  pbVar4 = (byte *)(in_EAX + *unaff_ESI);
  cVar3 = text_char_is_double_byte();
  if (cVar3 != '\0') {
    bVar1 = *pbVar4;
    bVar2 = pbVar4[1];
    *unaff_ESI = *unaff_ESI + 2;
    return CONCAT11(bVar1,bVar2);
  }
  bVar1 = *pbVar4;
  *unaff_ESI = *unaff_ESI + 1;
  return (ushort)bVar1;
}
#endif
