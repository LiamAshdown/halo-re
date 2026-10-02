// hwreq_parser_scan_for_applytoall_or_vendor  (Ghidra: FUN_0057a220; renamed, still unclaimed by
//   any prior pass)
// address 0x57a220, size 243 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: out/phase4/shell_types_notes.md: "Scans forward line by line through the
//   requirements script looking for an 'applytoall' or 'vendor' directive, parsing the matching
//   block when found." Matches the code: repeatedly parses "applytoall { ... }" blocks into
//   this->flags via hwreq_parser_parse_block, and stops (returning true, cursor left at the
//   start of the line) the moment a line begins with "vendor" followed by a delimiter.
// register convention: this in ESI (objdump: every field access is [esi+n] with no load at
//   entry, so ESI must already hold the parser on entry; confirmed live-in from the caller,
//   0x579bd0's parse method, which is not a Ghidra function).
// blam-cc: this in ESI (only parameter).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t hwreq_parser_parse_block(hwreq_parser *self, hwreq_property_set *target); // 0x57af10

// Scans forward from the parser's current cursor, parsing every "applytoall { ... }" block it
// finds into this->flags, until either a line beginning with "vendor" (followed by a delimiter)
// is reached -- left unconsumed, for the caller to handle -- or the end of the file is reached.
// Returns false only if a matched applytoall block fails to parse.
uint8_t hwreq_parser_scan_for_applytoall_or_vendor(hwreq_parser *self)
{
    char *line;
    char c;
    uint8_t result;

    for (;;) {
        if (_strnicmp((char *)self->cursor, "applytoall", 10) == 0) {
            c = ((char *)self->cursor)[10];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                // consume the "applytoall" line
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

                result = hwreq_parser_parse_block(self, (hwreq_property_set *)self->flags);
                if (result == 0) {
                    return result;
                }
                // fall through: skip the rest of the block's closing line and keep scanning
            }
        } else if (_strnicmp((char *)self->cursor, "vendor", 6) == 0) {
            c = ((char *)self->cursor)[6];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                break; // leave the cursor at the start of the "vendor" line
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
        line = (char *)self->cursor;
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if ((char *)self->end <= line) {
            return 1;
        }
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x57a220):

undefined4 FUN_0057a220(void)

{
  char cVar1;
  int iVar2;
  undefined4 uVar3;
  char *pcVar4;
  char *pcVar5;
  int unaff_ESI;

  do {
    iVar2 = __strnicmp(*(char **)(unaff_ESI + 8),"applytoall",10);
    if ((iVar2 == 0) &&
       (((((cVar1 = *(char *)(*(int *)(unaff_ESI + 8) + 10), cVar1 == '>' || (cVar1 == '<')) ||
          (cVar1 == '!')) || ((cVar1 == '=' || (cVar1 == ' ')))) ||
        ((cVar1 == '\r' || (cVar1 == '\t')))))) {
      do {
        pcVar4 = *(char **)(unaff_ESI + 8);
        pcVar5 = pcVar4 + 1;
        *(char **)(unaff_ESI + 8) = pcVar5;
        if (*pcVar4 == '\r') break;
      } while (pcVar5 < *(char **)(unaff_ESI + 0xc));
      if ((pcVar5 < *(char **)(unaff_ESI + 0xc)) && (*pcVar5 == '\n')) {
        *(char **)(unaff_ESI + 8) = pcVar4 + 2;
      }
      *(undefined4 *)(unaff_ESI + 0x10) = *(undefined4 *)(unaff_ESI + 8);
      *(int *)(unaff_ESI + 0x14) = *(int *)(unaff_ESI + 0x14) + 1;
      uVar3 = hwreq_parser_parse_block(unaff_ESI,*(undefined4 *)(unaff_ESI + 0x18));
      if ((char)uVar3 == '\0') {
        return uVar3;
      }
    }
    else {
      iVar2 = __strnicmp(*(char **)(unaff_ESI + 8),"vendor",6);
      if (iVar2 == 0) {
        cVar1 = *(char *)(*(int *)(unaff_ESI + 8) + 6);
        pcVar4 = (char *)0x0;
        if ((((cVar1 == '>') || (cVar1 == '<')) ||
            ((cVar1 == '!' || (((cVar1 == '=' || (cVar1 == ' ')) || (cVar1 == '\r')))))) ||
           (cVar1 == '\t')) break;
      }
    }
    do {
      pcVar4 = *(char **)(unaff_ESI + 8);
      pcVar5 = pcVar4 + 1;
      *(char **)(unaff_ESI + 8) = pcVar5;
      if (*pcVar4 == '\r') break;
    } while (pcVar5 < *(char **)(unaff_ESI + 0xc));
    if ((pcVar5 < *(char **)(unaff_ESI + 0xc)) && (*pcVar5 == '\n')) {
      *(char **)(unaff_ESI + 8) = pcVar4 + 2;
    }
    pcVar4 = *(char **)(unaff_ESI + 8);
    *(char **)(unaff_ESI + 0x10) = pcVar4;
    *(int *)(unaff_ESI + 0x14) = *(int *)(unaff_ESI + 0x14) + 1;
  } while (pcVar4 < *(char **)(unaff_ESI + 0xc));
  return CONCAT31((int3)((uint)pcVar4 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
