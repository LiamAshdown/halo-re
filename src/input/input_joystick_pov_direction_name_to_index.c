// input_joystick_pov_direction_name_to_index  (Ghidra: already named)
// address 0x491480, size 56 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/input_functions.md summary "Resolves a POV-hat compass direction name
// (north, northeast, ...) back to its numeric direction index."; out/phase4/input_types_notes.md
// corrects the table to pov_direction_names (0x0065b938, 10-byte stride, 8 entries, ending at
// 0x0065b988), not decimal_suffixes -- the "north" literal Ghidra shows is that table's first
// entry, and 0x65b988 is simply the address right after it (decimal_suffixes begins there).
// register convention: name in EBX (unaff_EBX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char pov_direction_names[8][10]; // 0x0065b938, "north" .. "northwest"

extern int32_t _stricmp(const char *a, const char *b); // 0x628d8b, libc

// blam-cc: name in EBX
// Resolves a POV-hat compass direction name (case-insensitive) back to its numeric direction
// index (0 north .. 7 northwest), or -1 if none match.
int16_t input_joystick_pov_direction_name_to_index(char *name)
{
    char *entry;
    int16_t index;

    index = 0;
    entry = pov_direction_names[0];
    while (index < 8) { // entry < 0x0065b988 in the binary
        if (_stricmp(name, entry) == 0) {
            return index;
        }
        entry = entry + 10;
        index = index + 1;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x491480):

short input_joystick_pov_direction_name_to_index(void)

{
  int iVar1;
  char *unaff_EBX;
  char *_Str2;
  short sVar2;

  sVar2 = 0;
  _Str2 = "north";
  do {
    iVar1 = __stricmp(unaff_EBX,_Str2);
    if (iVar1 == 0) {
      return sVar2;
    }
    _Str2 = _Str2 + 10;
    sVar2 = sVar2 + 1;
  } while ((int)_Str2 < 0x65b988);
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
