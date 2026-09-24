// game_engine_koth_reset_hill_marker_history  (Ghidra: FUN_0046b250; named per this rewrite)
// address 0x46b250, size 156 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: types/game.h king_hill_marker_history (0x0087a9a0, position[4] + state[4]).
//   CORRECTED against out/phase4/game_functions.md's summary ("seeding all four tracked slots
//   with the current hill location"): all four positions are seeded from the SAME fixed
//   constant at 0x00686b04, already named object_placement_default_network_vectors[4] in
//   src/objects/object_placement_data_initialize.c (its first element), not from any live hill
//   state -- there is no read of king_globals or any hill position anywhere in this function.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern real_vector3d object_placement_default_network_vectors[4]; // 0x00686b04
extern king_hill_marker_history king_hill_markers; // 0x0087a9a0

// Resets the moving-hill marker position history: all four slots are seeded with the same fixed
// default vector (not a live hill position -- see evidence), and all four state slots are
// cleared to 0.
void game_engine_koth_reset_hill_marker_history(void)
{
    int32_t i;
    for (i = 0; i < 4; i++) {
        king_hill_markers.position[i].x = object_placement_default_network_vectors[0].i;
        king_hill_markers.position[i].y = object_placement_default_network_vectors[0].j;
        king_hill_markers.position[i].z = object_placement_default_network_vectors[0].k;
        king_hill_markers.state[i] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x46b250), from tools/pack.py 0x46b250:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046b250(void)

{
  _DAT_0087a9a0 = *(undefined4 *)PTR_DAT_00686b04;
  _DAT_0087a9a4 = *(undefined4 *)(PTR_DAT_00686b04 + 4);
  _DAT_0087a9a8 = *(undefined4 *)(PTR_DAT_00686b04 + 8);
  _DAT_0087a9ac = *(undefined4 *)PTR_DAT_00686b04;
  _DAT_0087a9b0 = *(undefined4 *)(PTR_DAT_00686b04 + 4);
  _DAT_0087a9b4 = *(undefined4 *)(PTR_DAT_00686b04 + 8);
  _DAT_0087a9b8 = *(undefined4 *)PTR_DAT_00686b04;
  _DAT_0087a9bc = *(undefined4 *)(PTR_DAT_00686b04 + 4);
  _DAT_0087a9c0 = *(undefined4 *)(PTR_DAT_00686b04 + 8);
  _DAT_0087a9c4 = *(undefined4 *)PTR_DAT_00686b04;
  _DAT_0087a9c8 = *(undefined4 *)(PTR_DAT_00686b04 + 4);
  _DAT_0087a9cc = *(undefined4 *)(PTR_DAT_00686b04 + 8);
  _DAT_0087a9d0 = 0;
  _DAT_0087a9d4 = 0;
  _DAT_0087a9d8 = 0;
  _DAT_0087a9dc = 0;
  return;
}
#endif
