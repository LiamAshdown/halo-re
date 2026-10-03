// local_player_to_player_index  (Ghidra: local_player_to_player_index, already named)
// address 0x474d30, size 30 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/game_functions.md ("Looks up the player index currently assigned to a
//   given local-player slot"); types/game.h player_globals::local_players (+0x04), k_maximum_local_players (1).
// register convention: AX -> local_player_index.
//   // blam-cc: AX -> local_player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals; // 0x0087a478

// Returns the player handle currently bound to local-player slot `local_player_index`, or the
// wildcard if the slot index is out of range (this build only ever has slot 0).
datum_index local_player_to_player_index(int16_t local_player_index)
    // blam-cc: AX -> local_player_index
{
    if (local_player_index != -1 && local_player_index < 1) {
        return local_player_globals->local_players[local_player_index];
    }
    return (datum_index)-1;
}

#if 0
Original Ghidra decompilation (0x474d30), from tools/pack.py 0x474d30:

undefined4 local_player_to_player_index(void)

{
  short in_AX;

  if ((in_AX != -1) && (in_AX < 1)) {
    return *(undefined4 *)(DAT_0087a478 + 4 + in_AX * 4);
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
