// hwreq_token_parse_quoted_string  (Ghidra: hwreq_token_parse_quoted_string, already named)
// address 0x578c60, size 139 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: copies a "..."-delimited token into the shared static buffer hwreq_quoted_string
// (0x00722d58, k_hwreq_quoted_string_length bytes), overflow-tested against 0x00722e57 (the last
// byte of that buffer).
// register convention: parser in EAX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char hwreq_quoted_string[k_hwreq_quoted_string_length]; // 0x00722d58

extern void hwreq_parser_report_error(hwreq_parser *parser, const char *message); // 0x00578a20

// Parses a double-quoted string literal token into a shared static buffer, reporting parser
// errors for a missing quote or an overlong string.
char *hwreq_token_parse_quoted_string(hwreq_parser *parser)
{
    char *cursor;
    char c;
    char *out;
    char *buffer_end;

    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
        parser->cursor++;
    }

    c = *(char *)parser->cursor;
    cursor = (char *)parser->cursor + 1;
    parser->cursor = (uint32_t)cursor;
    if (c != '"') {
        hwreq_parser_report_error(parser, "Expecting ");
        return 0;
    }

    c = *cursor;
    out = hwreq_quoted_string;
    buffer_end = hwreq_quoted_string + k_hwreq_quoted_string_length - 1; // 0x00722e57

    for (;;) {
        if (c == '"') {
            *out = '\0';
            cursor = (char *)parser->cursor + 1;
            parser->cursor = (uint32_t)cursor;
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
                parser->cursor = (uint32_t)cursor;
            }
            return hwreq_quoted_string;
        }
        *out = c;
        out++;
        cursor = (char *)parser->cursor + 1;
        parser->cursor = (uint32_t)cursor;
        if (out > buffer_end) break;
        c = *cursor;
    }

    hwreq_parser_report_error(parser, "String too long");
    return 0;
}

#if 0
Original Ghidra decompilation (0x578c60):


undefined1 * hwreq_token_parse_quoted_string(void)

{
  char cVar1;
  int in_EAX;
  char *pcVar2;
  char *pcVar3;
  
  while( true ) {
    cVar1 = **(char **)(in_EAX + 8);
    if ((cVar1 != ' ') && (cVar1 != '\t')) break;
    *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
  }
  cVar1 = **(char **)(in_EAX + 8);
  pcVar3 = *(char **)(in_EAX + 8) + 1;
  *(char **)(in_EAX + 8) = pcVar3;
  if (cVar1 != '\"') {
    hwreq_parser_report_error("Expecting ");
    return (undefined1 *)0x0;
  }
  cVar1 = *pcVar3;
  pcVar2 = &DAT_00722d58;
  while( true ) {
    if (cVar1 == '\"') {
      *pcVar2 = '\0';
      pcVar3 = (char *)(*(int *)(in_EAX + 8) + 1);
      *(char **)(in_EAX + 8) = pcVar3;
      while ((*pcVar3 == ' ' || (*pcVar3 == '\t'))) {
        pcVar3 = pcVar3 + 1;
        *(char **)(in_EAX + 8) = pcVar3;
      }
      return &DAT_00722d58;
    }
    *pcVar2 = *pcVar3;
    pcVar2 = pcVar2 + 1;
    pcVar3 = (char *)(*(int *)(in_EAX + 8) + 1);
    *(char **)(in_EAX + 8) = pcVar3;
    if (&DAT_00722e57 < pcVar2) break;
    cVar1 = *pcVar3;
  }
  hwreq_parser_report_error("String too long");
  return (undefined1 *)0x0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
