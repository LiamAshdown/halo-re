// input_mouse_button_name_to_index  (Ghidra: already named)
// address 0x490f90, size 127 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Resolves a mouse button display name string
// back to its numeric button index."; identical structure to
// input_keyboard_key_name_to_index.c, bounded by k_control_mouse_button_count instead.
// register convention: name pointer as the recognized parameter (param_1)

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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void input_get_mouse_button_name(int16_t button_index, uint16_t *out_name); // this module, 0x490f20

// Resolves a mouse button display name string (ASCII, case-insensitive) back to its numeric
// button index (0 .. k_control_mouse_button_count - 1), or 0xffff if none match.
uint32_t input_mouse_button_name_to_index(char *name)
{
    uint32_t button_index;
    uint32_t length;
    uint32_t i;
    char ascii[24];
    uint16_t wide[24];

    button_index = 0;
    for (;;) {
        input_get_mouse_button_name((int16_t)button_index, wide);
        length = (uint32_t)wcslen((const wchar_t *)wide);
        if (length < 0x18) {
            for (i = 0; i < length; i++) {
                ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
            }
            ascii[i] = '\0';
        }
        if (_stricmp(name, ascii) == 0) {
            break;
        }
        button_index = button_index + 1;
        if ((int32_t)button_index > (int32_t)k_control_mouse_button_count - 1) {
            return 0xffff;
        }
    }
    return button_index & 0xffff;
}

#if 0
Original Ghidra decompilation (0x490f90):

uint input_mouse_button_name_to_index(char *param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char local_48 [24];
  char local_30 [48];

  uVar4 = 0;
  while( true ) {
    input_get_mouse_button_name();
    uVar1 = FUN_00625b7a(local_30);
    if (uVar1 < 0x18) {
      uVar3 = 0;
      if (uVar1 != 0) {
        do {
          if (local_30[uVar3 * 2 + 1] == '\0') {
            local_48[uVar3] = local_30[uVar3 * 2];
          }
          else {
            local_48[uVar3] = ' ';
          }
          uVar3 = uVar3 + 1;
        } while (uVar3 < uVar1);
      }
      local_48[uVar3] = '\0';
    }
    iVar2 = __stricmp(param_1,local_48);
    if (iVar2 == 0) break;
    uVar4 = uVar4 + 1;
    if (7 < (int)uVar4) {
      return 0xffff;
    }
  }
  return uVar4 & 0xffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
