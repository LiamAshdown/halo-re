// network_address_string_normalize  (Ghidra: network_address_string_normalize, already named)
// address 0x4dc5e0, size 327 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Parses a dotted-decimal IP address string (with
// an optional trailing port) and writes back a canonical 'a.b.c.d' or 'a.b.c.d:port' string,
// also reporting whether the address is the wildcard 0.0.0.0." Ghidra fully recovered the cdecl
// (address_string, out_buffer, out_is_any) signature and control flow.
// UNSURE: the sscanf format string additionally captures a 5th numeric field
// ("%d.%d.%d.%d.%d") that is never read back (local_4); this looks like it exists only so a
// dot-delimited 5th field does not make the match count exceed 4, and is preserved verbatim.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>

extern char network_address_parse_port(char *address_string, int32_t *port_out); // 0x4dc560, this module

// Parses the leading "%d.%d.%d.%d" of address_string (tolerating one extra dot-delimited field
// it never uses), range-checks each byte to 0..255, reports via out_is_any whether all four are
// zero, then calls network_address_parse_port on the ORIGINAL string to look for a ':port'
// suffix; formats out_buffer as "a.b.c.d:port" when a port was found, else "a.b.c.d".
char network_address_string_normalize(char *address_string, char *out_buffer, uint8_t *out_is_any)
{
    char result;
    int32_t a, b, c, d, e;
    int32_t matched;
    uint8_t is_any;
    int32_t port;

    result = 0;
    port = -1;
    matched = sscanf(address_string, "%d.%d.%d.%d.%d", &a, &b, &c, &d, &e);
    if (matched == 4 && -1 < a && a < 0x100 && -1 < b && b < 0x100 &&
        -1 < c && c < 0x100 && -1 < d && d < 0x100) {
        is_any = 0;
        if (a == 0 && b == 0 && c == 0 && d == 0) {
            is_any = 1;
        }
        if (out_is_any != 0) {
            *out_is_any = is_any;
        }
        result = network_address_parse_port(address_string, &port);
        if (result == 1) {
            if (port != -1) {
                snprintf(out_buffer, 0x19, "%d.%d.%d.%d:%d", a, b, c, d, port);
                return 1;
            }
            snprintf(out_buffer, 0x19, "%d.%d.%d.%d", a, b, c, d);
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4dc5e0):

char __cdecl
network_address_string_normalize(char *address_string,char *out_buffer,uchar *out_is_any)

{
  uchar uVar1;
  char cVar2;
  int iVar3;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  long local_8;
  undefined1 local_4 [4];

  cVar2 = '\0';
  local_8 = -1;
  iVar3 = _sscanf(address_string,"%d.%d.%d.%d.%d",&local_c,&local_10,&local_14,&local_18,local_4);
  if (((((iVar3 == 4) && (-1 < local_c)) && (local_c < 0x100)) &&
      (((-1 < local_10 && (local_10 < 0x100)) &&
       ((-1 < local_14 && ((local_14 < 0x100 && (-1 < local_18)))))))) && (local_18 < 0x100)) {
    uVar1 = '\0';
    if ((((local_c == 0) && (local_10 == 0)) && (local_14 == 0)) && (local_18 == 0)) {
      uVar1 = '\x01';
    }
    if (out_is_any != (uchar *)0x0) {
      *out_is_any = uVar1;
    }
    cVar2 = network_address_parse_port(&local_8);
    if (cVar2 == '\x01') {
      if (local_8 != -1) {
        __snprintf(out_buffer,0x19,"%d.%d.%d.%d:%d",local_c,local_10,local_14,local_18,local_8);
        return '\x01';
      }
      __snprintf(out_buffer,0x19,"%d.%d.%d.%d",local_c,local_10,local_14,local_18);
    }
  }
  return cVar2;
}
#endif
