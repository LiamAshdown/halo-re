// local_player_get_zoom_level  (Ghidra: FUN_00472740; renamed, no established name)
// address 0x472740, size 27 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase4/game_types_notes.md item 2 (see unit_invalidate_local_player_zoom_level.c);
// types/game.h local_player_control::desired_zoom_level (+0x24).
// register convention: local-player index in CX (Ghidra's `in_CX`).
//   // blam-cc: CX -> local_player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_control_globals *player_control_globals_ptr; // 0x006b145c

// blam-cc: CX -> local_player_index
// Returns the tracked desired_zoom_level for the given local player slot, or -1 if
// local_player_index itself is -1.
int32_t local_player_get_zoom_level(int16_t local_player_index)
{
    if (local_player_index != -1) {
        return player_control_globals_ptr->local_players[local_player_index].desired_zoom_level;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x472740), from tools/pack.py 0x472740:

undefined4 FUN_00472740(void)

{
  undefined4 uVar1;
  short in_CX;

  uVar1 = 0xffffffff;
  if (in_CX != -1) {
    uVar1 = CONCAT22((short)((uint)(in_CX * 0x40) >> 0x10),
                     *(undefined2 *)(in_CX * 0x40 + 0x34 + DAT_006b145c));
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
