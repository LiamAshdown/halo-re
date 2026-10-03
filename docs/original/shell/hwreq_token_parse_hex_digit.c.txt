// hwreq_token_parse_hex_digit  (Ghidra: hwreq_token_parse_hex_digit, already named)
// address 0x578ad0, size 71 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: classic '0'-'9'/'a'-'f'/'A'-'F' digit decode, advancing the cursor only on a match.
// register convention: parser in ESI.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// Parses a single hexadecimal digit at the cursor, advancing past it and returning its value,
// or -1 if the character is not a hex digit.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t hwreq_token_parse_hex_digit(hwreq_parser *parser)
{
    char *cursor = (char *)parser->cursor;
    char c = *cursor;

    if (c > '/' && c < ':') {
        parser->cursor = (uint32_t)(cursor + 1);
        return c - 0x30;
    }
    if (c > '`' && c < 'g') {
        parser->cursor = (uint32_t)(cursor + 1);
        return c - 0x57;
    }
    if (c > '@' && c < 'G') {
        parser->cursor = (uint32_t)(cursor + 1);
        return c - 0x37;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x578ad0):


int hwreq_token_parse_hex_digit(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  int unaff_ESI;
  
  pcVar2 = *(char **)(unaff_ESI + 8);
  cVar1 = *pcVar2;
  iVar3 = -1;
  if (('/' < cVar1) && (cVar1 < ':')) {
    *(char **)(unaff_ESI + 8) = pcVar2 + 1;
    return cVar1 + -0x30;
  }
  if (('`' < cVar1) && (cVar1 < 'g')) {
    *(char **)(unaff_ESI + 8) = pcVar2 + 1;
    return cVar1 + -0x57;
  }
  if (('@' < cVar1) && (cVar1 < 'G')) {
    iVar3 = cVar1 + -0x37;
    *(char **)(unaff_ESI + 8) = pcVar2 + 1;
  }
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
