// hwreq_parser_report_error  (Ghidra: hwreq_parser_report_error, already named)
// address 0x578a20, size 161 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: builds "%s on line %d - '%s'" from the message, line_number and up to
// k_hwreq_error_context_length characters of the current line, latches it once into
// error_message and error_reported (vtable slots 0x38/0x3c), matching hwreq_parser's layout.
// register convention: parser in ESI, message as the stack parameter.
// blam-cc: ESI -> parser (1st parameter), stack -> message (2nd parameter)
// Review fixes (objdump 0x578a20..0x578ac0): the message is assigned with std::string::assign
//   0x57bc90 (lea ecx,[esi+0x24] at 0x578aaf), which keeps or grows the existing buffer; the first
//   rewrite re-initialised the string through a private helper. The "..." is also appended when
//   the line is exactly 36 characters long, not only when it is longer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t _snprintf(char *buffer, uint32_t size, const char *format, ...); // 0x623a2d CRT
extern void msvc_string_assign_n(msvc_std_string *dest, const char *source, uint32_t length); // 0x57bc90, blam-cc: dest in ECX, source/length on the stack; library code, not in the function list

// Records a parser error message together with the current line number and a snippet of the
// offending line, for later reporting; ignored if an error was already latched on this line.
void hwreq_parser_report_error(hwreq_parser *parser, const char *message)
{
    char context[k_hwreq_error_context_length + 4];
    char formatted[0x100];
    const char *line;
    int32_t n;
    int32_t length;

    if (parser->error_reported != 0) {
        return;
    }

    line = (const char *)parser->line_start;
    n = 0;
    if (line[0] != '\r') {
        do {
            if (n == k_hwreq_error_context_length) {
                break;
            }
            context[n] = line[n];
            n++;
        } while (line[n] != '\r');
        if (n == k_hwreq_error_context_length) {
            // also when the line is exactly 36 characters long (0x578a57..0x578a5b)
            context[n] = '.';
            context[n + 1] = '.';
            context[n + 2] = '.';
            n += 3;
        }
    }
    context[n] = '\0';

    _snprintf(formatted, 0x100, "%s on line %d - '%s'", message, parser->line_number, context);

    length = 0;
    while (formatted[length] != '\0') length++;
    msvc_string_assign_n(&parser->error_message, formatted, (uint32_t)length);
    parser->error_reported = 1;
}

#if 0
Original Ghidra decompilation (0x578a20):


void hwreq_parser_report_error(undefined4 param_1)

{
  char *pcVar1;
  char *pcVar2;
  char cVar3;
  int unaff_ESI;
  char local_128 [40];
  char local_100 [256];
  
  pcVar2 = local_128;
  if (*(char *)(unaff_ESI + 0x20) == '\0') {
    pcVar1 = *(char **)(unaff_ESI + 0x10);
    cVar3 = *pcVar1;
    if (cVar3 != '\r') {
      pcVar2 = local_128;
      do {
        if (pcVar2 == local_128 + 0x24) goto LAB_00578a5d;
        *pcVar2 = cVar3;
        cVar3 = pcVar2[(int)(pcVar1 + (1 - (int)local_128))];
        pcVar2 = pcVar2 + 1;
      } while (cVar3 != '\r');
      if (pcVar2 == local_128 + 0x24) {
LAB_00578a5d:
        *pcVar2 = '.';
        pcVar2[1] = '.';
        pcVar2[2] = '.';
        pcVar2 = pcVar2 + 3;
      }
    }
    *pcVar2 = '\0';
    __snprintf(local_100,0x100,"%s on line %d - \'%s\'",param_1,*(undefined4 *)(unaff_ESI + 0x14),
               local_128);
    pcVar2 = local_100;
    do {
      cVar3 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar3 != '\0');
    FUN_0057bc90(local_100,(int)pcVar2 - (int)(local_100 + 1));
    *(undefined1 *)(unaff_ESI + 0x20) = 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
