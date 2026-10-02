// chimera__main_menu_music  (Chimera signature name, kept)
// address 0x4921a0, size 97 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: orphan pass (modules.json guessed "input", confidence 0.5, "lone main inside input
// run"; out/phase4/input_types_notes.md and src/input/README.md both flag it as belonging to
// the interface module). Its only caller in the binary is 0x4a3d77, inside
// hud_text_message_queue_init 0x4a3ce0 (interface module, symbols/functions.txt), which computes
// the BL argument as `(DAT_00719230 == 2)` before the call and zeroes DAT_00719230 right after
// (objdump 0x4a3d67..0x4a3d84). The body reuses the exact "sound\music\title1\title1" /
// 'lsnd' tag_lookup / sound_looping_stop / main_menu_music_pending idiom that
// src/interface/main_menu_play_title_music.c, src/interface/main_menu_on_shown.c and
// src/main/main_menu_music_stop.c already share; tag_lookup and sound_looping_stop keep those
// files' blam-cc. main_menu_play_title_music (0x4993e0) is reached by a tail jump
// (objdump 0x4921fb: `jmp 0x4993e0`), not a call, so it is modeled as an ordinary call.
// register convention: Ghidra reported the flag as "unaff_BL" (never assigned in this function);
// objdump confirms it is read live at entry (`test bl,bl` at 0x4921d3 and 0x4921e9) with no
// prior write, so it is the sole incoming register argument.
// blam-cc: BL -> finalize_render_frame
// UNSURE: the exact meaning of the BL flag (finalize_render_frame) is inferred from its only use
// (gating rasterizer_end_frame/rasterizer_reset_device_if_needed around the bink playback); the
// caller names it from a plain loop-iteration-count comparison and does not explain it further.

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

extern uint8_t main_menu_music_pending; // 0x00718fc6

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, foreign (cache module)
    // blam-cc: EDI -> group, stack -> path
extern void sound_looping_stop(datum_index sound_tag); // 0x544120, foreign (sound module)
    // blam-cc: EAX -> sound_tag (the tag_lookup result is still in EAX at the call site)
extern void sound_stop_all(void);                        // 0x54adb0, foreign (sound module)
extern void rasterizer_end_frame(void);                   // 0x517b90, foreign (rasterizer module)
extern uint8_t rasterizer_reset_device_if_needed(void);    // 0x517500, foreign (rasterizer module)
extern void movie_play_bink(const char *movie_path);       // 0x43ed20, foreign (main module)
extern void main_menu_play_title_music(void);              // 0x4993e0

// Stops the main menu's looping title theme if it is still marked pending, stops every other
// sound, plays the "ending.bik" movie (bracketing it with an end-of-frame/device-reset pair when
// finalize_render_frame is set), and restarts the title music if it was not already stopped.
void chimera__main_menu_music(uint8_t finalize_render_frame)
{
    if (main_menu_music_pending == 1) {
        datum_index sound_tag = tag_lookup(0x6c736e64 /* 'lsnd' */, (char *)"sound\\music\\title1\\title1");
        if (sound_tag != (datum_index)-1) {
            sound_looping_stop(sound_tag);
        }
        main_menu_music_pending = 0;
    }
    sound_stop_all();
    if (finalize_render_frame != 0) {
        rasterizer_end_frame();
    }
    movie_play_bink("ending.bik");
    if (finalize_render_frame != 0) {
        rasterizer_reset_device_if_needed();
    }
    if (main_menu_music_pending == 0) {
        main_menu_play_title_music();
    }
}

#if 0
Original Ghidra decompilation (0x4921a0):

void chimera__main_menu_music(void)

{
  int iVar1;
  char unaff_BL;

  if (DAT_00718fc6 == '\x01') {
    iVar1 = tag_lookup("sound\\music\\title1\\title1");
    if (iVar1 != -1) {
      sound_looping_stop();
    }
    DAT_00718fc6 = '\0';
  }
  sound_stop_all();
  if (unaff_BL != '\0') {
    rasterizer_end_frame();
  }
  movie_play_bink("ending.bik");
  if (unaff_BL != '\0') {
    rasterizer_reset_device_if_needed();
  }
  if (DAT_00718fc6 == '\0') {
    main_menu_play_title_music();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
