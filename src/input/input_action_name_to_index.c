// input_action_name_to_index  (Ghidra: already named)
// address 0x48fe60, size 55 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/input_functions.md summary "Resolves a textual game-control/action name
// (e.g. from a 'bind' command) to its numeric control id via case-insensitive string table
// lookup."; types/input.h names the table input_action_names[0x1b][0x10] at 0x0065b730, ending
// at 0x0065b8e0, and k_control_binding_unbound (saved_games.h) is the documented "not found"
// sentinel 0x7fff.
// register convention: name in EBX (unaff_EBX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern char input_action_names[k_input_action_count][0x10]; // 0x0065b730, "jump" .. "screenshot"

extern int32_t _stricmp(const char *a, const char *b); // 0x628d8b, libc

// blam-cc: name in EBX
// Resolves a game-control/action name (case-insensitive) to its input_action index, or
// k_control_binding_unbound (0x7fff) if none match.
int16_t input_action_name_to_index(char *name)
{
    char *entry;
    int16_t index;

    index = 0;
    entry = input_action_names[0];
    do {
        if (_stricmp(name, entry) == 0) {
            return index;
        }
        entry = entry + 0x10;
        index = index + 1;
    } while (index < k_input_action_count); // entry < 0x0065b8e0 in the binary
    return k_control_binding_unbound;
}

#if 0
Original Ghidra decompilation (0x48fe60):

short input_action_name_to_index(void)

{
  int iVar1;
  char *unaff_EBX;
  char *_Str2;
  short sVar2;

  sVar2 = 0;
  _Str2 = "jump";
  do {
    iVar1 = __stricmp(unaff_EBX,_Str2);
    if (iVar1 == 0) {
      return sVar2;
    }
    _Str2 = _Str2 + 0x10;
    sVar2 = sVar2 + 1;
  } while ((int)_Str2 < 0x65b8e0);
  return 0x7fff;
}
#endif
