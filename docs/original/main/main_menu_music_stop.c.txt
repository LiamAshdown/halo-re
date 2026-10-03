// main_menu_music_stop  (Ghidra: main_menu_music_stop, already named)
// address 0x4c8b40, size 73 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: matches the given name exactly; the "sound\\music\\title1\\title1" tag path and
// sound_looping_stop(EAX -> sound_tag) mirror src/interface/main_menu_play_title_music.c
// and src/interface/main_menu_on_shown.c's identical start/stop idiom for the same music.
// main_menu_music_pending (0x00718fc6) and input_mode_flags (0x00712542) reuse those files' and
// src/camera/camera_update.c's names. 0x00719756 is main_globals.main_menu_scenario_loaded
// (types/main.h, offset 0x056), NOT the interface module's own less-informed guess for the
// overlapping dword at 0x00719754 (see src/main/main_queue_map_change.c's file header for the
// same correction).
// register convention: no register-passed arguments.
// phase 4 review (disassembly 0x4c8b40..0x4c8b88): no drift; 0x00712542 is input_globals.mode_flags.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern main_globals main_globals_data; // 0x00719700
extern uint8_t main_menu_music_pending; // 0x00718fc6, foreign (interface module)
extern uint8_t ui_split_screen;         // 0x00718fc9, foreign (interface module)
extern input_abstraction_globals input_globals; // 0x00710328, foreign (input module); mode_flags at 0x00712542

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, foreign (cache module)
    // blam-cc: EDI -> group, stack -> path
extern void sound_looping_stop(datum_index sound_tag); // 0x544120, foreign (sound module)
    // blam-cc: EAX -> sound_tag (the tag_lookup result is still in EAX at 0x4c8b62)

// Stops the main menu's title theme music if it is currently playing, and clears the "showing
// the UI map" state (ui_split_screen, main_menu_scenario_loaded) and the menu-navigation input
// mode bit.
void main_menu_music_stop(void)
{
    if (main_menu_music_pending == 1) {
        datum_index sound_tag = tag_lookup(0x6c736e64 /* 'lsnd' */, (char *)"sound\\music\\title1\\title1");
        if (sound_tag != (datum_index)-1) {
            sound_looping_stop(sound_tag);
        }
        main_menu_music_pending = 0;
    }
    ui_split_screen = 0;
    main_globals_data.main_menu_scenario_loaded = 0;
    input_globals.mode_flags = input_globals.mode_flags & 0xfd;
}

#if 0
Original Ghidra decompilation (0x4c8b40):

void __cdecl main_menu_music_stop(void)

{
  int iVar1;

  if (DAT_00718fc6 == '\x01') {
    iVar1 = tag_lookup("sound\\music\\title1\\title1");
    if (iVar1 != -1) {
      looping_sound_object_detach();
    }
    DAT_00718fc6 = '\0';
  }
  DAT_00718fc9 = 0;
  DAT_00719754._2_1_ = 0;
  DAT_00712542 = DAT_00712542 & 0xfd;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
