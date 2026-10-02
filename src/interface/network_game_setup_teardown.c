// network_game_setup_teardown  (Ghidra: FUN_00495520, unnamed)
// address 0x495520, size 87 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Tears down the multiplayer game-setup UI widget
// and clears the transient network/game-setup state and player-profile cache."; src/game/
// game_start_new_map.c's confirmed current_game_engine (0x006f1d20, dispose vtable slot +8) and
// player_profile_cache (0x006b0b88, player_profile[16]); src/game/game_dispose.c's
// player_profile_cache_initialized (0x006f1d38); src/game/game_engine_apply_variant.c's
// game_engine_active_variant (0x0087ab20, game_variant, 0x98 bytes); src/game/
// game_engine_ensure_variant_history_has_entry.c's game_variant_saved_default_valid
// (0x00714e78); src/interface/widget_close.c's precedent name for 0x00719720.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t game_variant_saved_default_valid;      // 0x00714e78
extern int16_t network_game_mode; // 0x00719720, types/game.h: 0 local, 1 client, 2 host (word access)
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t player_profile_cache_initialized;       // 0x006f1d38
extern player_profile player_profile_cache[16];         // 0x006b0b88
extern game_variant game_engine_active_variant;         // 0x0087ab20

// Clears the "game variant has an unsaved default" flag and the per-controller network-setup
// discriminant, disposes the current game engine definition if one is installed (calling its
// dispose vtable slot at +8), resets the player-profile cache if it was initialized, and clears
// the active game_variant working copy.
void network_game_setup_teardown(void)
{
    game_variant_saved_default_valid = 0;
    network_game_mode = 0;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }

    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }

    memset(&game_engine_active_variant, 0, sizeof(game_engine_active_variant));
}

#if 0
Original Ghidra decompilation (0x495520):

void FUN_00495520(void)

{
  int iVar1;
  undefined4 *puVar2;

  DAT_00714e78 = 0;
  DAT_00719720 = 0;
  if (DAT_006f1d20 != 0) {
    if (*(code **)(DAT_006f1d20 + 8) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 8))();
    }
    DAT_006f1d20 = 0;
  }
  if (DAT_006f1d38 == '\x01') {
    puVar2 = &DAT_006b0b88;
    for (iVar1 = 0xc0; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    DAT_006f1d38 = '\0';
  }
  puVar2 = &DAT_0087ab20;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
