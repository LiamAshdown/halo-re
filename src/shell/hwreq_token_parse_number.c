// hwreq_token_parse_number  (Ghidra: hwreq_token_parse_number, already named)
// address 0x578b20, size 308 bytes
// name confidence: 0.75   rewrite confidence: 0.65
// evidence: "0x"-prefixed hex (up to k_hwreq_maximum_hex_digits=8 digits, else "Number too
// large") or plain decimal, else "Number expected"; calls hwreq_token_skip_whitespace and
// hwreq_token_parse_hex_digit, both in this module.
// register convention: parser in EAX.
// UNSURE: the hex digit decode is inlined a second time here (rather than reusing
// hwreq_token_parse_hex_digit) because the original does so too, likely because this loop also
// needs the peeked character even when it is not a valid digit (to know whether to stop);
// preserved as inlined logic identical to hwreq_token_parse_hex_digit's own tables.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"

extern void hwreq_token_skip_whitespace(hwreq_parser *parser); // 0x00578a00
extern void hwreq_parser_report_error(hwreq_parser *parser, const char *message); // 0x00578a20


// Parses a decimal or 0x-prefixed hexadecimal integer literal token, reporting a parser error
// if no valid number is present.
int32_t hwreq_token_parse_number(hwreq_parser *parser)
{
    char *cursor;
    char c;
    int32_t digit;
    int32_t next_digit;
    int32_t value;
    uint32_t digit_count;

    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
        parser->cursor++;
    }

    cursor = (char *)parser->cursor;

    if (*(uint16_t *)cursor == 0x7830 /* "0x" */) {
        parser->cursor = (uint32_t)(cursor + 2);
        digit = hwreq_token_parse_hex_digit(parser);
        if (digit != -1) {
            value = 0;
            digit_count = 0;
            for (;;) {
                if (digit_count > 7) {
                    hwreq_parser_report_error(parser, "Number too large");
                    return -1;
                }
                c = *(char *)parser->cursor;
                value = value * 0x10 + digit;
                digit_count++;
                next_digit = -1;
                if (c >= '0' && c <= '9') {
                    next_digit = c - 0x30;
                    parser->cursor++;
                } else if (c > '`' && c < 'g') {
                    next_digit = c - 0x57;
                    parser->cursor++;
                } else if (c > '@' && c < 'G') {
                    next_digit = c - 0x37;
                    parser->cursor++;
                }
                if (next_digit == -1) {
                    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
                        parser->cursor++;
                    }
                    return value;
                }
                digit = next_digit;
            }
        }
    } else if (*cursor > '/' && *cursor < ':' && (digit = hwreq_token_parse_hex_digit(parser), digit != -1)) {
        c = *(char *)parser->cursor;
        while (c > '/' && c < ':') {
            next_digit = hwreq_token_parse_hex_digit(parser);
            if (next_digit == -1) break;
            digit = next_digit + digit * 10;
            c = *(char *)parser->cursor;
        }
        hwreq_token_skip_whitespace(parser);
        return digit;
    }

    hwreq_parser_report_error(parser, "Number expected");
    return -1;
}

#if 0
Original Ghidra decompilation (0x578b20):


/* WARNING: Removing unreachable block (ram,0x00578b7e) */
/* WARNING: Removing unreachable block (ram,0x00578ba0) */

int hwreq_token_parse_number(void)

{
  char cVar1;
  short *psVar2;
  int in_EAX;
  int iVar3;
  int iVar4;
  uint uVar5;
  
  while( true ) {
    cVar1 = **(char **)(in_EAX + 8);
    if ((cVar1 != ' ') && (cVar1 != '\t')) break;
    *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
  }
  psVar2 = *(short **)(in_EAX + 8);
  if (*psVar2 == 0x7830) {
    *(short **)(in_EAX + 8) = psVar2 + 1;
    iVar3 = hwreq_token_parse_hex_digit();
    if (iVar3 != -1) {
      iVar4 = 0;
      uVar5 = 0;
      do {
        if (7 < uVar5) {
          hwreq_parser_report_error("Number too large");
          return -1;
        }
        cVar1 = **(char **)(in_EAX + 8);
        iVar4 = iVar4 * 0x10 + iVar3;
        uVar5 = uVar5 + 1;
        iVar3 = -1;
        if ((cVar1 < '0') || ('9' < cVar1)) {
          if (('`' < cVar1) && (cVar1 < 'g')) {
            iVar3 = cVar1 + -0x57;
            goto LAB_00578c1d;
          }
          if (('@' < cVar1) && (cVar1 < 'G')) {
            iVar3 = cVar1 + -0x37;
            goto LAB_00578c1d;
          }
        }
        else {
          iVar3 = cVar1 + -0x30;
LAB_00578c1d:
          *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
        }
        if (iVar3 == -1) {
          while( true ) {
            cVar1 = **(char **)(in_EAX + 8);
            if ((cVar1 != ' ') && (cVar1 != '\t')) break;
            *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
          }
          return iVar4;
        }
      } while( true );
    }
  }
  else if ((('/' < (char)*psVar2) && ((char)*psVar2 < ':')) &&
          (iVar3 = hwreq_token_parse_hex_digit(), iVar3 != -1)) {
    cVar1 = **(char **)(in_EAX + 8);
    while ((('/' < cVar1 && (**(char **)(in_EAX + 8) < ':')) &&
           (iVar4 = hwreq_token_parse_hex_digit(), iVar4 != -1))) {
      iVar3 = iVar4 + iVar3 * 10;
      cVar1 = **(char **)(in_EAX + 8);
    }
    hwreq_token_skip_whitespace();
    return iVar3;
  }
  hwreq_parser_report_error("Number expected");
  return -1;
}
#endif
