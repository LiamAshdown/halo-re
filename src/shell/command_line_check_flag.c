// command_line_check_flag  (Ghidra: command_line_check_flag, already named)
// address 0x542760, size 114 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: walks shell_argv/shell_argc looking for a '-'-prefixed token matching flag_name
// (_stricmp), optionally returning the following non-flag token through the EDI out-pointer.
// register convention: flag_name is the recognized stack parameter; the out-value pointer is
// unaff_EDI.
// blam-cc: EDI -> out_value (2nd parameter, may be NULL)
// Review fix: the result is returned in AL only (mov al,bl / mov al,1 at 0x5427a7 / 0x5427ce; the
//   upper EAX bytes hold stale _stricmp / argc values), so the return type is uint8_t.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern char **shell_argv; // 0x00721e90
extern int32_t shell_argc; // 0x00721e94
extern int32_t __stricmp(const char *a, const char *b);

// Looks up a named '-flag' in the parsed command line and reports whether it is present,
// optionally returning its following value argument through out_value.
uint8_t command_line_check_flag(const char *flag_name, const char **out_value)
{
    int32_t i;
    char *token;

    if (out_value != 0) {
        *out_value = 0;
    }

    for (i = 0; i < shell_argc; i++) {
        token = shell_argv[i];
        if (token[0] == '-' && __stricmp(flag_name, token) == 0) {
            if (out_value != 0 && i + 1 < shell_argc) {
                char *next_token = shell_argv[i + 1];
                if (next_token[0] != '-') {
                    *out_value = next_token;
                }
            }
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x542760):


undefined4 command_line_check_flag(char *param_1)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_EDI;
  
  if (unaff_EDI != (undefined4 *)0x0) {
    *unaff_EDI = 0;
  }
  iVar3 = 0;
  if (0 < DAT_00721e94) {
    do {
      pcVar1 = *(char **)(DAT_00721e90 + iVar3 * 4);
      if ((*pcVar1 == '-') && (iVar2 = __stricmp(param_1,pcVar1), iVar2 == 0)) {
        if ((unaff_EDI != (undefined4 *)0x0) &&
           ((iVar3 + 1 < DAT_00721e94 &&
            (pcVar1 = *(char **)(DAT_00721e90 + 4 + iVar3 * 4), *pcVar1 != '-')))) {
          *unaff_EDI = pcVar1;
        }
        return 1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < DAT_00721e94);
  }
  return 0;
}
#endif
