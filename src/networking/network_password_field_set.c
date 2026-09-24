// network_password_field_set  (Ghidra: FUN_004df070; named per this rewrite)
// address 0x4df070, size 27 bytes
// name confidence: 0.25   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Register-based helper that copies a
// wide-character string (e.g. a password) into the object at unaff_ESI+8 and clears the field
// immediately following it." Neither the object nor the exact fields at +8/+0x86 could be tied
// to a specific header-declared struct (network_server_globals.password is 9 wide chars at
// +0x9fc, not +8, and the 0x3f-char copy count does not match its size); kept as raw offsets on
// a generic object pointer.
// register convention: object in ESI (unaff_ESI), source string in EAX (in_EAX). blam-cc:
// EAX -> source, ESI -> object

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

// blam-cc: EAX -> source, ESI -> object
void network_password_field_set(uint8_t *object, wchar_t *source)
{
    wcsncpy((wchar_t *)(object + 8), source, 0x3f);
    *(uint16_t *)(object + 0x86) = 0;
}

#if 0
Original Ghidra decompilation (0x4df070):

uint FUN_004df070(void)

{
  wchar_t *in_EAX;
  wchar_t *pwVar1;
  int unaff_ESI;

  pwVar1 = _wcsncpy((wchar_t *)(unaff_ESI + 8),in_EAX,0x3f);
  *(undefined2 *)(unaff_ESI + 0x86) = 0;
  return (uint)pwVar1 & 0xffffff00;
}
#endif
