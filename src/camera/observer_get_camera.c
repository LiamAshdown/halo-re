// observer_get_camera  (Ghidra: camera_get_globals_for_player; renamed for this rewrite)
// address 0x4479a0, size 23 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: out/phase4/camera_types_notes.md ("camera_get_globals_for_player: observer_get_camera");
// 0x006ac6d0 is observers[0].camera (types/camera.h), stride 0xa7 dwords = 0x29c bytes, matching
// the observer stride.
// register convention: player index in CX (in_CX); no stack parameters.

#include "tags.h"
#include "memory.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern observer observers[1]; // 0x006ac65c


// blam-cc: CX -> player_index
// Returns a pointer to the per-player published camera state, or 0 if the index is invalid.
observer_camera *observer_get_camera(int16_t player_index)
{
    if (player_index == -1) {
        return 0;
    }
    return &observers[player_index].camera;
}

#if 0
Original Ghidra decompilation (0x4479a0):

undefined4 * camera_get_globals_for_player(void)

{
  undefined4 *puVar1;
  short in_CX;

  puVar1 = (undefined4 *)0x0;
  if (in_CX != -1) {
    puVar1 = &DAT_006ac6d0 + in_CX * 0xa7;
  }
  return puVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
