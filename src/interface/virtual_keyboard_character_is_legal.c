// virtual_keyboard_character_is_legal  (Ghidra: FUN_004a8b80, renamed)
// address 0x4a8b80, size 87 bytes
// name confidence: 0.4 (chosen)   rewrite confidence: 0.85
// evidence: phase-4 summary "Returns whether a typed character (CL) is legal for the current
// virtual-keyboard field type (EAX): digits only, hostname/IP-like charset, or a general
// blacklist check"; types/interface.h virtual_keyboard_globals::field_kind (tested here as the
// EAX field-type value) and the character filter note "checked by the character filter at
// 0x4a8b80".
// UNSURE: field kinds 3/4/5 are not otherwise named in this pass; kept as raw literals with the
// summary's own reading (3 = general/blacklist fallthrough alongside default, 4 =
// hostname/IP-like, 5 = digits only).
// Phase-4 s2 review: EAX is virtual_keyboard.validation_mode (the dword at 0x00719410 that
// virtual_keyboard_process_input loads right before the call), not field_kind; the return is a
// byte (mode 5 returns AL of isdigit). The blacklist is the string at 0x0066a8c8 through the
// pointer at 0x00692d78, tested with strchr (0x6257e0).
// register convention: validation mode in EAX (in_EAX), character in CL (in_CL), both unresolved
// register reads.
//   // blam-cc: validation_mode -> EAX, character -> CL

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *virtual_keyboard_blacklist_charset; // 0x00692d78, UNSURE: passed to strchr

// blam-cc: validation_mode -> EAX, character -> CL
// Filters a typed character against the current virtual-keyboard field's allowed character set:
// field kind 5 is digits only; field kind 4 allows '.', '-' and ':' plus alphanumerics
// (hostname/IP-like); every other kind (including 3) falls through to a general blacklist
// membership test.
uint8_t virtual_keyboard_character_is_legal(int32_t validation_mode, uint8_t character)
{
    if (validation_mode != 3) {
        if (validation_mode != 4) {
            if (validation_mode != 5) {
                const char *blocked = strchr((const char *)virtual_keyboard_blacklist_charset, character);
                return (uint8_t)(1 - (blocked != 0));
            }
            return (uint8_t)isdigit(character); // the caller tests AL of the CRT result
        }
        if (character != '.' && character != '-' && character != ':') {
            if (!isalnum(character)) {
                return 0;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a8b80):

uint FUN_004a8b80(void)

{
  int in_EAX;
  int iVar1;
  uint uVar2;
  byte in_CL;

  if (in_EAX != 3) {
    if (in_EAX != 4) {
      if (in_EAX != 5) {
        iVar1 = FUN_006257e0(PTR_DAT_00692d78,in_CL);
        return (uint)(byte)(1 - (iVar1 != 0));
      }
      uVar2 = _isdigit((uint)in_CL);
      return uVar2;
    }
    if (((in_CL != 0x2e) && (in_CL != 0x2d)) && (in_CL != 0x3a)) {
      iVar1 = _isalnum((uint)in_CL);
      if (iVar1 == 0) {
        return 0;
      }
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
