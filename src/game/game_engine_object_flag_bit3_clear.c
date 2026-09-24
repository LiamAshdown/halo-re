// game_engine_object_flag_bit3_clear  (Ghidra: FUN_00462c10; renamed per its summary)
// address 0x462c10, size 32 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Low-confidence boolean accessor for one bit of the
// game engine option bitfield, gated on the engine being active and a valid object id");
// types/game.h game_variant::flags (+0x38, aliased 0x006f1cc0).
// register convention: no register-passed arguments; param_1 is this function's own stack
// parameter (an object/player handle, tested only against -1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (flags aliased 0x006f1cc0)

uint8_t game_engine_object_flag_bit3_clear(int32_t handle)
{
    uint8_t result = 1;

    if (current_game_engine != 0 && handle != -1) {
        result = (~(uint8_t)(game_engine_variant.flags >> 3)) & 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x462c10), from tools/pack.py 0x462c10:

byte FUN_00462c10(int param_1)

{
  byte bVar1;

  bVar1 = 1;
  if ((DAT_006f1d20 != 0) && (param_1 != -1)) {
    bVar1 = ~(byte)(DAT_006f1cc0 >> 3) & 1;
  }
  return bVar1;
}
#endif
