// game_engine_get_time_scale  (Ghidra: game_engine_get_time_scale, already named)
// address 0x470ce0, size 34 bytes, cc=__cdecl
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/game.h game_time_globals::speed (+0x18); network_game_mode (0x00719720,
// forced to 1.0 for client/host).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t network_game_mode;    // 0x00719720
extern game_time_globals *game_time; // 0x006f1d6c

// Returns the effective simulation time-scale factor: always 1.0 while connected as a network
// client or host, otherwise the current tick record's own speed.
float game_engine_get_time_scale(void)
{
    if (network_game_mode != 1 && network_game_mode != 2) {
        return game_time->speed;
    }
    return 1.0f;
}

#if 0
Original Ghidra decompilation (0x470ce0), from tools/pack.py 0x470ce0:

float __cdecl game_engine_get_time_scale(void)

{
  if ((DAT_00719720 != 1) && (DAT_00719720 != 2)) {
    return *(float *)(DAT_006f1d6c + 0x18);
  }
  return 1.0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
