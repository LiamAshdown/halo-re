// hs_help_print_function  (Ghidra: hs_help_print_function, already named)
// address 0x4841b0, size 183 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: looks up a function by name, prints its formatted signature
// (hs_format_function_signature), then prints its info string one line at a time (splitting on
// '\n', matching the multi-line documentation strings hs_doc also writes out).
// register convention: __cdecl; the function-name argument is not visible at all in Ghidra's
// decompile of this function (it shows zero parameters), so its register is unrecoverable from
// this batch's tools -- modeled as the first parameter/register slot, EAX, by the blam-cc
// convention's default.
// UNSURE: the argument register above; and buffer[2048] is sized to match hs_doc's identical
// local_800, but the true declared size at this address is not independently confirmed.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t hs_find_function_by_name(char *name); // 0x00483520, this batch
extern void hs_format_function_signature(int16_t function_index, char *out); // 0x00484300, this batch
extern void chimera__console_out(char *text); // 0x00496b50

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

// blam-cc: function name in EAX (see UNSURE above)
// Console command handler that prints a single HS function's formatted signature and
// documentation string (split across multiple console lines at each '\n').
void hs_help_print_function(char *name)
{
    int16_t function_index;
    char buffer[2048];
    char *newline;
    char *line;

    function_index = hs_find_function_by_name(name);
    if (function_index != -1) {
        hs_format_function_signature(function_index, buffer);
        chimera__console_out(buffer);
        strcpy(buffer, hs_function_definitions[function_index]->info);
        newline = strchr(buffer, '\n');
        if (newline == 0) {
            chimera__console_out(buffer);
            return;
        }
        line = buffer;
        while (line != 0) {
            if (newline == 0) {
                chimera__console_out(line);
                return;
            }
            *newline = '\0';
            chimera__console_out(line);
            line = newline + 1;
            newline = strchr(line, '\n');
        }
    }
}

#if 0
Original Ghidra decompilation (0x4841b0):

void hs_help_print_function(void)

{
  char cVar1;
  short sVar2;
  undefined1 *puVar3;
  char *pcVar4;
  char *pcVar5;
  char local_800 [2048];

  sVar2 = hs_find_function_by_name();
  if (sVar2 != -1) {
    hs_format_function_signature();
    chimera__console_out(local_800);
    pcVar4 = *(char **)((&PTR_DAT_00688b58)[sVar2] + 0x10);
    pcVar5 = local_800;
    do {
      cVar1 = *pcVar4;
      pcVar4 = pcVar4 + 1;
      *pcVar5 = cVar1;
      pcVar5 = pcVar5 + 1;
    } while (cVar1 != '\0');
    puVar3 = (undefined1 *)FUN_006257e0(local_800,10);
    if (puVar3 == (undefined1 *)0x0) {
      chimera__console_out(local_800);
      return;
    }
    pcVar5 = local_800;
    do {
      if (puVar3 == (undefined1 *)0x0) {
        chimera__console_out(pcVar5);
        return;
      }
      *puVar3 = 0;
      chimera__console_out(pcVar5);
      pcVar5 = puVar3 + 1;
      puVar3 = (undefined1 *)FUN_006257e0(pcVar5,10);
    } while (pcVar5 != (char *)0x0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
