// main_menu_on_shown  (Ghidra: FUN_00498ab0; named by out/phase4/interface_types_notes.md's own
// widget_instance note: "widget_instance + 0x15 ... tested by main_menu_on_shown @0x498ab0")
// address 0x498ab0, size 111 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: types_notes.md names this address directly; the body detaches any pending title-menu
// music (looking the tag back up rather than caching the earlier datum, matching
// main_menu_play_title_music's own tag_lookup call) and then arms the current root widget's
// auto-close/fade timer from a caller-supplied duration, clearing that controller's go-back
// history. Disassembly (0x498ab0..0x498ad6) confirms tag_lookup's dropped EDI group argument
// ('lsnd') and that sound_looping_stop receives the found tag index in EAX, the same
// pattern as main_menu_play_title_music.
// register convention: cdecl, one stack parameter (fade_milliseconds); Ghidra fully resolved it.
// UNSURE: sound_looping_stop (0x544120, sound module) is declared with its
// best-recovered register convention only; its own decompile was not read in this session.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint8_t main_menu_music_pending;    // 0x00718fc6
extern widget_instance *ui_root_widget[1]; // 0x00718f94
extern int32_t ui_time_milliseconds;       // 0x00718f9c
extern widget_history_node *ui_widget_history[3]; // 0x00718f98

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void sound_looping_stop(datum_index sound_tag); // 0x544120, sound module, UNSURE signature
// blam-cc: EAX -> sound_tag
extern void widget_pool_list_free_all(widget_history_node **head); // 0x4994b0

// Called when the main menu widget is (re)shown: detaches the title music if it is still marked
// pending (it never actually started playing, e.g. because the tag lookup or the earlier call
// raced), then, if the root widget is present and not already closing, arms its auto-close timer
// so it fades out `fade_milliseconds` after a 100 ms hold past its remaining creation-based
// budget, and discards controller 0's go-back history.
void main_menu_on_shown(int32_t fade_milliseconds)
{
    if (main_menu_music_pending == 1) {
        datum_index sound_tag = tag_lookup(0x6c736e64 /* 'lsnd' */, "sound\\music\\title1\\title1");

        if (sound_tag != (datum_index)-1) {
            sound_looping_stop(sound_tag);
        }
        main_menu_music_pending = 0;
    }
    if (ui_root_widget[0] != (widget_instance *)0 && ui_root_widget[0]->is_error_dialog == 0) {
        ui_root_widget[0]->milliseconds_auto_close_fade = fade_milliseconds;
        ui_root_widget[0]->milliseconds_to_auto_close =
            (ui_time_milliseconds - ui_root_widget[0]->creation_time) + 100;
        if (ui_widget_history[0] != (widget_history_node *)0) {
            widget_pool_list_free_all(&ui_widget_history[0]);
        }
    }
}

#if 0
Original Ghidra decompilation (0x498ab0):

void FUN_00498ab0(undefined4 param_1)

{
  int iVar1;

  if (DAT_00718fc6 == '\x01') {
    iVar1 = tag_lookup("sound\\music\\title1\\title1");
    if (iVar1 != -1) {
      looping_sound_object_detach();
    }
    DAT_00718fc6 = '\0';
  }
  if ((DAT_00718f94 != 0) && (*(char *)(DAT_00718f94 + 0x15) == '\0')) {
    *(undefined4 *)(DAT_00718f94 + 0x20) = param_1;
    *(int *)(DAT_00718f94 + 0x1c) = (DAT_00718f9c - *(int *)(DAT_00718f94 + 0x18)) + 100;
    if (DAT_00718f98 != 0) {
      FUN_004994b0();
    }
  }
  return;
}
#endif
