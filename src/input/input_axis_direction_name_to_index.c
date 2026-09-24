// input_axis_direction_name_to_index  (Ghidra: already named)
// address 0x4911f0, size 127 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Resolves an axis-direction display name
// string back to its numeric direction index (0 or 1)."; identical structure to
// input_keyboard_key_name_to_index.c, bounded to 2 entries and a 9-wide-character buffer.
// register convention: name pointer as the recognized parameter (param_1)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <wchar.h>
#include <string.h>

extern void input_get_axis_direction_name(int16_t direction_index, uint16_t *out_name); // this module, 0x491180
extern int32_t _stricmp(const char *a, const char *b); // 0x628d8b, libc

// Resolves an axis-direction display name string (ASCII, case-insensitive) back to its numeric
// direction index (0 or 1), or 0xffff if neither matches.
int16_t input_axis_direction_name_to_index(char *name)
{
    uint32_t direction_index;
    uint32_t length;
    uint32_t i;
    char ascii[12];
    uint16_t wide[9];

    direction_index = 0;
    for (;;) {
        input_get_axis_direction_name((int16_t)direction_index, wide);
        length = (uint32_t)wcslen(wide);
        if (length < 9) {
            for (i = 0; i < length; i++) {
                ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
            }
            ascii[i] = '\0';
        }
        if (_stricmp(name, ascii) == 0) {
            break;
        }
        direction_index = direction_index + 1;
        if ((int32_t)direction_index > 1) {
            return -1; // 0xffff in AX
        }
    }
    return (int16_t)direction_index; // returned in AX
}

#if 0
Original Ghidra decompilation (0x4911f0):

uint input_axis_direction_name_to_index(char *param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char local_20 [12];
  char local_14 [20];

  uVar4 = 0;
  while( true ) {
    input_get_axis_direction_name();
    uVar1 = FUN_00625b7a(local_14);
    if (uVar1 < 9) {
      uVar3 = 0;
      if (uVar1 != 0) {
        do {
          if (local_14[uVar3 * 2 + 1] == '\0') {
            local_20[uVar3] = local_14[uVar3 * 2];
          }
          else {
            local_20[uVar3] = ' ';
          }
          uVar3 = uVar3 + 1;
        } while (uVar3 < uVar1);
      }
      local_20[uVar3] = '\0';
    }
    iVar2 = __stricmp(param_1,local_20);
    if (iVar2 == 0) break;
    uVar4 = uVar4 + 1;
    if (1 < (int)uVar4) {
      return 0xffff;
    }
  }
  return uVar4 & 0xffff;
}
#endif
