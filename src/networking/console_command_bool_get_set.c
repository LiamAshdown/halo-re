// console_command_bool_get_set  (Ghidra: console_command_bool_get_set, already named)
// address 0x4e2990, size 258 bytes
// name confidence: 0.8   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Shared console-command implementation that
// inspects or, given an argument, parses and sets a boolean value, then reports it -- used by
// boolean sv_* commands such as sv_single_flag_force_reset." Matches
// src/networking/sv_single_flag_force_reset.c's own call
// `console_command_bool_get_set(param_2,"sv_single_flag_force_reset")`. The literal "0"/"false"
// and "1"/"true" comparisons and the two report/usage message strings pin the behaviour exactly.
// register convention: EAX = argument_count, EBX = value (uint8_t *, implicit -- the boolean
// this call is getting or setting, set up by each individual sv_* caller before the call, e.g.
// sv_single_flag_force_reset.c's own `DAT_0071c306`), stack = arguments, name. Confirmed by
// sv_single_flag_force_reset.c calling this with only 2 of its stack arguments visible
// (`console_command_bool_get_set(param_2,"sv_single_flag_force_reset")`), consistent with EAX
// (argument_count) and EBX (value) arriving via register.
//   // blam-cc: EAX -> argument_count, EBX -> value, stack -> arguments, name
// UNSURE: `string_to_lowercase` and `string_trim_whitespace` are each called here with zero
// visible arguments; the natural operand (the local scratch buffer) is used for both, but
// string_trim_whitespace's own established signature takes a `char **` (an in/out pointer
// variable), which this call site has no such variable for -- modelled via a local `cursor`
// pointer initialized to the buffer, satisfying that signature, even though the subsequent
// comparisons (per Ghidra) read the buffer directly rather than through any trimmed/advanced
// cursor.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <string.h>

extern char * string_to_lowercase(char *string); // 0x4491e0, other module
extern void string_trim_whitespace(char **string_ptr); // 0x4e4040, this module
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// blam-cc: EAX -> argument_count, EBX -> value, stack -> arguments, name
// Shared get/set implementation for boolean sv_* console commands: with no argument, reports
// the current value; with one, accepts "0"/"false" or "1"/"true" (case-insensitive, trimmed)
// and stores it through `value`, then reports the new value the same way.
void console_command_bool_get_set(uint32_t argument_count, uint8_t *value, char **arguments, const char *name)
{
    char buffer[256];
    char *cursor;

    if (argument_count == 0) {
    report:
        chimera__console_out((ColorARGB *)0, "%s: %u", name, *value);
        return;
    }
    if (argument_count == 1) {
        char *text = arguments[0];
        if (text[0] != '\0') {
            strncpy(buffer, text, 0xff);
            buffer[0xff] = 0;
            string_to_lowercase(buffer);
            cursor = buffer;
            string_trim_whitespace(&cursor);
            if (strncmp(buffer, "0", 2) == 0 || strncmp(buffer, "false", 6) == 0) {
                *value = 0;
                goto report;
            }
            if (strncmp(buffer, "1", 2) == 0 || strncmp(buffer, "true", 5) == 0) {
                *value = 1;
                goto report;
            }
        }
    }
    chimera__console_out((ColorARGB *)0, "Incorrect usage. Type help %s for more information.", name);
}

#if 0
Original Ghidra decompilation (0x4e2990), from tools/pack.py 0x4e2990:

void console_command_bool_get_set(undefined4 *param_1,undefined4 param_2)

{
  char cVar1;
  int in_EAX;
  char *pcVar2;
  int iVar3;
  undefined1 *unaff_EBX;
  char *pcVar4;
  bool bVar5;
  char local_100 [255];
  undefined1 local_1;

  if (in_EAX == 0) {
LAB_004e2a5a:
    chimera__console_out("%s: %u",param_2,*unaff_EBX);
    return;
  }
  if (in_EAX == 1) {
    pcVar4 = (char *)*param_1;
    pcVar2 = pcVar4;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    if (pcVar2 != pcVar4 + 1) {
      _strncpy(local_100,pcVar4,0xff);
      local_1 = 0;
      string_to_lowercase();
      string_trim_whitespace();
      iVar3 = 2;
      bVar5 = true;
      pcVar4 = local_100;
      pcVar2 = "0";
      do {
        if (iVar3 == 0) break;
        iVar3 = iVar3 + -1;
        bVar5 = *pcVar4 == *pcVar2;
        pcVar4 = pcVar4 + 1;
        pcVar2 = pcVar2 + 1;
      } while (bVar5);
      if (!bVar5) {
        iVar3 = 6;
        bVar5 = true;
        pcVar4 = local_100;
        pcVar2 = "false";
        do {
          if (iVar3 == 0) break;
          iVar3 = iVar3 + -1;
          bVar5 = *pcVar4 == *pcVar2;
          pcVar4 = pcVar4 + 1;
          pcVar2 = pcVar2 + 1;
        } while (bVar5);
        if (!bVar5) {
          iVar3 = 2;
          bVar5 = true;
          pcVar4 = local_100;
          pcVar2 = "1";
          do {
            if (iVar3 == 0) break;
            iVar3 = iVar3 + -1;
            bVar5 = *pcVar4 == *pcVar2;
            pcVar4 = pcVar4 + 1;
            pcVar2 = pcVar2 + 1;
          } while (bVar5);
          if (!bVar5) {
            iVar3 = 5;
            bVar5 = true;
            pcVar4 = local_100;
            pcVar2 = "true";
            do {
              if (iVar3 == 0) break;
              iVar3 = iVar3 + -1;
              bVar5 = *pcVar4 == *pcVar2;
              pcVar4 = pcVar4 + 1;
              pcVar2 = pcVar2 + 1;
            } while (bVar5);
            if (!bVar5) goto LAB_004e2a78;
          }
          *unaff_EBX = 1;
          goto LAB_004e2a5a;
        }
      }
      *unaff_EBX = 0;
      goto LAB_004e2a5a;
    }
  }
LAB_004e2a78:
  chimera__console_out("Incorrect usage. Type help %s for more information.",param_2);
  return;
}
#endif
