// hwreq_parser_scan_for_applytoall  (Ghidra: FUN_0057a320; renamed, still unclaimed by any prior
//   pass)
// address 0x57a320, size 192 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: out/phase4/shell_types_notes.md: "Scans forward line by line through the
//   requirements script looking for an 'applytoall' directive, parsing the matching block when
//   found." Same line-skipping and applytoall-block-parsing shape as
//   hwreq_parser_scan_for_applytoall_or_vendor 0x57a220, but with no "vendor" stop condition --
//   it always runs to the end of the file.
// register convention: this in ESI (objdump: every field access is [esi+n] with no load at
//   entry; same calling shape as 0x57a220's sibling).
// blam-cc: this in ESI (only parameter).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern uint8_t hwreq_parser_parse_block(hwreq_parser *this, hwreq_property_set *target); // 0x57af10

// Scans forward from the parser's current cursor to the end of the file, parsing every
// "applytoall { ... }" block it finds into this->flags. Returns false only if a matched
// applytoall block fails to parse; returns true once the cursor reaches the end of the file.
uint8_t hwreq_parser_scan_for_applytoall(hwreq_parser *this)
{
    char *line;
    char c;
    uint8_t result;

    for (;;) {
        if (_strnicmp((char *)this->cursor, "applytoall", 10) == 0) {
            c = ((char *)this->cursor)[10];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                // consume the "applytoall" line
                do {
                    line = (char *)this->cursor;
                    this->cursor = (uint32_t)(line + 1);
                    if (*line == '\r') break;
                } while ((char *)this->cursor < (char *)this->end);
                if ((char *)this->cursor < (char *)this->end && *(char *)this->cursor == '\n') {
                    this->cursor = (uint32_t)(line + 2);
                }
                this->line_start = this->cursor;
                this->line_number = this->line_number + 1;

                result = hwreq_parser_parse_block(this, (hwreq_property_set *)this->flags);
                if (result == 0) {
                    return result;
                }
                // fall through: skip the rest of the block's closing line and keep scanning
            }
        }

        do {
            line = (char *)this->cursor;
            this->cursor = (uint32_t)(line + 1);
            if (*line == '\r') break;
        } while ((char *)this->cursor < (char *)this->end);
        if ((char *)this->cursor < (char *)this->end && *(char *)this->cursor == '\n') {
            this->cursor = (uint32_t)(line + 2);
        }
        line = (char *)this->cursor;
        this->line_start = this->cursor;
        this->line_number = this->line_number + 1;
        if ((char *)this->end <= line) {
            return 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x57a320):

uint FUN_0057a320(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  uint uVar4;
  char *pcVar5;
  int unaff_ESI;

  do {
    iVar3 = __strnicmp(*(char **)(unaff_ESI + 8),"applytoall",10);
    if ((iVar3 == 0) &&
       (((((cVar1 = *(char *)(*(int *)(unaff_ESI + 8) + 10), cVar1 == '>' || (cVar1 == '<')) ||
          (cVar1 == '!')) || ((cVar1 == '=' || (cVar1 == ' ')))) ||
        ((cVar1 == '\r' || (cVar1 == '\t')))))) {
      do {
        pcVar2 = *(char **)(unaff_ESI + 8);
        pcVar5 = pcVar2 + 1;
        *(char **)(unaff_ESI + 8) = pcVar5;
        if (*pcVar2 == '\r') break;
      } while (pcVar5 < *(char **)(unaff_ESI + 0xc));
      if ((pcVar5 < *(char **)(unaff_ESI + 0xc)) && (*pcVar5 == '\n')) {
        *(char **)(unaff_ESI + 8) = pcVar2 + 2;
      }
      *(undefined4 *)(unaff_ESI + 0x10) = *(undefined4 *)(unaff_ESI + 8);
      *(int *)(unaff_ESI + 0x14) = *(int *)(unaff_ESI + 0x14) + 1;
      uVar4 = hwreq_parser_parse_block(unaff_ESI,*(undefined4 *)(unaff_ESI + 0x18));
      if ((char)uVar4 == '\0') {
        return uVar4 & 0xffffff00;
      }
    }
    do {
      pcVar2 = *(char **)(unaff_ESI + 8);
      pcVar5 = pcVar2 + 1;
      *(char **)(unaff_ESI + 8) = pcVar5;
      if (*pcVar2 == '\r') break;
    } while (pcVar5 < *(char **)(unaff_ESI + 0xc));
    if ((pcVar5 < *(char **)(unaff_ESI + 0xc)) && (*pcVar5 == '\n')) {
      *(char **)(unaff_ESI + 8) = pcVar2 + 2;
    }
    pcVar2 = *(char **)(unaff_ESI + 8);
    *(char **)(unaff_ESI + 0x10) = pcVar2;
    *(int *)(unaff_ESI + 0x14) = *(int *)(unaff_ESI + 0x14) + 1;
    if (*(char **)(unaff_ESI + 0xc) <= pcVar2) {
      return CONCAT31((int3)((uint)pcVar2 >> 8),1);
    }
  } while( true );
}
#endif
