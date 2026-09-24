// text_char_is_double_byte  (Ghidra: text_char_is_double_byte, already named)
// address 0x557750, size 266 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md documents the five lead/trail byte-range
//   tables as the DBCS code pages text_encoding_state selects between (Shift-JIS, EUC,
//   Big5, Korean UHC/cp949, Korean Johab -- the language names are inferred from the
//   ranges, the binary only has the numbers 1..5). "ibukprlctn" (0x00671fa0) is the set
//   of letters that make a '|'-prefixed pair a two-byte markup escape rather than a
//   literal character. FUN_006257e0 is the CRT strchr, kept unnamed per the existing
//   house convention for that address (see src/interface/virtual_keyboard_character_is_legal.c).
// register convention: EAX = string (pointer to the two bytes to classify), confirmed by
//   objdump 0x557750 "mov bl,[eax]" / "mov al,[eax+1]" at entry; return value in AL.
//   No stack arguments.

#include "tags.h"
#include "memory.h"
#include "text.h"

extern int16_t text_encoding_state;      // 0x006e4800
extern char text_markup_codes[11]; // 0x00671fa0, "ibukprlctn"

extern char *FUN_006257e0(char *s, int ch); // 0x6257e0, CRT strchr, not this module

// blam-cc: EAX=string, no other arguments
// Classifies the byte pair at string[0..1]: true (1) when the two bytes must be consumed
// together as a single unit, either a '|x' markup escape (x one of "ibukprlctn") or a
// lead/trail pair of the active DBCS code page (text_encoding_state); false (0) for an ordinary
// single-byte character, NUL, or an encoding value with no code page (single-byte / out
// of range).
uint8_t text_char_is_double_byte(uint8_t *string)
{
    uint8_t lead, trail;
    uint8_t result;
    int trail_below_threshold; // bVar4
    int trail_at_boundary;     // bVar5, only meaningful across the shared jumps below

    lead = string[0];
    if (lead == 0) {
        return 0;
    }
    trail = string[1];
    result = 0;

    if (lead == 0x7c && trail != 0 && FUN_006257e0(text_markup_codes, trail) != (char *)0) {
        goto mark_double_byte;
    }

    switch (text_encoding_state) {
    case _text_encoding_shift_jis:
        if (lead < 0x81 || 0x9f < lead) {
            if (lead < 0xe0) return 0;
            if (lead == 0xff) return 0;
        }
        if (trail < 0x40) return 0;
        if (0xfc < trail) return 0;
        if (trail == 0x7f) return 0;
        return 1;

    case _text_encoding_euc:
        if (lead < 0xa1) return 0;
        if (lead == 0xff) return 0;
        trail_below_threshold = trail < 0xa1;
        break;

    case _text_encoding_big5:
        if (lead < 0x81) return 0;
        if (lead == 0xff) return 0;
        if (0x3f < trail && trail < 0x7f) goto mark_double_byte;
        trail_below_threshold = trail < 0xa1;
        break;

    case _text_encoding_korean_uhc:
        if (lead < 0x81) return 0;
        if (lead == 0xff) return 0;
        if (0x40 < trail && trail < 0x5b) goto mark_double_byte;
        if (0x60 < trail) {
            trail_below_threshold = trail < 0x7a;
            trail_at_boundary = trail == 0x7a;
            goto check_trail_boundary;
        }
        goto trail_below_0x81;

    case _text_encoding_korean_johab:
        if ((lead < 0x84 || 0xd3 < lead) && (lead < 0xd8 || 0xde < lead)) {
            if (lead < 0xe0) return 0;
            if (0xf9 < lead) return 0;
        }
        if (0x40 < trail) {
            trail_below_threshold = trail < 0x7e;
            trail_at_boundary = trail == 0x7e;
check_trail_boundary:
            if (trail_below_threshold || trail_at_boundary) goto mark_double_byte;
        }
trail_below_0x81:
        trail_below_threshold = trail < 0x81;
        break;

    default:
        return result;
    }

    if (!trail_below_threshold && trail != 0xff) {
mark_double_byte:
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x557750):

undefined1 text_char_is_double_byte(void)

{
  byte bVar1;
  byte bVar2;
  byte *in_EAX;
  int iVar3;
  bool bVar4;
  bool bVar5;
  undefined1 local_2;

  bVar1 = *in_EAX;
  if (bVar1 == 0) {
    return 0;
  }
  bVar2 = in_EAX[1];
  local_2 = 0;
  if (((bVar1 == 0x7c) && (bVar2 != 0)) && (iVar3 = FUN_006257e0("ibukprlctn",bVar2), iVar3 != 0))
  goto LAB_0055784e;
  switch(DAT_006e4800) {
  case 1:
    if ((bVar1 < 0x81) || (0x9f < bVar1)) {
      if (bVar1 < 0xe0) {
        return 0;
      }
      if (bVar1 == 0xff) {
        return 0;
      }
    }
    if (bVar2 < 0x40) {
      return 0;
    }
    if (0xfc < bVar2) {
      return 0;
    }
    if (bVar2 == 0x7f) {
      return 0;
    }
    return 1;
  case 2:
    if (bVar1 < 0xa1) {
      return 0;
    }
    if (bVar1 == 0xff) {
      return 0;
    }
    bVar4 = bVar2 < 0xa1;
    break;
  case 3:
    if (bVar1 < 0x81) {
      return 0;
    }
    if (bVar1 == 0xff) {
      return 0;
    }
    if ((0x3f < bVar2) && (bVar2 < 0x7f)) goto LAB_0055784e;
    bVar4 = bVar2 < 0xa1;
    break;
  case 4:
    if (bVar1 < 0x81) {
      return 0;
    }
    if (bVar1 == 0xff) {
      return 0;
    }
    if ((0x40 < bVar2) && (bVar2 < 0x5b)) goto LAB_0055784e;
    if (0x60 < bVar2) {
      bVar4 = bVar2 < 0x7a;
      bVar5 = bVar2 == 0x7a;
      goto LAB_00557844;
    }
    goto LAB_00557846;
  case 5:
    if (((bVar1 < 0x84) || (0xd3 < bVar1)) && ((bVar1 < 0xd8 || (0xde < bVar1)))) {
      if (bVar1 < 0xe0) {
        return 0;
      }
      if (0xf9 < bVar1) {
        return 0;
      }
    }
    if (0x40 < bVar2) {
      bVar4 = bVar2 < 0x7e;
      bVar5 = bVar2 == 0x7e;
LAB_00557844:
      if (bVar4 || bVar5) goto LAB_0055784e;
    }
LAB_00557846:
    bVar4 = bVar2 < 0x81;
    break;
  default:
    goto switchD_0055779f_default;
  }
  if ((!bVar4) && (bVar2 != 0xff)) {
LAB_0055784e:
    local_2 = 1;
  }
switchD_0055779f_default:
  return local_2;
}
#endif
