// input_joystick_pov_name_to_index  (Ghidra: FUN_00491590)
// address 0x491590, size 127 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/input_types_notes.md: "0x491590: parses a 'povN direction' name (prefix
// pov at 0x0065b920, number table 0x0065b988, direction via 0x491480). It is not a device-by-name
// lookup." objdump of 0x491590..0x49160e confirms the name string in EAX, the prefix
// joystick_pov_prefix (0x0065b920), the decimal-suffix loop bounded to 16 entries
// (0x65b988..0x65b9b8, i.e. k_control_gamepad_pov_count, not the full 32-entry table), and the
// output direction pointer as a plain stack argument.
// UNSURE: like input_joystick_axis_name_to_index, a short suffix can match inside a longer one
// (ascending order 0..15), and the code searches from prefix_match + 1 (not prefix_match + 3,
// i.e. not past the whole word "pov") for the digit suffix -- both kept exactly as decompiled.
// If the direction lookup on the matched suffix's remainder fails, the function returns -1
// immediately rather than trying the remaining suffixes.
// register convention: name in EAX; out_direction as the one stack parameter

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern char joystick_pov_prefix[0x18]; // 0x0065b920, "pov"
extern char decimal_suffixes[0x20][3]; // 0x0065b988, "0" .. "31" (only the first 16 are scanned)

extern char *strstr(const char *haystack, const char *needle); // CRT strstr (0x625430: the MSVC asm strstr, haystack then needle; case-sensitive)

    // blam-cc: name in EBX

// blam-cc: name in EAX, out_direction on the stack
// Resolves a "povN direction" style joystick input name string (e.g. "pov2 north") back to its
// numeric POV-hat index (0..15, the return value) and direction (0..7, written to
// *out_direction), or -1 if the name doesn't contain the "pov" prefix, no suffix matches, or the
// matched suffix's remainder isn't a valid direction name.
int16_t input_joystick_pov_name_to_index(char *name, int16_t *out_direction)
{
    char *after_prefix;
    char *suffix;
    char *match;
    char *rest;
    int32_t pov_index;
    int32_t length;
    int16_t direction_index;

    after_prefix = strstr(name, joystick_pov_prefix);
    if (after_prefix == (char *)0) {
        return -1;
    }

    match = after_prefix + 1;
    pov_index = 0;
    suffix = decimal_suffixes[0];
    while (pov_index < 0x10) { // suffix < 0x0065b9b8 in the binary
        rest = strstr(match, suffix);
        if (rest != (char *)0) {
            length = 0;
            while (suffix[length] != '\0') {
                length = length + 1;
            }
            // 0x4915d5..0x4915e9: the direction name handed to 0x491480 (in EBX) is the PREFIX
            // match + 1 + strlen(suffix), not the suffix match + strlen(suffix) -- so for a
            // name like "pov2 north" it is "v2 north" and never parses. Shipped behaviour,
            // reproduced (UNSURE whether any caller relies on it).
            rest = match + length;
            direction_index = input_joystick_pov_direction_name_to_index(rest);
            *out_direction = direction_index;
            if (direction_index != -1) {
                return (int16_t)pov_index;
            }
            return -1;
        }
        suffix = suffix + 3;
        pov_index = pov_index + 1;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x491590):

undefined2 FUN_00491590(short *param_1)

{
  char cVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  char *pcVar5;
  int iVar6;
  undefined1 *puVar7;

  iVar6 = -1;
  iVar3 = FUN_00625430();
  if (iVar3 != 0) {
    iVar6 = 0;
    puVar7 = &DAT_0065b988;
    while (iVar4 = FUN_00625430(iVar3 + 1,puVar7), iVar4 == 0) {
      puVar7 = puVar7 + 3;
      iVar6 = iVar6 + 1;
      if (0x65b9b7 < (int)puVar7) {
        return 0xffff;
      }
    }
    pcVar5 = &DAT_0065b988 + iVar6 * 3;
    do {
      cVar1 = *pcVar5;
      pcVar5 = pcVar5 + 1;
    } while (cVar1 != '\0');
    sVar2 = input_joystick_pov_direction_name_to_index();
    *param_1 = sVar2;
    if (sVar2 == -1) {
      return 0xffff;
    }
  }
  return (short)iVar6;
}
#endif
