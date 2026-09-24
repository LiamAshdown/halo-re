// ui_wide_string_has_non_whitespace  (Ghidra: FUN_004a8b10, renamed)
// address 0x4a8b10, size 55 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.65
// evidence: phase-4 summary "Scans a wide string (pointer in EAX) and returns true if it
// contains at least one non-whitespace character"; iswctype mask 0x8 is MSVC's _SPACE class.
// register convention: string pointer in EAX (in_EAX, unresolved register read).
//   // blam-cc: EAX -> text
// FIXED (register inputs, objdump): EAX carries text (read at 0x4a8b12, mov esi,eax); the old
// note wrote "text -> EAX" (name first, "->" separator) which the checker's parser does not
// recognize -- only "EAX -> text" or "text in EAX" forms are.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#define WCTYPE_SPACE 0x0008
extern int32_t _iswctype(uint16_t ch, int32_t mask);

// blam-cc: EAX -> text
// Returns true as soon as a non-whitespace wide character is found, or false if the string is
// all whitespace (or empty).
uint8_t ui_wide_string_has_non_whitespace(const uint16_t *text)
{
    uint16_t ch = *text;
    while (ch != 0) {
        if (!_iswctype(ch, WCTYPE_SPACE)) {
            return 1;
        }
        text = text + 1;
        ch = *text;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4a8b10):

undefined4 FUN_004a8b10(void)

{
  wint_t wVar1;
  wint_t *in_EAX;
  int iVar2;

  wVar1 = *in_EAX;
  while( true ) {
    if (wVar1 == 0) {
      return 0;
    }
    iVar2 = _iswctype(*in_EAX,8);
    if (iVar2 == 0) break;
    in_EAX = in_EAX + 1;
    wVar1 = *in_EAX;
  }
  return 1;
}
#endif
