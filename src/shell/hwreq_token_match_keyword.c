// hwreq_token_match_keyword  (Ghidra: hwreq_token_match_keyword, already named)
// address 0x578fa0, size 80 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: case-insensitive prefix compare of the cursor against keyword, requiring the
// following character to be a valid delimiter (>, <, !, =, space, CR, tab); does not consume the
// token (no cursor write).
// register convention: keyword in EDX, parser in EDI.
// blam-cc: EDX -> keyword (1st parameter), EDI -> parser (2nd parameter)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"


// Tests whether the parser's cursor is currently positioned at the given keyword, followed by a
// valid delimiter character, without consuming the token.
uint32_t hwreq_token_match_keyword(const char *keyword, hwreq_parser *parser)
{
    uint32_t length;
    const char *p;
    char delimiter;

    p = keyword;
    while (*p != '\0') p++;
    length = (uint32_t)(p - keyword);

    if (_strnicmp((char *)parser->cursor, keyword, length) == 0) {
        delimiter = ((char *)parser->cursor)[length];
        if (delimiter == '>' || delimiter == '<' || delimiter == '!' || delimiter == '=' ||
            delimiter == ' ' || delimiter == '\r' || delimiter == '\t') {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x578fa0):


undefined4 hwreq_token_match_keyword(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  char *in_EDX;
  int unaff_EDI;
  
  pcVar2 = in_EDX;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  iVar3 = __strnicmp(*(char **)(unaff_EDI + 8),in_EDX,(int)pcVar2 - (int)(in_EDX + 1));
  if ((iVar3 == 0) &&
     ((((cVar1 = *(char *)(((int)pcVar2 - (int)(in_EDX + 1)) + *(int *)(unaff_EDI + 8)),
        cVar1 == '>' || (cVar1 == '<')) || (cVar1 == '!')) ||
      (((cVar1 == '=' || (cVar1 == ' ')) || ((cVar1 == '\r' || (cVar1 == '\t')))))))) {
    return 1;
  }
  return 0;
}
#endif
