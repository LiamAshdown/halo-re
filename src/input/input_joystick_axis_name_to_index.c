// input_joystick_axis_name_to_index  (Ghidra: FUN_004913e0)
// address 0x4913e0, size 155 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Resolves a joystick axis-plus-direction name
// string back to its numeric axis index and direction flag."; objdump of 0x4913e0..0x49147a
// confirms the name string arrives in EAX, the prefix searched for is joystick_axis_prefix
// "axis" (0x0065b908), and -- once Ghidra's confusing `pcVar5 + (iVar4 - ...)` pointer math is
// resolved algebraically -- the code simply strstr()s each decimal suffix ("0".."31") within the
// text following "axis", then parses whatever follows the matched digits as a direction name.
// UNSURE: because suffixes are tried in ascending numeric order and a short suffix ("0") can
// match inside a longer one ("10"), a name like "axis10 +" can fail to resolve (the "0" inside
// "10" matches first, leaving " +" -- with a leading space -- as the direction text). Kept
// exactly as decompiled; not fixed.
// register convention: name in EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>

extern char joystick_axis_prefix[0x18]; // 0x0065b908, "axis"
extern char decimal_suffixes[0x20][3];  // 0x0065b988, "0" .. "31"

extern char *_strstr(char *haystack, const char *needle); // 0x625430, libc
extern int16_t input_axis_direction_name_to_index(char *name); // this module, 0x4911f0

// blam-cc: name in EAX
// Resolves a joystick axis-plus-direction name string (e.g. "axis3 +") back to its numeric axis
// index (0..31, written to the return value) and direction (1 or 0, written to *out_direction),
// or -1 if the name doesn't contain the "axis" prefix or no suffix/direction combination matches.
int16_t input_joystick_axis_name_to_index(char *name, uint8_t *out_direction)
{
    char *after_prefix;
    char *suffix;
    char *match;
    char *rest;
    int32_t axis_index;
    int16_t direction_index;

    after_prefix = _strstr(name, joystick_axis_prefix);
    if (after_prefix == (char *)0) {
        return -1;
    }

    axis_index = 0;
    suffix = decimal_suffixes[0];
    while (axis_index < 0x20) { // suffix < 0x0065b9e8 in the binary
        match = _strstr(after_prefix, suffix);
        if (match != (char *)0) {
            rest = match + strlen(suffix);
            direction_index = input_axis_direction_name_to_index(rest);
            if (direction_index == 0) {
                *out_direction = 1;
                return (int16_t)axis_index;
            }
            if (direction_index == 1) {
                *out_direction = 0;
                return (int16_t)axis_index;
            }
            return -1;
        }
        suffix = suffix + 3;
        axis_index = axis_index + 1;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4913e0):

uint FUN_004913e0(undefined1 *param_1)

{
  char cVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  char *pcVar5;
  uint uVar6;
  undefined1 *puVar7;

  iVar3 = FUN_00625430();
  if (iVar3 != 0) {
    uVar6 = 0;
    puVar7 = &DAT_0065b988;
    do {
      iVar4 = FUN_00625430(iVar3,puVar7);
      if (iVar4 != 0) {
        pcVar5 = &DAT_0065b988 + uVar6 * 3;
        do {
          cVar1 = *pcVar5;
          pcVar5 = pcVar5 + 1;
        } while (cVar1 != '\0');
        sVar2 = input_axis_direction_name_to_index
                          (pcVar5 + (iVar4 - (int)(&DAT_0065b989 + uVar6 * 3)));
        if (sVar2 == 0) {
          *param_1 = 1;
          return uVar6 & 0xffff;
        }
        if (sVar2 == 1) {
          *param_1 = 0;
          return uVar6 & 0xffff;
        }
        return CONCAT22(sVar2 >> 0xf,0xffff);
      }
      puVar7 = puVar7 + 3;
      uVar6 = uVar6 + 1;
    } while ((int)puVar7 < 0x65b9e8);
  }
  return 0xffff;
}
#endif
