// network_address_string_is_valid  (Ghidra: FUN_004dc730; named per this rewrite)
// address 0x4dc730, size 91 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Validates that a user-supplied string looks
// like a legal IP address or hostname, optionally with a port suffix." Matches the code: if the
// string is non-empty and does NOT already parse as a normalized dotted address, every character
// must be '.', '-', ':' or alphanumeric.
// register convention: address string in EAX (in_EAX). blam-cc: EAX -> address_string

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <ctype.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char network_address_string_normalize(char *address_string, char *out_buffer, uint8_t *out_is_any); // 0x4dc5e0, this module

// VERIFIED against disassembly 0x4dc730..0x4dc78a (2026-09-30): every branch and the return value match.
// blam-cc: EAX -> address_string
char network_address_string_is_valid(char *address_string)
{
    char scratch[28];
    char normalize_result;
    char valid;
    char c;

    normalize_result = 0;
    if (*address_string != '\0') {
        normalize_result = network_address_string_normalize(address_string, scratch, 0);
        if (normalize_result == 0) {
            valid = 1;
            while (1) {
                c = *address_string;
                if (c == '\0') {
                    return valid;
                }
                if (c == '.' || c == '-' || c == ':' || isalnum((uint8_t)c)) {
                    valid = 1;
                } else {
                    valid = 0;
                }
                address_string = address_string + 1;
                if (valid == 0) {
                    break;
                }
            }
            return valid;
        }
    }
    return normalize_result;
}

#if 0
Original Ghidra decompilation (0x4dc730):

char FUN_004dc730(void)

{
  char cVar1;
  char cVar2;
  char *in_EAX;
  int iVar3;
  char local_1c [28];

  cVar1 = '\0';
  if ((*in_EAX != '\0') &&
     (cVar1 = network_address_string_normalize(in_EAX,local_1c,(uchar *)0x0), cVar1 == '\0')) {
    cVar2 = '\x01';
    do {
      cVar1 = *in_EAX;
      if (cVar1 == '\0') {
        return cVar2;
      }
      if ((((cVar1 == '.') || (cVar1 == '-')) || (cVar1 == ':')) ||
         (iVar3 = _isalnum((int)cVar1), iVar3 != 0)) {
        cVar2 = '\x01';
      }
      else {
        cVar2 = '\0';
      }
      in_EAX = in_EAX + 1;
      cVar1 = '\0';
    } while (cVar2 != '\0');
  }
  return cVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
