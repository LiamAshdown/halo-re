// network_name_string_is_valid_for_mode  (Ghidra: FUN_004e4350; renamed -- see evidence)
// address 0x4e4350, size 276 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md ("Validates that a user-entered string (e.g. a
// player or game name) contains only characters renderable by the UI font and satisfies
// mode-specific leading-character restrictions"); the "ui\\small_ui" tag lookup and
// text_get_character_metrics/FUN_004a8b80 callee pair, both foreign (interface/text modules).
// register convention: __cdecl, three recognized parameters.
// UNSURE: `param_2` (passed to FUN_004a8b80 as an implicit register per that function's own
// unresolved shape) and the exact mode values beyond 1 and 3 are not otherwise attested in this
// batch; FUN_004a8b10/FUN_004a8b80/text_get_character_metrics/tag_lookup are all foreign to this
// module and declared with minimal shapes.

#include "tags.h"
#include "memory.h"
#include <string.h>

extern void *tag_lookup(const char *tag_path); // foreign, UNSURE shape
extern int32_t text_get_character_metrics(uint8_t ch); // foreign, UNSURE shape: this call site
    // passes only the character in Ghidra's own decompile; a font handle almost certainly also
    // flows through an unresolved register
extern uint8_t virtual_keyboard_character_is_legal(uint8_t ch, void *character); // foreign, UNSURE shape
extern uint8_t ui_wide_string_has_non_whitespace(void); // foreign, UNSURE shape

// Checks that every character of `name` is renderable in the small UI font and, for mode 3
// (UNSURE: player-name entry), that the name is non-empty and does not begin with a space or
// byte 0xa0. For mode 1 (UNSURE: server-name entry), additionally requires FUN_004a8b10 to pass.
uint8_t network_name_string_is_valid_for_mode(char *name, void *character, int32_t mode)
{
    uint8_t ok = 1;
    int32_t len;
    int32_t i;

    tag_lookup("ui\\small_ui");
    len = strlen(name);
    if (mode == 3) {
        ok = *name != 0;
        if (!ok) {
            return ok;
        }
    }
    for (i = 0; i < len; i = i + 1) {
        uint8_t ch = (uint8_t)name[i];
        if (ch < ' ' || ch == 0xff || text_get_character_metrics(ch) == 0 || virtual_keyboard_character_is_legal(ch, character) == 0) {
            ok = 0;
            break;
        }
        if (mode == 3) {
            if (i == 0) {
                if (*name == ' ') {
                    return 0;
                }
                if ((uint8_t)*name == 0xa0) {
                    return 0;
                }
            }
            ok = 1;
        }
    }
    if (mode != 1) {
        return ok;
    }
    return ok != 0 && ui_wide_string_has_non_whitespace() != 0;
}

#if 0
Original Ghidra decompilation (0x4e4350), from tools/pack.py 0x4e4350:

bool FUN_004e4350(char *param_1,undefined4 param_2,int param_3)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  bool local_9;

  local_9 = true;
  tag_lookup("ui\\small_ui");
  pcVar2 = param_1;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  if ((param_3 == 3) && (local_9 = *param_1 != '\0', !local_9)) {
    return local_9;
  }
  iVar4 = 0;
  if (0 < (int)pcVar2 - (int)(param_1 + 1)) {
    do {
      if ((((param_1[iVar4] < ' ') || (param_1[iVar4] == 0xff)) ||
          (iVar3 = text_get_character_metrics(), iVar3 == 0)) ||
         (cVar1 = FUN_004a8b80(), cVar1 == '\0')) {
        local_9 = false;
        break;
      }
      if (param_3 == 3) {
        if (iVar4 == 0) {
          if (*param_1 == ' ') {
            return false;
          }
          if (*param_1 == -0x60) {
            return false;
          }
        }
        local_9 = true;
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < (int)pcVar2 - (int)(param_1 + 1));
  }
  if (param_3 != 1) {
    return local_9;
  }
  if ((local_9 != false) && (cVar1 = FUN_004a8b10(), cVar1 != '\0')) {
    return true;
  }
  return false;
}
#endif
