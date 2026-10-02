// player_profile_cache_initialize  (Ghidra: player_profile_cache_initialize, already named)
// address 0x466c20, size 63 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/game.h player_profile_cache[16] (0x006b0b88), player_profile_cache_count
//   (0x006f1d34), player_profile_cache_initialized (0x006f1d38); the 0xc0-dword loop is exactly
//   16 * sizeof(player_profile) (0x300 bytes) and the second, byte-stride-0x30 loop separately
//   (and redundantly, since the first loop already zeroed it) clears each slot's `in_use` byte.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern player_profile player_profile_cache[16];    // 0x006b0b88
extern int32_t player_profile_cache_count;          // 0x006f1d34
extern uint8_t player_profile_cache_initialized;    // 0x006f1d38

// One-time initializer: zeroes the whole 16-slot player-profile cache table (and, redundantly,
// each slot's `in_use` flag a second time) and resets the live count. A no-op after the first
// call.
void player_profile_cache_initialize(void)
{
    uint8_t *bytes;
    uint32_t i;
    int32_t slot;

    if (player_profile_cache_initialized != 0) {
        return;
    }

    bytes = (uint8_t *)player_profile_cache;
    for (i = 0; i < sizeof(player_profile_cache); i = i + 1) {
        bytes[i] = 0;
    }

    for (slot = 0; slot < 16; slot = slot + 1) {
        player_profile_cache[slot].in_use = 0;
    }

    player_profile_cache_count = 0;
    player_profile_cache_initialized = 1;
}

#if 0
Original Ghidra decompilation (0x466c20), from tools/pack.py 0x466c20:

void player_profile_cache_initialize(void)

{
  undefined4 *puVar1;
  int iVar2;

  if (DAT_006f1d38 == '\0') {
    puVar1 = &DAT_006b0b88;
    for (iVar2 = 0xc0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar1 = 0;
      puVar1 = puVar1 + 1;
    }
    puVar1 = &DAT_006b0b88;
    do {
      *(undefined1 *)puVar1 = 0;
      puVar1 = puVar1 + 0xc;
    } while ((int)puVar1 < 0x6b0e88);
    DAT_006f1d34 = 0;
    DAT_006f1d38 = '\x01';
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
