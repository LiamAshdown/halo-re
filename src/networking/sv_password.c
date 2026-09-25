// sv_password  (Ghidra: sv_password, already named)
// address 0x4e2ff0, size 259 bytes
// name confidence: 0.9   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md; CEA-pdb match on all three literal strings;
// network_server_password_set (FUN_004e0910, already written, (source, server) signature)
// matches the final sync call here.
// register convention: EAX = argument_count, stack = arguments.
//   // blam-cc: EAX -> argument_count, stack -> arguments
// UNSURE: FUN_00557990 is presumed to be a stack/cookie-guard style helper that returns the
// address of the buffer it was handed, same as in sv_name.c.
// UNSURE: an empty password argument is accepted (`cVar3 = '\x01'` unconditionally) without
// going through network_name_string_is_valid_for_mode at all -- preserved exactly, since the
// summary would otherwise suggest validation always runs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#include <string.h>

extern uint16_t network_server_password[9]; // 0x0071c2f4 (UNSURE name; distinct storage from
    // network_server_globals::password, mirrored into it by network_server_password_set)
extern uint8_t network_server_password_is_default; // 0x0071c304 (UNSURE name)
extern network_server_globals *network_server; // 0x0071c2d4

extern wchar_t *string_convert_ascii_to_unicode(void); // foreign, presumed to return &scratch (UNSURE, see header)
extern uint8_t network_name_string_is_valid_for_mode(char *name, void *dest, int32_t mode); // 0x4e4350, this module
extern void network_server_password_set(const wchar_t *source, network_server_globals *server); // 0x4e0910, this module
extern void chimera__console_out(const char *format, ...); // 0x496b50

// blam-cc: EAX -> argument_count, stack -> arguments
// Console command: with no arguments, reports the current server password; with one, validates
// it is at most 8 characters, accepts it as-is if empty (clearing the password) or validates it
// as printable ASCII otherwise, then stores it and syncs the live server object if hosting.
void sv_password(uint32_t argument_count, char **arguments)
{
    wchar_t scratch[8];

    if (argument_count == 0) {
    report:
        chimera__console_out("sv_password: %ls", network_server_password);
        return;
    }
    if (argument_count == 1) {
        char *text = arguments[0];
        int32_t length = (int32_t)strlen(text);

        if ((uint32_t)length < 9) {
            wchar_t *result = string_convert_ascii_to_unicode();
            if (result == scratch) {
                uint8_t ok = 1;
                if (text[0] != '\0') {
                    ok = network_name_string_is_valid_for_mode(text, scratch, 0);
                }
                if (ok != 0) {
                    network_server_globals *server = network_server;
                    wcsncpy(network_server_password, scratch, 8);
                    network_server_password_is_default = 0;
                    if (server != 0) {
                        network_server_password_set(scratch, server);
                    }
                    goto report;
                }
            }
            chimera__console_out("Server passwords must only contain printable ASCII characters supported by the Halo UI.");
        } else {
            chimera__console_out("Server passwords must be no more than %d characters.", 8);
        }
    }
    chimera__console_out("Incorrect usage. Type help sv_password for more information.");
}

#if 0
Original Ghidra decompilation (0x4e2ff0), from tools/pack.py 0x4e2ff0:

void sv_password(undefined4 *param_1)

{
  char *pcVar1;
  int iVar2;
  char cVar3;
  int in_EAX;
  char *pcVar4;
  wchar_t *pwVar5;
  wchar_t local_14 [8];
  undefined2 local_4;

  if (in_EAX == 0) {
LAB_004e30a0:
    chimera__console_out("sv_password: %ls",&DAT_0071c2f4);
    return;
  }
  if (in_EAX == 1) {
    pcVar1 = (char *)*param_1;
    pcVar4 = pcVar1;
    do {
      cVar3 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar3 != '\0');
    if ((uint)((int)pcVar4 - (int)(pcVar1 + 1)) < 9) {
      pwVar5 = (wchar_t *)FUN_00557990();
      local_4 = 0;
      if (local_14 == pwVar5) {
        pcVar4 = pcVar1;
        do {
          cVar3 = *pcVar4;
          pcVar4 = pcVar4 + 1;
        } while (cVar3 != '\0');
        cVar3 = '\x01';
        if (pcVar4 != pcVar1 + 1) {
          cVar3 = FUN_004e4350(pcVar1,local_14,0);
        }
        iVar2 = DAT_0071c2d4;
        if (cVar3 != '\0') {
          _wcsncpy((wchar_t *)&DAT_0071c2f4,local_14,8);
          _DAT_0071c304 = 0;
          if (iVar2 != 0) {
            FUN_004e0910();
          }
          goto LAB_004e30a0;
        }
      }
      chimera__console_out
                (
                "Server passwords must only contain printable ASCII characters supported by the Halo UI."
                );
    }
    else {
      chimera__console_out("Server passwords must be no more than %d characters.",8);
    }
  }
  chimera__console_out("Incorrect usage. Type help sv_password for more information.");
  return;
}
#endif
