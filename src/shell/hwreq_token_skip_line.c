// hwreq_token_skip_line  (Ghidra: hwreq_token_skip_line, already named)
// address 0x5789d0, size 43 bytes
// name confidence: 0.65   rewrite confidence: 0.85
// evidence: advances parser->cursor past the next CR (and a following LF), then resets
// line_start and increments line_number, matching hwreq_parser's field layout exactly.
// register convention: parser in EAX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Tokenizer helper that advances the parser's cursor to the start of the next line, updating
// the cached line-start pointer and line-number counter.
void hwreq_token_skip_line(hwreq_parser *parser)
{
    char *p;
    char *cursor = (char *)parser->cursor;
    char *end = (char *)parser->end;

    do {
        p = cursor;
        cursor = p + 1;
        if (*p == '\r') break;
    } while (cursor < end);

    if (cursor < end && *cursor == '\n') {
        cursor = p + 2;
    }

    parser->cursor = (uint32_t)cursor;
    parser->line_start = (uint32_t)cursor;
    parser->line_number = parser->line_number + 1;
}

#if 0
Original Ghidra decompilation (0x5789d0):


void hwreq_token_skip_line(void)

{
  char *pcVar1;
  int in_EAX;
  char *pcVar2;
  
  do {
    pcVar1 = *(char **)(in_EAX + 8);
    pcVar2 = pcVar1 + 1;
    *(char **)(in_EAX + 8) = pcVar2;
    if (*pcVar1 == '\r') break;
  } while (pcVar2 < *(char **)(in_EAX + 0xc));
  if ((pcVar2 < *(char **)(in_EAX + 0xc)) && (*pcVar2 == '\n')) {
    *(char **)(in_EAX + 8) = pcVar1 + 2;
  }
  *(undefined4 *)(in_EAX + 0x10) = *(undefined4 *)(in_EAX + 8);
  *(int *)(in_EAX + 0x14) = *(int *)(in_EAX + 0x14) + 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
