// string_is_numeric  (Ghidra: string_is_numeric, already named)
// address 0x4e3f30, size 56 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md ("Returns whether a string consists solely of
// digits and minus signs (or is empty)"); this batch's sv_find_client_by_name_or_index.c uses it
// exactly that way to disambiguate a numeric slot index from a player name.
// register convention: EAX -> string (matches sv_find_client_by_name_or_index.c's call site,
// which passes its own EAX-borne argument straight through with no setup).
//   // blam-cc: EAX -> string

#include "tags.h"
#include "memory.h"
#include <ctype.h>

// Returns 1 if `string` is NULL, empty, or consists solely of digits and/or minus signs;
// returns 0 as soon as any other character is found.
uint8_t string_is_numeric(char *string) // blam-cc: EAX -> string
{
    while (1) {
        if (string == 0 || *string == 0) {
            return 1;
        }
        if (!isdigit((uint8_t)*string) && *string != '-') {
            break;
        }
        string = string + 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e3f30), from tools/pack.py 0x4e3f30:

undefined4 string_is_numeric(void)

{
  char *in_EAX;
  int iVar1;

  while( true ) {
    if ((in_EAX == (char *)0x0) || (*in_EAX == '\0')) {
      return 1;
    }
    iVar1 = _isdigit((int)*in_EAX);
    if ((iVar1 == 0) && (*in_EAX != '-')) break;
    in_EAX = in_EAX + 1;
  }
  return 0;
}
#endif
