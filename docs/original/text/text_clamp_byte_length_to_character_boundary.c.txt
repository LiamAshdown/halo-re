// text_clamp_byte_length_to_character_boundary  (Ghidra: FUN_00557720; renamed here)
// address 0x557720, size 40 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase2/results/text_00.json: "Adjusts an in-place byte-length value so it
//   lands on a valid (non-split) DBCS character boundary." Nearly identical loop to
//   text_find_character_boundary (0x5576d0), but only rewrites the length in place.
// register convention: Ghidra's decompile dropped the string pointer entirely (it never
//   names unaff_EBX). objdump 0x557720 shows "add eax,ebx" feeding the
//   text_char_is_double_byte call, with EBX never loaded in this function -- a genuine
//   caller-set register argument. EDI = length_inout (in/out), read at entry
//   ("cmp WORD PTR [edi],si") and written at the end ("mov WORD PTR [edi],si"). No stack
//   arguments.

#include "tags.h"
#include "memory.h"
#include "text.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t text_char_is_double_byte(uint8_t *string); // 0x557750

// blam-cc: EBX=string, EDI=length_inout
// Walks string one character at a time (1 or 2 bytes, DBCS-aware) while the running
// offset is below *length_inout, then rewrites *length_inout with the offset where the
// walk stopped: the first character boundary at or AFTER its original value (a length
// that splits a double-byte pair is rounded up by one, 0x557744 cmp si,[edi] / jl).
// A length of 0 or less becomes 0.
void text_clamp_byte_length_to_character_boundary(uint8_t *string, int16_t *length_inout)
{
    int16_t position;

    position = 0;
    if (0 < *length_inout) {
        do {
            if (text_char_is_double_byte(string + position)) {
                position += 2;
            } else {
                position += 1;
            }
        } while (position < *length_inout);
    }
    *length_inout = position;
}

#if 0
Original Ghidra decompilation (0x557720):

void FUN_00557720(void)

{
  char cVar1;
  short sVar2;
  short *unaff_EDI;

  sVar2 = 0;
  if (0 < *unaff_EDI) {
    do {
      cVar1 = text_char_is_double_byte();
      if (cVar1 == '\0') {
        sVar2 = sVar2 + 1;
      }
      else {
        sVar2 = sVar2 + 2;
      }
    } while (sVar2 < *unaff_EDI);
  }
  *unaff_EDI = sVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
