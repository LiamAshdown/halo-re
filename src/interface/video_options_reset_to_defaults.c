// video_options_reset_to_defaults  (Ghidra: FUN_004bb5e0, named in phase 4)
// address 0x4bb5e0, size 92 bytes (0x1ffc byte frame through _chkstk 0x628240)
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4bb5e0..0x4bb63b in the phase-4 review. Renamed from
// video_options_menu_open: it does not open anything, it fills a 0x1ffc byte profile with the
// default video options (player_profile_set_default_video_options, second argument 0),
// keeps the current gamma (byte 0x0071d1e4 into the profile at +0xa76) and repopulates the
// video options screen (grandparent of the button) from it, then plays sound 2. The first
// rewrite used a 0xa76 byte buffer, wrote the gamma at +0x1586 and played sound 0.
// Returns the result of the defaults call (AL).
// register convention: plain cdecl, one stack argument (the button widget).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t video_gamma_current; // 0x0071d1e4, UNSURE name

extern uint8_t player_profile_set_default_video_options(uint8_t *profile, int32_t flag); // 0x53b000, cdecl
extern void video_options_menu_populate(widget_instance *screen, uint8_t *settings); // 0x4baec0, cdecl
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

uint8_t video_options_reset_to_defaults(widget_instance *button)
{
    uint8_t profile[0x1ffc];
    uint8_t result = player_profile_set_default_video_options(profile, 0);

    if (result != 0) {
        profile[0xa76] = video_gamma_current;
        video_options_menu_populate(button->parent->parent, profile);
        widget_play_sound_effect(2);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4bb5e0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char FUN_004bb5e0(int param_1)

{
  char cVar1;
  undefined1 local_1ffc [2678];
  undefined1 local_1586;
  undefined4 uStack_4;

  uStack_4 = 0x4bb5ea;
  cVar1 = player_profile_set_default_video_options(local_1ffc,0);
  if (cVar1 != '\0') {
    local_1586 = DAT_0071d1e4;
    FUN_004baec0(*(undefined4 *)(*(int *)(param_1 + 0x30) + 0x30),local_1ffc);
    widget_play_sound_effect();
  }
  return cVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
