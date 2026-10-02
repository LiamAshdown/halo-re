// teams_are_enemies  (Ghidra: FUN_0045bd50, already named per symbols/functions.txt)
// address 0x45bd50, size 91 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: types/game.h team_pair_globals::enemy_bits (0xa4, indexed a+b*10, inverted from the
//   reflexive default team_pair_table_init_defaults seeds); network_game_mode / current_game_engine
//   gate (types/game.h globals list, 0x006f1d20).
// register convention: both team indices are the recognized register parameters (in_CX, in_DX).
//   // blam-cc: ECX -> team_a, EDX -> team_b (per Ghidra's in_CX/in_DX naming)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern team_pair_globals *team_pair_data;            // 0x006b0b84

// Returns whether two 0-9 team indices are hostile. Outside a loaded multiplayer game engine
// this just compares the indices; inside one, out-of-range indices default to "enemies" and
// in-range ones consult the enemy_bits table (reflexively seeded, so same-team never matches).
uint8_t teams_are_enemies(int16_t team_a, int16_t team_b)
    // blam-cc: in_CX -> team_a, in_DX -> team_b
{
    int32_t index;

    if (current_game_engine != (game_engine_definition *)0) {
        return team_b != team_a;
    }
    if (-1 < team_b && team_b < 10 && -1 < team_a && team_a < 10) {
        index = (int32_t)team_a + team_b * 10;
        return (uint8_t)(1 - ((team_pair_data->enemy_bits[index >> 5] & (1u << (index & 0x1f))) != 0));
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x45bd50), from tools/pack.py 0x45bd50:

bool FUN_0045bd50(void)

{
  int iVar1;
  char cVar2;
  short in_CX;
  short in_DX;

  cVar2 = '\x01';
  if (DAT_006f1d20 != 0) {
    return in_DX != in_CX;
  }
  if ((((-1 < in_DX) && (in_DX < 10)) && (-1 < in_CX)) && (in_CX < 10)) {
    iVar1 = (int)in_CX + in_DX * 10;
    cVar2 = '\x01' - ((1 << ((byte)iVar1 & 0x1f) & *(uint *)(DAT_006b0b84 + 0xa4 + (iVar1 >> 5) * 4)
                      ) != 0);
  }
  return (bool)cVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
