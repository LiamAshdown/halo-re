// path_split_components  (Ghidra: FUN_00556000, renamed)
// address 0x556000, size 202 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/saved_games_functions.md summary "Splits a path string into directory,
// filename-stem and extension boundary pointers, similar to _splitpath."
// Confirmed against objdump 0x556000..0x5560c9: ESI (register) is the path string; EBX and EDI
// (registers) and the two stack pointer arguments are four independent char** out-parameters,
// all initialised to end-of-string before the backward scan; the third (stack) argument is the
// char "split extension" flag read at [esp+0x10]/[esp+0xc] throughout. text_find_character_boundary (this
// module's neighbour at 0x5576d0, outside this batch) is called with (path, &remaining_length)
// and returns the character just before that length in AX, decrementing the length by 1 or 2
// characters to respect double-byte (DBCS) lead bytes -- see text_char_is_double_byte (0x557750).
// register convention: EBX, ESI, EDI as described above; two stack pointer arguments then the
// stack char flag (confirmed by objdump: [esp+8] and [esp+0xc] before the push ebp in the
// prologue are the two stack pointers, [esp+0x10] afterwards is the flag).
// UNSURE: the exact directory/stem/extension role of each of the four out-parameters is not
// pinned down beyond what the byte-for-byte pointer writes below already say; treated as
// mechanically as the binary does rather than guessed at semantically.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int16_t text_find_character_boundary(char *path, int16_t *remaining_length); // 0x5576d0, outside this batch

// blam-cc: dir_start_out in EBX, path in ESI, ext_fallback_out in EDI; name_end_out and
// ext_start_out as ordinary stack arguments (in that order), then the stack char
// split_extension flag
// Scans path backward one (possibly double-byte) character at a time. Every one of the four
// out-parameters starts pointing at path's terminating NUL. The first '\\' encountered sets
// dir_start_out to just past itself (once), unless split_extension is set and no extension has
// been found yet, in which case the path is truncated in place at that backslash and
// ext_fallback_out is set to just past it instead (marking "no extension"). If split_extension
// is set, the first '.' encountered (while ext_fallback_out and ext_start_out are still both
// unset) truncates the path in place at that '.' and sets ext_start_out to just past it. If the
// scan reaches the very start of the string with split_extension set and ext_fallback_out still
// unset, ext_fallback_out is set to path itself; otherwise, if ext_fallback_out never moved off
// path, name_end_out is set to path.
void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension)
{
    char *end;
    int16_t length;
    int16_t remaining;
    int16_t ch;

    end = path;
    while (*end != '\0') {
        end++;
    }
    length = (int16_t)(end - path);

    *name_end_out = end;
    *dir_start_out = end;
    *ext_fallback_out = end;
    *ext_start_out = end;
    remaining = length;

    while (remaining != 0) {
        ch = text_find_character_boundary(path, &remaining);
        if (ch == '.') {
            if (split_extension != 0 && **ext_fallback_out == '\0' && **ext_start_out == '\0') {
                path[remaining] = '\0';
                *ext_start_out = path + remaining + 1;
            }
        } else if (ch == '\\') {
            if (split_extension == 0 || **ext_fallback_out != '\0') {
                if (**dir_start_out == '\0') {
                    *dir_start_out = path + remaining + 1;
                }
            } else {
                path[remaining] = '\0';
                *ext_fallback_out = path + remaining + 1;
            }
        }
    }

    if (split_extension != 0 && **ext_fallback_out == '\0') {
        *ext_fallback_out = path;
        return;
    }
    if (*ext_fallback_out != path) {
        *name_end_out = path;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x556000):

void FUN_00556000(undefined4 *param_1,undefined4 *param_2,char param_3)

{
  char cVar1;
  short sVar2;
  char *pcVar3;
  short sVar4;
  int *unaff_EBX;
  char *unaff_ESI;
  undefined4 *unaff_EDI;

  pcVar3 = unaff_ESI;
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  sVar4 = (short)pcVar3 - ((short)unaff_ESI + 1);
  pcVar3 = unaff_ESI + sVar4;
  *param_1 = pcVar3;
  *unaff_EBX = (int)pcVar3;
  *unaff_EDI = pcVar3;
  *param_2 = pcVar3;
  while (sVar4 != 0) {
    sVar2 = FUN_005576d0();
    if (sVar2 == 0x2e) {
      if (((param_3 != '\0') && (*(char *)*unaff_EDI == '\0')) && (*(char *)*param_2 == '\0')) {
        unaff_ESI[sVar4] = '\0';
        *param_2 = unaff_ESI + sVar4 + 1;
      }
    }
    else if (sVar2 == 0x5c) {
      if ((param_3 == '\0') || (*(char *)*unaff_EDI != '\0')) {
        if (*(char *)*unaff_EBX == '\0') {
          *unaff_EBX = (int)(unaff_ESI + sVar4 + 1);
        }
      }
      else {
        unaff_ESI[sVar4] = '\0';
        *unaff_EDI = unaff_ESI + sVar4 + 1;
      }
    }
  }
  if ((param_3 != '\0') && (*(char *)*unaff_EDI == '\0')) {
    *unaff_EDI = unaff_ESI;
    return;
  }
  if ((char *)*unaff_EDI != unaff_ESI) {
    *param_1 = unaff_ESI;
  }
  return;
}
#endif
