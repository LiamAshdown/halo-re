// game_engine_invoke_profile_post_update_callback  (Ghidra: FUN_00466e60; named per
// out/phase4/game_functions.md: "Invokes the current game-engine's optional callback stored at
// offset 0x94, if one is registered.")
// address 0x466e60, size 23 bytes
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: types/game.h game_engine_definition::profile_post_update (+0x94, "0x466e60 and the
//   network layer (0x4dfa10)" per its own comment -- i.e. this function is that comment's cited
//   caller).
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20

// Invokes the active game engine's optional profile_post_update callback, if one is registered.
void game_engine_invoke_profile_post_update_callback(void)
{
    void (*callback)(void) = (void (*)(void))current_game_engine->profile_post_update;
    if (callback != (void (*)(void))0) {
        callback();
    }
}

#if 0
Original Ghidra decompilation (0x466e60), from tools/pack.py 0x466e60:

void FUN_00466e60(void)

{
  if (*(code **)(DAT_006f1d20 + 0x94) != (code *)0x0) {
    (**(code **)(DAT_006f1d20 + 0x94))();
  }
  return;
}
#endif
