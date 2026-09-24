// game_engine_is_map_and_variant_valid  (Ghidra: FUN_00463920; renamed per its summary)
// address 0x463920, size 81 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Checks whether the currently loaded map is present/
// enabled in the multiplayer maps table and, optionally, that the active game variant is a
// recognized one"); this batch's game_engine_get_variant_by_name (0x4622d0); src/game/
// game_engine_initialize_for_new_game.c's map_list_find_known_map_index signature.
// register convention: a map path in ESI (unaff_ESI, passed to strrchr to isolate the filename);
// an optional variant name in EDI (unaff_EDI, gates -- and, when non-NULL, is forwarded to --
// the existence check).
//   // blam-cc: unaff_ESI -> map_path, unaff_EDI -> variant_name
// UNSURE: map_list_find_known_map_index is called here (and at its other call site) with no
// visible arguments; presumably it consumes strrchr's return value via a register this
// decompilation does not surface.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

extern int32_t multiplayer_map_count;   // 0x00712dd0, UNSURE identity
extern uint8_t *multiplayer_map_table;  // 0x00712dcc, UNSURE identity, stride 0xc, enabled flag at +8

extern char *_strrchr(const char *str, int32_t ch); // 0x623bc0
extern int32_t map_list_find_known_map_index(void); // 0x494ff0
extern uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out);
    // 0x4622d0, this module; blam-cc: ECX -> name, stack -> out. A NULL `out` only tests
    // whether the name is recognized.
uint32_t game_engine_is_map_and_variant_valid(const char *map_path, const char *variant_name)
{
    int32_t map_index;

    _strrchr(map_path, '\\');
    map_index = map_list_find_known_map_index();

    if (map_index != -1 && -1 < map_index && map_index < multiplayer_map_count &&
        multiplayer_map_table[8 + map_index * 0xc] != 0) {
        if (variant_name != 0) {
            return (uint32_t)game_engine_get_variant_by_name(variant_name, 0); // UNSURE: name elided by Ghidra
                // by Ghidra (shown as a literal 0), modeled with variant_name forwarded instead
        }
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x463920), from tools/pack.py 0x463920:

uint FUN_00463920(void)

{
  uint uVar1;
  uint uVar2;
  char *unaff_ESI;
  int unaff_EDI;

  _strrchr(unaff_ESI,0x5c);
  uVar1 = map_list_find_known_map_index();
  uVar2 = uVar1;
  if ((((uVar1 != 0xffffffff) && (-1 < (int)uVar1)) && ((int)uVar1 < DAT_00712dd0)) &&
     (uVar2 = uVar1 * 3, *(char *)(DAT_00712dcc + 8 + uVar1 * 0xc) != '\0')) {
    uVar2 = CONCAT31((int3)(uVar2 >> 8),1);
    if (unaff_EDI != 0) {
      uVar2 = game_engine_get_variant_by_name(0);
    }
    return uVar2;
  }
  return uVar2 & 0xffffff00;
}
#endif
