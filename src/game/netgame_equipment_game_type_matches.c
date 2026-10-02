// netgame_equipment_game_type_matches  (Ghidra: FUN_0045f7c0; named per
// out/phase4/game_functions.md)
// address 0x45f7c0, size 128 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Checks whether a netgame equipment placement's
// allowed game-type list matches the currently active game variant"); types/game.h
// game_engine_index (the 1..6 values compared against).
// register convention: entry count in EDX (in_EDX), current game_engine_index in ESI
// (unaff_ESI); the array pointer is this function's one recognized stack parameter.
//   // blam-cc: EDX -> count, ESI -> current_engine_index, stack -> types
// UNSURE: the special list values 0x0c/0x0d/0x0e are not documented anywhere in this batch's
// evidence; kept as literal constants ("wildcard", "all but ctf", "all but ctf and race" are
// guesses based on which game_engine_index values they exclude, not confirmed).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_engine_definition *current_game_engine; // 0x006f1d20

// blam-cc: EDX -> count, ESI -> current_engine_index, stack -> types
// Outside a running multiplayer game, matches only if every entry in `types[0..count)` is 0
// (the placement has no per-type restriction at all). While a game engine is loaded, matches if
// any entry equals `current_engine_index` directly, or is one of the special "combined" values
// 0x0c (always matches), 0x0d (matches unless the engine is ctf, index 1) or 0x0e (matches
// unless the engine is ctf or race, indices 1 or 5).
uint8_t netgame_equipment_game_type_matches(int16_t *types, int32_t count, int32_t current_engine_index)
{
    uint8_t result;
    int32_t i;

    if (current_game_engine == 0) {
        result = 1;
        for (i = 0; i < count; i++) {
            result = result & (types[i] == 0);
        }
        return result;
    }

    result = 0;
    for (i = 0; i < count; i++) {
        int16_t type = types[i];
        uint8_t matches_directly = (type == current_engine_index);
        result = result | matches_directly;

        if (type == 0x0c) {
            result = result | 1;
        } else if (type == 0x0d) {
            result = result | (current_engine_index != 1);
        } else if (type == 0x0e) {
            result = result | (current_engine_index != 1 && current_engine_index != 5);
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x45f7c0), from tools/pack.py 0x45f7c0:

uint FUN_0045f7c0(int param_1)

{
  short sVar1;
  byte bVar2;
  uint uVar3;
  undefined3 uVar4;
  int in_EDX;
  int unaff_ESI;
  int iVar5;
  bool bVar6;

  if (DAT_006f1d20 == 0) {
    iVar5 = 0;
    uVar3 = 1;
    if (0 < in_EDX) {
      do {
        uVar3 = (uint)((byte)uVar3 & *(short *)(param_1 + iVar5 * 2) == 0);
        iVar5 = iVar5 + 1;
      } while (iVar5 < in_EDX);
      return uVar3;
    }
  }
  else {
    uVar3 = DAT_006f1d20 & 0xffffff00;
    iVar5 = 0;
    if (0 < in_EDX) {
      do {
        sVar1 = *(short *)(param_1 + iVar5 * 2);
        uVar4 = (undefined3)(uVar3 >> 8);
        bVar2 = (byte)uVar3 | sVar1 == unaff_ESI;
        uVar3 = CONCAT31(uVar4,bVar2);
        if (sVar1 == 0xc) {
          uVar3 = uVar3 | 1;
        }
        else {
          if (sVar1 == 0xd) {
            bVar6 = unaff_ESI != 1;
          }
          else {
            if (sVar1 != 0xe) goto LAB_0045f822;
            if ((unaff_ESI == 1) || (unaff_ESI == 5)) {
              bVar6 = false;
            }
            else {
              bVar6 = true;
            }
          }
          uVar3 = CONCAT31(uVar4,bVar2 | bVar6);
        }
LAB_0045f822:
        iVar5 = iVar5 + 1;
      } while (iVar5 < in_EDX);
    }
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
