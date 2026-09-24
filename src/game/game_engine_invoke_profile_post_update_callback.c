// game_engine_invoke_profile_post_update_callback  (Ghidra: FUN_00466e60; named per
// out/phase4/game_functions.md: "Invokes the current game-engine's optional callback stored at
// offset 0x94, if one is registered.")
// address 0x466e60, size 23 bytes
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: types/game.h game_engine_definition::profile_post_update (+0x94, "0x466e60 and the
//   network layer (0x4dfa10)" per its own comment -- i.e. this function is that comment's cited
//   caller).
// register convention: ECX -> arg_ecx, EDX -> arg_edx; both forwarded unchanged to the callback
//   (pushed at 0x466e6f/0x466e70, call eax at 0x466e71).
//   // blam-cc: ECX -> arg_ecx, EDX -> arg_edx
// FIXED (register inputs, objdump): both ECX (pushed at 0x466e6f) and EDX (pushed at 0x466e70)
// are live-in and forwarded unchanged to the profile_post_update callback; the previous rewrite
// modeled the callback as taking no arguments. Neither register's real meaning is recoverable
// without a specific profile_post_update implementation (the callback pointer is data-driven,
// read out of the active game_engine_definition), so both are kept as opaque forwarded values.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20

// UNSURE: exact meaning of both arguments; see file header.
typedef void (*profile_post_update_proc)(uint32_t arg_edx, uint32_t arg_ecx);

// blam-cc: ECX -> arg_ecx, EDX -> arg_edx
// Invokes the active game engine's optional profile_post_update callback, if one is registered.
void game_engine_invoke_profile_post_update_callback(uint32_t arg_ecx, uint32_t arg_edx)
{
    profile_post_update_proc callback = (profile_post_update_proc)current_game_engine->profile_post_update;
    if (callback != 0) {
        callback(arg_edx, arg_ecx); // pushed in this order to match objdump: push ecx then push edx
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
