// hwreq_parser_find_requirements_section  (Ghidra: hwreq_parser_find_requirements_section,
//   already named)
// address 0x57ae50, size 187 bytes
// name confidence: 0.55  rewrite confidence: 0.8
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Locates the
//   top-level 'Requirements' section in the hardware compatibility script and begins parsing
//   it." Scans line by line for a line starting "Requirements" followed by a delimiter, skips
//   past that line, and parses everything after it as a block into this->requirements (+0x1c)
//   via hwreq_parser_parse_block.
// register convention: this in ESI (Ghidra's unaff_ESI; every field access is [esi+n] with no
//   load at entry).
// blam-cc: this in ESI (only parameter).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern uint8_t hwreq_parser_parse_block(hwreq_parser *self, hwreq_property_set *target); // 0x57af10

// Scans forward from the parser's cursor for a line beginning "Requirements" followed by a
// delimiter; once found, skips that line and parses the remainder of the file as a block into
// this->requirements. Returns true (with nothing parsed) if the end of the file is reached
// first in either scan.
uint8_t hwreq_parser_find_requirements_section(hwreq_parser *self)
{
    char *line;
    char c;

    for (;;) {
        if (_strnicmp((char *)self->cursor, "Requirements", 12) == 0) {
            c = ((char *)self->cursor)[12];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                break;
            }
        }
        do {
            line = (char *)self->cursor;
            self->cursor = (uint32_t)(line + 1);
            if (*line == '\r') break;
        } while ((char *)self->cursor < (char *)self->end);
        if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
            self->cursor = (uint32_t)(line + 2);
        }
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if (!((char *)self->cursor < (char *)self->end)) {
            break;
        }
    }

    if ((char *)self->end <= (char *)self->cursor) {
        return 1;
    }

    do {
        line = (char *)self->cursor;
        self->cursor = (uint32_t)(line + 1);
        if (*line == '\r') break;
    } while ((char *)self->cursor < (char *)self->end);
    if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
        self->cursor = (uint32_t)(line + 2);
    }
    self->line_number = self->line_number + 1;
    self->line_start = self->cursor;

    return hwreq_parser_parse_block(self, (hwreq_property_set *)self->requirements) != 0;
}

#if 0
Original Ghidra decompilation (0x57ae50):

bool hwreq_parser_find_requirements_section(void)

{
  char *pcVar1;
  char cVar2;
  int iVar3;
  char *pcVar4;
  char *pcVar5;
  int unaff_ESI;

  do {
    iVar3 = __strnicmp(*(char **)(unaff_ESI + 8),"Requirements",0xc);
    if (iVar3 == 0) {
      cVar2 = *(char *)(*(int *)(unaff_ESI + 8) + 0xc);
      if (((((cVar2 == '>') || (cVar2 == '<')) || (cVar2 == '!')) ||
          ((cVar2 == '=' || (cVar2 == ' ')))) || ((cVar2 == '\r' || (cVar2 == '\t')))) break;
    }
    do {
      pcVar1 = *(char **)(unaff_ESI + 8);
      pcVar5 = pcVar1 + 1;
      *(char **)(unaff_ESI + 8) = pcVar5;
      if (*pcVar1 == '\r') break;
    } while (pcVar5 < *(char **)(unaff_ESI + 0xc));
    if ((pcVar5 < *(char **)(unaff_ESI + 0xc)) && (*pcVar5 == '\n')) {
      *(char **)(unaff_ESI + 8) = pcVar1 + 2;
    }
    *(char **)(unaff_ESI + 0x10) = *(char **)(unaff_ESI + 8);
    *(int *)(unaff_ESI + 0x14) = *(int *)(unaff_ESI + 0x14) + 1;
  } while (*(char **)(unaff_ESI + 8) < *(char **)(unaff_ESI + 0xc));
  pcVar1 = *(char **)(unaff_ESI + 0xc);
  if (pcVar1 <= *(char **)(unaff_ESI + 8)) {
    return true;
  }
  do {
    pcVar5 = *(char **)(unaff_ESI + 8);
    pcVar4 = pcVar5 + 1;
    *(char **)(unaff_ESI + 8) = pcVar4;
    if (*pcVar5 == '\r') break;
  } while (pcVar4 < pcVar1);
  if ((pcVar4 < pcVar1) && (*pcVar4 == '\n')) {
    *(char **)(unaff_ESI + 8) = pcVar5 + 2;
  }
  *(int *)(unaff_ESI + 0x14) = *(int *)(unaff_ESI + 0x14) + 1;
  *(undefined4 *)(unaff_ESI + 0x10) = *(undefined4 *)(unaff_ESI + 8);
  cVar2 = hwreq_parser_parse_block(unaff_ESI,*(undefined4 *)(unaff_ESI + 0x1c));
  return cVar2 != '\0';
}
#endif
