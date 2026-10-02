// sv_banlist_file  (Ghidra: sv_banlist_file, already named)
// address 0x4e3db0, size 280 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; literal strings for the validation errors and
// the "%s\\banned%s.txt" path format; types/networking.h notes network_banlist_full_path
// (0x0071c308) is what this command builds.
// register convention: disassembly (objdump -d -M intel) shows `arguments` read from a genuine
// stack slot two pushes deep ([esp+0xc] after `push ecx; push esi`, landing above both spilled
// registers) -- the same EAX/stack shape as sv_ban_penalty.c and friends in this batch, not the
// ECX shape sv_ban.c/sv_kick.c use.
//   // blam-cc: EAX -> argument_count, stack -> arguments
// UNSURE: Ghidra's own decompile builds the destination path by copying the argument string
// in place over the tail of network_banlist_full_path's existing contents (a byte-for-byte walk
// that only makes sense as a very literal transcription of `strcpy`); this rewrite uses strcpy
// directly, which is observably identical for a NUL-terminated source.
// reconciled: R10 profile_directory is char[0x105] (k_profile_directory_storage_size; shell zeroes 0x41 dwords + 1 byte at 0x540ef9); the misdeclared `char *install_directory_path` at the same address is the profile_directory array (0x4e3e90 pushes 0x6ac900 itself)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char network_banlist_full_path[0x104]; // 0x0071c308
extern char profile_directory[0x105]; // 0x006ac900 (types/cache.h); pushed as an address (0x4e3e90)

extern void network_banlist_load(void); // this module, 0x4e3160 (excluded from this batch)
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: with no arguments, reports the current ban-list filename suffix; with one,
// validates it is non-empty, at most 0xf5 characters, and alphanumeric, then rebuilds
// network_banlist_full_path as "<install dir>\bannedSUFFIX.txt" and reloads the list.
void sv_banlist_file(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    if (argument_count == 0) {
    report:
        chimera__console_out((ColorARGB *)0, (char *)"sv_banlist_file: %s", network_banlist_full_path);
        return;
    }
    if (argument_count == 1) {
        char *suffix = (char *)arguments[0];
        int32_t len = strlen(suffix);

        if (len == 0) {
            chimera__console_out((ColorARGB *)0, (char *)"Ban file names must not be empty.");
        } else if (0xf5 < len) {
            chimera__console_out((ColorARGB *)0, (char *)"Ban file names cannot be longer than %d characters.", 0xfa);
        } else {
            int32_t i;
            for (i = 0; suffix[i] != 0; i = i + 1) {
                if (!isalnum((uint8_t)suffix[i])) {
                    chimera__console_out((ColorARGB *)0, (char *)"Ban list file names must be alphanumeric.");
                    goto usage;
                }
            }
            strcpy(network_banlist_full_path, suffix);
            sprintf(network_banlist_full_path, "%s\\banned%s.txt", profile_directory, suffix);
            network_banlist_load();
            goto report;
        }
    }
usage:
    chimera__console_out((ColorARGB *)0, (char *)"Incorrect usage. Type help sv_banlist_file for more information.");
}

#if 0
Original Ghidra decompilation (0x4e3db0), from tools/pack.py 0x4e3db0:

void sv_banlist_file(undefined4 *param_1)

{
  char cVar1;
  char *pcVar2;
  int in_EAX;
  char *pcVar3;
  int iVar4;
  int iVar5;

  if (in_EAX == 0) {
LAB_004e3eb1:
    chimera__console_out("sv_banlist_file: %s",&DAT_0071c308);
    return;
  }
  if (in_EAX == 1) {
    pcVar2 = (char *)*param_1;
    pcVar3 = pcVar2;
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    if (pcVar3 == pcVar2 + 1) {
      chimera__console_out("Ban file names must not be empty.");
    }
    else {
      pcVar3 = pcVar2;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
      } while (cVar1 != '\0');
      if (0xf5 < (uint)((int)pcVar3 - (int)(pcVar2 + 1))) {
        chimera__console_out("Ban file names cannot be longer than %d characters.",0xfa);
        chimera__console_out();
        return;
      }
      iVar5 = 0;
      cVar1 = *pcVar2;
      while (cVar1 != '\0') {
        iVar4 = _isalnum((int)pcVar2[iVar5]);
        if (iVar4 == 0) {
          chimera__console_out("Ban list file names must be alphanumeric.");
          break;
        }
        iVar4 = iVar5 + 1;
        iVar5 = iVar5 + 1;
        cVar1 = pcVar2[iVar4];
      }
      if (pcVar2[iVar5] == '\0') {
        pcVar3 = pcVar2;
        do {
          cVar1 = *pcVar3;
          pcVar3[(int)&DAT_0071c308 - (int)pcVar2] = cVar1;
          pcVar3 = pcVar3 + 1;
        } while (cVar1 != '\0');
        __snprintf(&DAT_0071c308,0x104,"%s\\banned%s.txt",&DAT_006ac900,pcVar2);
        network_banlist_load();
        goto LAB_004e3eb1;
      }
    }
  }
  chimera__console_out();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
