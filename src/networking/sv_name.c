// sv_name  (Ghidra: sv_name, already named)
// address 0x4e2ed0, size 274 bytes
// name confidence: 0.9   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md; CEA-pdb match on all four literal strings;
// network_name_string_is_valid_for_mode (FUN_004e4350, already written) matches its own
// (name, dest, mode) signature here.
// register convention: EAX = argument_count, stack = arguments.
//   // blam-cc: EAX -> argument_count, stack -> arguments
// UNSURE: FUN_00557990 is presumed to be a stack/cookie-guard style helper that returns the
// address of the buffer it was handed; the `local_80 == pwVar5` comparison is preserved
// literally as a sanity check on its result rather than reinterpreted.
// UNSURE: FUN_004df070 (this module, network_password_field_set per its own file) is called
// here with zero visible arguments; its own (object, source) signature does not match this
// call site (this is the *name*, not password, sync path), so it is declared locally with no
// arguments instead, matching Ghidra literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#include <string.h>

extern uint16_t network_server_name[64]; // 0x00699588, "Halo" by default (UNSURE name)
extern uint8_t network_server_name_is_default; // 0x00699606 (UNSURE name)
extern network_server_globals *network_server; // 0x0071c2d4

extern wchar_t *string_convert_ascii_to_unicode(void); // foreign, presumed to return &scratch (UNSURE, see header)
extern uint8_t network_name_string_is_valid_for_mode(char *name, void *dest, int32_t mode); // 0x4e4350, this module
extern void network_password_field_set(void); // 0x4df070, this module, called here with no
    // visible arguments (UNSURE, see header)
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// blam-cc: EAX -> argument_count, stack -> arguments
// Console command: with no arguments, reports the current server name; with one, validates it
// is 1-63 characters of printable ASCII and, if so, converts and stores it as the new server
// name, resetting the "is default" flag and syncing the live server object if hosting.
void sv_name(uint32_t argument_count, char **arguments)
{
    wchar_t scratch[63];

    if (argument_count == 0) {
    report:
        chimera__console_out((ColorARGB *)0, "sv_name: %ls", network_server_name);
        return;
    }
    if (argument_count == 1) {
        char *name = arguments[0];
        int32_t length = (int32_t)strlen(name);

        if (length != 0 && (uint32_t)length < 0x40) {
            wchar_t *result = string_convert_ascii_to_unicode();
            if (result == scratch) {
                if (network_name_string_is_valid_for_mode(name, scratch, 3) != 0) {
                    network_server_globals *server = network_server;
                    wcsncpy(network_server_name, scratch, 0x3f);
                    network_server_name_is_default = 0;
                    if (server != 0) {
                        network_password_field_set();
                    }
                    goto report;
                }
            }
            chimera__console_out((ColorARGB *)0, "Server names must only contain printable ASCII characters supported by the Halo UI.");
            goto usage;
        }
        chimera__console_out((ColorARGB *)0, "Server names must be between 1 and %d characters.", 0x3f);
    }
usage:
    chimera__console_out((ColorARGB *)0, "Incorrect usage. Type help sv_name for more information.");
}

#if 0
Original Ghidra decompilation (0x4e2ed0), from tools/pack.py 0x4e2ed0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void sv_name(undefined4 *param_1)

{
  char *pcVar1;
  int iVar2;
  char cVar3;
  int in_EAX;
  char *pcVar4;
  wchar_t *pwVar5;
  wchar_t local_80 [63];
  undefined2 local_2;

  if (in_EAX == 0) {
LAB_004e2f8b:
    chimera__console_out("sv_name: %ls",u_Halo_00699588);
    return;
  }
  if (in_EAX == 1) {
    pcVar1 = (char *)*param_1;
    pcVar4 = pcVar1;
    do {
      cVar3 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar3 != '\0');
    if (pcVar4 != pcVar1 + 1) {
      pcVar4 = pcVar1;
      do {
        cVar3 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar3 != '\0');
      if ((uint)((int)pcVar4 - (int)(pcVar1 + 1)) < 0x40) {
        pwVar5 = (wchar_t *)FUN_00557990();
        local_2 = 0;
        if (local_80 == pwVar5) {
          cVar3 = FUN_004e4350(pcVar1,local_80,3);
          iVar2 = DAT_0071c2d4;
          if (cVar3 != '\0') {
            _wcsncpy(u_Halo_00699588,local_80,0x3f);
            _DAT_00699606 = 0;
            if (iVar2 != 0) {
              FUN_004df070();
            }
            goto LAB_004e2f8b;
          }
        }
        chimera__console_out
                  (
                  "Server names must only contain printable ASCII characters supported by the Halo UI."
                  );
        goto LAB_004e2fb7;
      }
    }
    chimera__console_out("Server names must be between 1 and %d characters.",0x3f);
  }
LAB_004e2fb7:
  chimera__console_out("Incorrect usage. Type help sv_name for more information.");
  return;
}
#endif
