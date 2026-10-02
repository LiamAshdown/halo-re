// path_remove_last_component  (Ghidra: path_remove_last_component, already named)
// address 0x555f80, size 113 bytes
// name confidence: 0.8   rewrite confidence: 0.55
// evidence: already named by Ghidra/CEA. Confirmed against objdump 0x555f80..0x555fe5: EBX
// (register) is the path buffer, with no stack arguments; text_find_character_boundary (0x5576d0, outside this
// batch) is called as (path, &remaining_length) exactly as in path_split_components.c, and its
// double-byte-aware backward character step is used to find the rightmost '\\' (or the start of
// the string if there is none), then text_char_is_double_byte (0x557750) determines whether that
// final character is one or two bytes wide before truncating path there.
// register convention: path buffer in EBX (confirmed by objdump; no stack arguments).
// UNSURE: the double-byte handling of the final truncation point is transliterated as literally
// as possible from the Ghidra output; see path_split_components.c for the same caveat.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t text_find_character_boundary(char *path, int16_t *remaining_length); // 0x5576d0, outside this batch
extern uint8_t text_char_is_double_byte(const char *at); // 0x557750, outside this batch

// blam-cc: path buffer in EBX
// Truncates path in place by removing its trailing path component: scans backward from the end
// (respecting double-byte characters) for the rightmost '\\', then NULs the string at (or just
// after, if no backslash was found) that position.
void path_remove_last_component(char *path)
{
    char *end;
    int16_t length;
    int16_t remaining;
    int16_t ch;
    int16_t last_backslash_pos;
    char *at;
    uint8_t is_double_byte;
    uint16_t final_char;
    int16_t final_width;

    end = path;
    while (*end != '\0') {
        end++;
    }
    length = (int16_t)(end - path);

    remaining = length;
    do {
        last_backslash_pos = 0;
        if (remaining == 0) {
            break;
        }
        ch = text_find_character_boundary(path, &remaining);
        last_backslash_pos = remaining;
    } while (ch != '\\');

    at = path + last_backslash_pos;
    is_double_byte = text_char_is_double_byte(at);
    if (!is_double_byte) {
        final_char = (uint16_t)(uint8_t)*at;
        final_width = 1;
    } else {
        final_width = 2;
        final_char = (uint16_t)(((uint8_t)at[0] << 8) | (uint8_t)at[1]);
    }
    if (final_char == '\\') {
        path[last_backslash_pos + final_width - 1] = '\0';
        return;
    }
    path[last_backslash_pos + final_width] = '\0';
    return;
}

#if 0
Original Ghidra decompilation (0x555f80):

void path_remove_last_component(void)

{
  char cVar1;
  short sVar2;
  ushort uVar3;
  char *pcVar4;
  char *unaff_EBX;
  short sVar5;
  short sVar6;
  byte *pbVar7;

  pcVar4 = unaff_EBX;
  do {
    cVar1 = *pcVar4;
    pcVar4 = pcVar4 + 1;
  } while (cVar1 != '\0');
  sVar5 = (short)pcVar4 - ((short)unaff_EBX + 1);
  do {
    sVar6 = 0;
    if (sVar5 == 0) break;
    sVar2 = FUN_005576d0();
    sVar6 = sVar5;
  } while (sVar2 != 0x5c);
  pbVar7 = (byte *)(unaff_EBX + sVar6);
  cVar1 = text_char_is_double_byte();
  if (cVar1 == '\0') {
    uVar3 = (ushort)*pbVar7;
    sVar5 = 1;
  }
  else {
    sVar5 = 2;
    uVar3 = CONCAT11(*pbVar7,pbVar7[1]);
  }
  if (uVar3 == 0x5c) {
    unaff_EBX[(short)(sVar6 + sVar5 + -1)] = '\0';
    return;
  }
  unaff_EBX[(short)(sVar6 + sVar5)] = '\0';
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
