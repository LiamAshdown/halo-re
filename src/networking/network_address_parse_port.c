// network_address_parse_port  (Ghidra: network_address_parse_port, already named)
// address 0x4dc560, size 119 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Parses and range-checks a numeric port
// substring (after a delimiter) out of an address string, optionally writing the value through
// an output pointer." Ghidra fully recovered the cdecl `port_out` stack parameter and the
// control flow; only the address-string input is an elided register argument.
// register convention: address string in EAX (in_EAX, elided from Ghidra's own signature).
// blam-cc: EAX -> address_string, stack -> port_out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// blam-cc: EAX -> address_string
// Finds the first ':' in address_string; with none present, succeeds trivially (port_out
// untouched). Otherwise every character after the ':' must be a digit and atol() of that
// substring must fall in 1..0xffff; on success (with no ':' or with a valid in-range number)
// writes the parsed value through port_out when it is non-NULL.
char network_address_parse_port(char *address_string, int32_t *port_out)
{
    int all_digits;
    char *colon;
    char *cursor;
    int32_t port;
    char result;

    all_digits = 1;
    colon = strchr(address_string, ':');
    if (colon == 0) {
        return 1;
    }
    cursor = colon + 1;
    while (*cursor != '\0') {
        if (!isdigit((uint8_t)*cursor)) {
            all_digits = 0;
        }
        cursor = cursor + 1;
        if (!all_digits) {
            return 0;
        }
    }
    if (!all_digits) {
        return 0;
    }
    port = atol(colon + 1);
    result = (port < 1 || 0xffff < port) ? 0 : 1;
    if (port_out == 0) {
        return result;
    }
    *port_out = port;
    return result;
}

#if 0
Original Ghidra decompilation (0x4dc560):

char __cdecl network_address_parse_port(long *port_out)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  long lVar5;
  char *pcVar6;

  bVar1 = true;
  iVar3 = FUN_006257e0();
  if (iVar3 == 0) {
    return '\x01';
  }
  pcVar6 = (char *)(iVar3 + 1);
  while (*pcVar6 != '\0') {
    iVar4 = _isdigit((int)*pcVar6);
    if (iVar4 == 0) {
      bVar1 = false;
    }
    pcVar6 = pcVar6 + 1;
    if (!bVar1) {
      return '\0';
    }
  }
  if (!bVar1) {
    return '\0';
  }
  lVar5 = _atol((char *)(iVar3 + 1));
  if ((lVar5 < 1) || (0xffff < lVar5)) {
    cVar2 = '\0';
  }
  else {
    cVar2 = '\x01';
  }
  if (port_out == (long *)0x0) {
    return cVar2;
  }
  *port_out = lVar5;
  return cVar2;
}
#endif
