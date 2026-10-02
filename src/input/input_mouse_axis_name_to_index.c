// input_mouse_axis_name_to_index  (Ghidra: already named)
// address 0x4910c0, size 172 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Resolves a mouse axis-plus-direction display
// name string back to its axis index and direction."; same ASCII-fold + stricmp pattern as
// input_keyboard_key_name_to_index.c, nested over 3 axes x 2 directions.
// UNSURE: the decompiled `if ((short)uVar5 != -1) goto LAB_00491167;` around the match is
// unreachable (the outer loop variable is never -1), so the match always returns immediately;
// reproduced as a plain early return.
// register convention: name pointer and out_direction as the two recognized parameters

#include "crt.h"
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

extern void input_get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name);
    // this module, 0x491010

// Resolves a mouse axis-plus-direction display name string (ASCII, case-insensitive) back to
// its axis index (0 .. k_control_mouse_axis_count - 1) and direction (1 or 0, written to
// *out_direction), or 0xffff if none match.
uint32_t input_mouse_axis_name_to_index(char *name, uint8_t *out_direction)
{
    static const uint8_t k_directions[2] = { 1, 0 };
    uint32_t axis_index;
    int32_t dir;
    uint32_t length;
    uint32_t i;
    char ascii[36];
    uint16_t wide[33];

    for (axis_index = 0; axis_index <= (uint32_t)k_control_mouse_axis_count - 1; axis_index++) {
        for (dir = 0; dir < 2; dir++) {
            input_get_mouse_axis_name((int16_t)axis_index, k_directions[dir], wide);
            length = (uint32_t)wcslen((const wchar_t *)wide);
            if (length < 0x21) {
                for (i = 0; i < length; i++) {
                    ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
                }
                ascii[i] = '\0';
            }
            if (_stricmp(name, ascii) == 0) {
                *out_direction = k_directions[dir];
                return axis_index & 0xffff;
            }
        }
    }
    return 0xffff;
}

#if 0
Original Ghidra decompilation (0x4910c0):

uint input_mouse_axis_name_to_index(char *param_1,undefined1 *param_2)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  undefined1 local_6c [4];
  char local_68 [36];
  char local_44 [68];

  uVar4 = 0xffffffff;
  local_6c[0] = 1;
  local_6c[1] = 0;
  uVar1 = 0;
  do {
    uVar5 = uVar1;
    iVar6 = 0;
    do {
      input_get_mouse_axis_name(uVar5,local_6c[iVar6]);
      uVar1 = FUN_00625b7a(local_44);
      if (uVar1 < 0x21) {
        uVar3 = 0;
        if (uVar1 != 0) {
          do {
            if (local_44[uVar3 * 2 + 1] == '\0') {
              local_68[uVar3] = local_44[uVar3 * 2];
            }
            else {
              local_68[uVar3] = ' ';
            }
            uVar3 = uVar3 + 1;
          } while (uVar3 < uVar1);
        }
        local_68[uVar3] = '\0';
      }
      iVar2 = __stricmp(param_1,local_68);
      if (iVar2 == 0) {
        *param_2 = local_6c[iVar6];
        uVar4 = uVar5;
        if ((short)uVar5 != -1) goto LAB_00491167;
        break;
      }
      iVar6 = iVar6 + 1;
    } while (iVar6 < 2);
    uVar1 = uVar5 + 1;
    uVar5 = uVar4;
    if (2 < (int)uVar1) {
LAB_00491167:
      return uVar5 & 0xffff;
    }
  } while( true );
}
#endif
