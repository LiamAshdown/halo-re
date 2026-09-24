// hwreq_token_skip_whitespace  (Ghidra: hwreq_token_skip_whitespace, already named)
// address 0x578a00, size 20 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: advances parser->cursor past spaces and tabs.
// register convention: parser in EDX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// Tokenizer helper that skips spaces and tabs at the parser's current cursor position.
void hwreq_token_skip_whitespace(hwreq_parser *parser)
{
    char *cursor = (char *)parser->cursor;
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
    parser->cursor = (uint32_t)cursor;
}

#if 0
Original Ghidra decompilation (0x578a00):


void hwreq_token_skip_whitespace(void)

{
  char cVar1;
  int in_EDX;
  
  while( true ) {
    cVar1 = **(char **)(in_EDX + 8);
    if ((cVar1 != ' ') && (cVar1 != '\t')) break;
    *(char **)(in_EDX + 8) = *(char **)(in_EDX + 8) + 1;
  }
  return;
}
#endif
