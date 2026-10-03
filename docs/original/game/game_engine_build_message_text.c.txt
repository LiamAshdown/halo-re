// game_engine_build_message_text  (Ghidra: FUN_00460890; renamed -- it builds either a kill-feed
// or HUD-hint message string, trying the active game variant's override callback first)
// address 0x460890, size 56 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Builds a HUD/kill-feed message's text, preferring a
// game-variant-supplied override before falling back to the default builder"); the sole caller
// (game_engine_pick_hud_hint, FUN_00463150 in this same batch) calls it with zero visible
// arguments in Ghidra's own rendering, exactly like the two inlined copies of this same "try
// override, else call the default builder" pattern already committed in
// game_engine_on_player_death.c; types/game.h game_engine_definition::build_message_text
// (+0x6c); game_engine_build_kill_feed_message_text.c's own header ("EBX -> out, stack ->
// message_type, subject, buffer_size").
// register convention: objdump -d -M intel --start-address=0x460890 --stop-address=0x4608c8
// bin/halo.exe shows this function's own incoming EAX is saved to EBX and never touched again,
// then forwarded verbatim (still in EBX) into the call to game_engine_build_kill_feed_message_text
// -- i.e. it is this function's own "out" buffer, arriving in EAX. The two remaining values that
// call needs (subject, buffer_size) are never loaded from this function's own stack frame; they
// come from ESI/EDI exactly as they were on entry (unaff_ESI/unaff_EDI), i.e. genuine
// pass-through register arguments from this function's own caller.
//   // blam-cc: EAX -> out, unaff_ESI -> buffer_size, unaff_EDI -> subject,
//   //          stack -> param_1 (the override callback's own extra argument), message_type
// VERIFIED against disassembly 0x460890..0x4608c7 (2026-09-30). Fixed: the +0x6c override is called with five stack
//   args (param_1, message_type, subject, out, buffer_size) and 0x45e680 gets EAX = param_1 (the same player handle).
// RESOLVED (was modeled as void): the result is the same uint8_t "built" flag game_engine_build_kill_feed_message_text
//   returns.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_engine_definition *current_game_engine; // 0x006f1d20

// blam-cc: EAX -> recipient, EBX -> out, stack -> message_type, subject, buffer_size
extern uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type,
    datum_index subject, size_t buffer_size); // 0x45e680

// blam-cc: EAX -> out, unaff_ESI -> buffer_size, unaff_EDI -> subject, stack -> param_1, message_type
uint8_t game_engine_build_message_text(wchar_t *out, uint32_t buffer_size, datum_index subject,
    uint32_t param_1, uint32_t message_type)
{
    char handled = 0;

    if (current_game_engine->build_message_text != 0) {
        handled = ((char (*)(uint32_t, uint32_t, datum_index, wchar_t *, uint32_t))current_game_engine->build_message_text)
            (param_1, message_type, subject, out, buffer_size); // 0x4608a4..0x4608ad
    }
    if (handled == 0) {
        return game_engine_build_kill_feed_message_text(param_1, out, message_type, subject, buffer_size);
    }
    return (uint8_t)handled;
}

#if 0
Original Ghidra decompilation (0x460890), from tools/pack.py 0x460890:

void FUN_00460890(undefined4 param_1,undefined4 param_2)

{
  char cVar1;
  
  if ((*(code **)(DAT_006f1d20 + 0x6c) != (code *)0x0) &&
     (cVar1 = (**(code **)(DAT_006f1d20 + 0x6c))(param_1,param_2), cVar1 != '\0')) {
    return;
  }
  game_engine_build_kill_feed_message_text(param_2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
