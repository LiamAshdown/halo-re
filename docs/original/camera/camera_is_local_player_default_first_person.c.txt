// camera_is_local_player_default_first_person  (Ghidra: FUN_004455f0; renamed, Blam-style, not previously named)
// address 0x4455f0, size 74 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x4455f0..0x44563c; matches.
// evidence: out/phase4/camera_functions.md "Checks whether the first local player's camera is
//   currently in the default first-person mode." The scanning loop is written generically (as
//   if for several local players) but types/camera.h k_camera_local_player_count == 1 for this
//   build, so it can only ever inspect local player 0.
// register convention: __cdecl, no arguments (confirmed: no stack or register reads before the
//   loop starts at index 0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals; // 0x0087a478
extern director directors[1];                // 0x006ac560

extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command); // 0x446d60, this module

// True if the first local player exists and its active pov procedure is the default first
// person one (i.e. not third person, scripted, dead, flying or editor).
uint8_t camera_is_local_player_default_first_person(void)
{
    int16_t local_player_index;

    local_player_index = 0;
    while (local_player_index == -1 || local_player_index > 0 ||
           local_player_globals->local_players[local_player_index] == k_datum_index_none) {
        local_player_index++;
        if (local_player_index > 0) {
            return 0;
        }
    }

    return (directors[local_player_index].pov_proc == camera_first_person_compute_pov) ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x4455f0):

undefined1 FUN_004455f0(void)

{
  undefined1 uVar1;
  short sVar2;

  uVar1 = 0;
  sVar2 = 0;
  while (((sVar2 == -1 || (0 < sVar2)) || (*(int *)(DAT_0087a478 + 4 + sVar2 * 4) == -1))) {
    sVar2 = sVar2 + 1;
    if (0 < sVar2) {
      return uVar1;
    }
  }
  if ((sVar2 * 0xf8 != -0x6ac560) &&
     ((code *)(&DAT_006ac568)[sVar2 * 0x3e] == camera_first_person_compute_pov)) {
    uVar1 = 1;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
