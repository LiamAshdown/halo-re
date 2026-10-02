// ui_network_game_options_refresh  (Ghidra: FUN_004a3b30, renamed)
// renamed from FUN_004a3b30 in the naming pass
// address 0x4a3b30, size 56 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: functions.md: "Build/refresh callback for the network game-options list widget."
// Trivial wrapper forwarding its own implicit ECX/ESI to ui_network_game_options_populate (this session), matching
// the same zero-visible-argument call pattern as FUN_004a2270.c.
// register convention: same as ui_network_game_options_populate: ECX -> widget, ESI -> options_record.
// blam-cc: ECX -> widget, ESI -> options_record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void player_profile_set_default_server_options(void); // 0x53a150, foreign (profile module), UNSURE
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90
extern uint8_t ui_network_game_options_populate(widget_instance *widget, const uint8_t *options_record); // 0x4a3960

void ui_network_game_options_refresh(widget_instance *widget, const uint8_t *options_record)
{
    player_profile_set_default_server_options();
    widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
    ui_network_game_options_populate(widget, options_record);
}

#if 0
Original Ghidra decompilation (0x4a3b30):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a3b30(void)

{
  FUN_0053a150();
  widget_play_sound_effect();
  FUN_004a3960();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
