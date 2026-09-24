// game_engine_unload  (Ghidra: game_engine_unload, already named)
// address 0x45c330, size 59 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/game.h current_game_engine (0x006f1d20, game_engine_definition::dispose at
//   +0x08), player_profile_cache (0x006b0b88, 16 x 0x30 == 0xc0 dwords) and
//   player_profile_cache_initialized (0x006f1d38); mirrors the same three globals
//   game_dispose.c (0x45acd0) and game_engine_load_from_variant.c (0x45c2c0) already use.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t player_profile_cache_initialized;     // 0x006f1d38
extern player_profile player_profile_cache[16];         // 0x006b0b88 (16 entries, 0xc0 dwords)

// Disposes of the currently loaded game engine (via its vtable's dispose entry) and, if it had
// been populated, zeroes the 16-entry player profile cache.
void __cdecl game_engine_unload(void)
{
    uint32_t *cache_words;
    int32_t i;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }
    if (player_profile_cache_initialized == 1) {
        cache_words = (uint32_t *)player_profile_cache;
        for (i = 0xc0; i != 0; i = i - 1) {
            *cache_words = 0;
            cache_words = cache_words + 1;
        }
        player_profile_cache_initialized = 0;
    }
}

#if 0
Original Ghidra decompilation (0x45c330), from tools/pack.py 0x45c330:

void __cdecl game_engine_unload(void)

{
  int iVar1;
  undefined4 *puVar2;

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
  return;
}
#endif
