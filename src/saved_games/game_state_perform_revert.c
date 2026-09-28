// game_state_perform_revert  (Ghidra: game_state_perform_revert, already named)
// address 0x538200, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md; PTR_chimera__revert_0069e7b0 is
// game_state_revert_proc (types/saved_games.h global list, 0x543a90); the trailing 13-callback
// loop is byte-for-byte the same loop as game_state_dispatch_load_callbacks (0x537f70), called
// here instead of reproduced inline since it does exactly the same thing.
// Phase 4 review: matched objdump 0x538200..0x538279 instruction for instruction (the 13-call
// loop is game_state_dispatch_load_callbacks inlined).
// register convention: no parameters, no return value.
// UNSURE: 0x00746fa4, 0x00719738, 0x0071973c, 0x0071974f and 0x00719754 are outside this
// module's documented globals (not listed in types/saved_games.h); left as opaque externs
// (likely interface-owned "revert available" / busy-state flags).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint8_t game_state_revert_available; // 0x006e2dd9
extern uint8_t unknown_00746fa4; // 0x00746fa4, UNSURE
extern uint16_t unknown_00719754; // 0x00719754, UNSURE (low word of a dword)
extern uint8_t unknown_0071973c; // 0x0071973c, UNSURE
extern uint8_t unknown_00719738; // 0x00719738, UNSURE
extern uint8_t unknown_0071974f; // 0x0071974f, UNSURE
extern uint8_t game_state_write_in_progress; // 0x006e3000
extern game_state_proc game_state_revert_proc; // 0x0069e7b0

extern void __stdcall Sleep(uint32_t milliseconds);
extern uint8_t game_state_read_persistent_storage(void); // 0x539330, bool in AL (result unused here)
extern void game_state_dispatch_load_callbacks(void); // 0x537f70

void game_state_perform_revert(void)
{
    if (game_state_revert_available == 0 && unknown_00746fa4 == 0) {
        unknown_00719754 = 0xffff;
        unknown_0071973c = 0;
        unknown_00719738 = 1;
        unknown_0071974f = 0;
        return;
    }

    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    game_state_revert_proc();
    game_state_read_persistent_storage();
    game_state_dispatch_load_callbacks();
}

#if 0
Original Ghidra decompilation (0x538200):

void game_state_perform_revert(void)

{
  undefined **ppuVar1;
  int iVar2;

  if ((DAT_006e2dd9 == '\0') && (DAT_00746fa4 == '\0')) {
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719738 = 1;
    DAT_0071974f = 0;
    return;
  }
  if (DAT_006e3000 != '\0') {
    while (DAT_006e3000 != '\0') {
      Sleep(0);
    }
  }
  (*(code *)PTR_chimera__revert_0069e7b0)();
  game_state_read_persistent_storage();
  ppuVar1 = &PTR_FUN_0069e7b4;
  iVar2 = 0xd;
  do {
    (*(code *)*ppuVar1)();
    ppuVar1 = ppuVar1 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
