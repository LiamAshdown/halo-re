// string_convert_unicode_to_ascii  (Ghidra: FUN_00557950; renamed here. The first rewrite
//   used the review-queue name wide_string_to_narrow_padded, but nothing is padded: the
//   copy stops at the source NUL. Renamed to mirror string_convert_ascii_to_unicode 0x557990.)
// address 0x557950, size 60 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase2/results/text_00.json: "Converts a wide-character string into a
//   fixed-capacity narrow (single-byte) buffer, replacing any non-ASCII wide characters
//   with spaces." wcslen (0x625b7a) is a plain cdecl wide-character strlen
//   (objdump: walks 16-bit units to the NUL, returns the count excluding it).
// register convention (objdump 0x557950..0x557990): the single "push edi" at entry both
//   saves nothing (there is no matching pop -- "add esp,4" right after the call cleans it
//   up instead) and supplies wcslen's own stack argument, i.e. wcslen(edi).
//   EDI = source, ESI = dest (both caller-set registers, unaff_EDI/unaff_ESI, never
//   loaded from the stack in this function). capacity is the sole stack argument.

#include "tags.h"
#include "memory.h"
#include "text.h"

extern uint32_t wcslen(const uint16_t *s); // 0x625b7a, wide strlen (wcslen), not this module

// blam-cc: ESI=dest, EDI=source, stack=capacity
// Copies source (UTF-16) into dest (narrow, single-byte) truncating each non-ASCII wide
// character (any nonzero high byte) to a space, NUL-terminating dest. Returns dest on
// success, or (void *)0 if source's length does not fit in capacity - 1 characters (leaving
// dest untouched).
uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity)
{
    uint32_t length;
    uint32_t i;

    length = wcslen(source);
    if (length > (uint32_t)(capacity - 1)) {
        return (uint8_t *)((void *)0);
    }
    for (i = 0; i < length; i++) {
        if ((source[i] & 0xff00) != 0) {
            dest[i] = 0x20;
        } else {
            dest[i] = (uint8_t)source[i];
        }
    }
    dest[i] = 0;
    return dest;
}

#if 0
Original Ghidra decompilation (0x557950):

int FUN_00557950(int param_1)

{
  uint uVar1;
  uint uVar2;
  int unaff_ESI;
  int unaff_EDI;

  uVar1 = FUN_00625b7a();
  if (uVar1 <= param_1 - 1U) {
    uVar2 = 0;
    if (uVar1 != 0) {
      do {
        if (*(char *)(unaff_EDI + 1 + uVar2 * 2) == '\0') {
          *(undefined1 *)(uVar2 + unaff_ESI) = *(undefined1 *)(unaff_EDI + uVar2 * 2);
        }
        else {
          *(undefined1 *)(uVar2 + unaff_ESI) = 0x20;
        }
        uVar2 = uVar2 + 1;
      } while (uVar2 < uVar1);
    }
    *(undefined1 *)(uVar2 + unaff_ESI) = 0;
    return unaff_ESI;
  }
  return 0;
}
#endif
