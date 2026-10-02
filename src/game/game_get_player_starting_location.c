// game_get_player_starting_location  (Ghidra: FUN_00477640; named per this rewrite)
// address 0x477640, size 42 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Returns a pointer to the player-starting-location
//   record at the given index, or 0 if the index is out of range"); types/tags.h
//   ScenarioPlayerStartingLocation (0x34 bytes); Scenario::player_starting_locations reflexive
//   at scenario+0x354/+0x358 (matches types/game.h's own +0x354 anchor for this same
//   reflexive).
// register convention: CX -> index.
//   // blam-cc: CX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c

// Returns a pointer to player_starting_locations[index], or NULL if index is negative or past
// the reflexive's count.
ScenarioPlayerStartingLocation *game_get_player_starting_location(int16_t index)
    // blam-cc: CX -> index
{
    if (index >= 0 && index < global_scenario->player_starting_locations.count) {
        return &((ScenarioPlayerStartingLocation *)global_scenario->player_starting_locations.pointer)[index];
    }
    return (ScenarioPlayerStartingLocation *)0;
}

#if 0
Original Ghidra decompilation (0x477640), from tools/pack.py 0x477640:

int FUN_00477640(void)

{
  int iVar1;
  short in_CX;

  iVar1 = 0;
  if (-1 < in_CX) {
    if ((int)in_CX < *(int *)(global_scenario + 0x354)) {
      iVar1 = in_CX * 0x34 + *(int *)(global_scenario + 0x358);
    }
  }
  return iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
