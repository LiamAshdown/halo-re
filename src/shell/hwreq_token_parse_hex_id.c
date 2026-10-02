// hwreq_token_parse_hex_id  (Ghidra: hwreq_token_parse_hex_id, already named)
// address 0x578ef0, size 129 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: parses exactly 4 hex digits (the first inline, the rest via
// hwreq_token_parse_hex_digit) into a 16-bit value, e.g. a vendor/device id.
// register convention: parser in EAX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t hwreq_token_parse_hex_digit(hwreq_parser *parser); // 0x00578ad0

// Parses a 4-hex-digit token (e.g. a vendor or device ID) into a 16-bit value, returning -1 on
// any invalid digit.
uint32_t hwreq_token_parse_hex_id(hwreq_parser *parser)
{
    char c;
    int32_t d0, d1, d2, d3;

    c = *(char *)parser->cursor;
    if (c >= '0' && c <= '9') {
        d0 = c - 0x30;
    } else if (c >= 'a' && c <= 'f') {
        d0 = c - 0x57;
    } else if (c >= 'A' && c <= 'F') {
        d0 = c - 0x37;
    } else {
        return 0xffffffff;
    }
    parser->cursor++;
    if (d0 == -1) {
        return 0xffffffff;
    }

    d1 = hwreq_token_parse_hex_digit(parser);
    if (d1 == -1) return 0xffffffff;
    d2 = hwreq_token_parse_hex_digit(parser);
    if (d2 == -1) return 0xffffffff;
    d3 = hwreq_token_parse_hex_digit(parser);
    if (d3 == -1) return 0xffffffff;

    return (uint32_t)(d0 << 0xc | d1 << 8 | d2 << 4 | d3);
}

#if 0
Original Ghidra decompilation (0x578ef0):


uint hwreq_token_parse_hex_id(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  
  cVar1 = **(char **)(in_EAX + 8);
  if ((cVar1 < '0') || ('9' < cVar1)) {
    if ((cVar1 < 'a') || ('f' < cVar1)) {
      if (cVar1 < 'A') {
        return 0xffffffff;
      }
      if ('F' < cVar1) {
        return 0xffffffff;
      }
      iVar2 = cVar1 + -0x37;
    }
    else {
      iVar2 = cVar1 + -0x57;
    }
  }
  else {
    iVar2 = cVar1 + -0x30;
  }
  *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
  if (iVar2 == -1) {
    return 0xffffffff;
  }
  iVar3 = hwreq_token_parse_hex_digit();
  if (iVar3 != -1) {
    iVar4 = hwreq_token_parse_hex_digit();
    if (iVar4 != -1) {
      uVar5 = hwreq_token_parse_hex_digit();
      if (uVar5 != 0xffffffff) {
        return iVar2 << 0xc | iVar3 << 8 | iVar4 << 4 | uVar5;
      }
    }
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
