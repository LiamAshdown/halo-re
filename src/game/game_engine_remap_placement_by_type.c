// game_engine_remap_placement_by_type  (Ghidra: FUN_004630b0; renamed per its summary)
// address 0x4630b0, size 68 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Given a player referenced start/placement object,
// dispatches to the netgame-flag or netgame-equipment remapping routine based on that
// placement's stored type"); this batch's game_engine_resolve_netgame_flag_role (0x462df0, type
// 2) and game_engine_resolve_multiplayer_placement (0x462c30, type 3); types/cache.h
// tag_instance (stride 0x20, data at +0x14).
// register convention: a placement handle in EAX (in_EAX).
//   // blam-cc: EAX -> handle
// UNSURE: the double indirection (`**(short **)tag_data`, i.e. the tag data's own first field
// read as a pointer and then dereferenced again for its first short) has no established meaning
// -- ordinary tag data does not normally embed a live pointer as its first field. Transcribed
// literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern tag_instance *tag_instances;                 // 0x0087bc14

extern int32_t game_engine_resolve_netgame_flag_role(uint32_t handle); // 0x462df0, this batch
extern uint32_t game_engine_resolve_multiplayer_placement(uint32_t handle); // 0x462c30, this batch

// blam-cc: EAX -> handle
uint32_t game_engine_remap_placement_by_type(uint32_t handle)
{
    // returns EAX on every path: the handle itself when nothing is remapped (EAX is never written before the
    // early rets at 0x4630f3), otherwise the tail-jumped resolver's result (jmp 0x462df0 / 0x462c30)
    int16_t type;

    if (current_game_engine == 0 || handle == 0xffffffff) {
        return handle;
    }
    type = **(int16_t **)&tag_instances[handle & 0xffff].data;
    if (type == 2) {
        return (uint32_t)game_engine_resolve_netgame_flag_role(handle);
    }
    if (type == 3) {
        return game_engine_resolve_multiplayer_placement(handle);
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x4630b0), from tools/pack.py 0x4630b0:

void FUN_004630b0(void)

{
  short sVar1;
  uint in_EAX;

  if (((DAT_006f1d20 != 0) && (in_EAX != 0xffffffff)) &&
     (sVar1 = **(short **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), sVar1 != 1)) {
    if (sVar1 == 2) {
      FUN_00462df0();
      return;
    }
    if (sVar1 == 3) {
      FUN_00462c30();
      return;
    }
  }
  return;
}
#endif
