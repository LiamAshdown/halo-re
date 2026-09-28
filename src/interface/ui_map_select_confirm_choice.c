// ui_map_select_confirm_choice  (no Ghidra function; Ghidra only created the mid-body fragment
//   0x49d850 "render_widget_recursive")
// address 0x49d7c0, size 234 bytes (0x49d7c0..0x49d8a9, `ret`s at 0x49d86d, 0x49d891, 0x49d8a9)
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: objdump 0x49d7c0..0x49d8aa. The only reference is the widget event handler table at
//   0x00692840 (neighbours 0x49d5d0, 0x49d5f0, 0x49d7a0, 0x49d8b0, 0x49df70 ui_free_profile_list),
//   and src/interface/README.md already describes it as "map list event handler that owns the
//   0x49d850 fragment (copies the selected map path, loads it, records it through 0x53d5e0)".
//   It is the multiplayer counterpart of ui_level_select_confirm_choice:
//   - the list widget's committed selection is the int16 at widget + 0x3c (types/interface.h's
//     list-widget note); it indexes ui_lists[ui_list_current] (0x006b3830 + 0xc * 0x00692c04,
//     count at +4, ui_list_item data at +8) and the item's `id` (+0x08) indexes map_list
//     (0x00712dcc, 0xc stride, path at +0). An out-of-range selection leaves the index at -1,
//     which the original then uses unchecked (kept).
//   - the file name after the last backslash of that path goes to cache_file_exists (0x442bb0,
//     EAX name, ESI = the 0x800-byte header buffer at esp+0x10), exactly as map_list_add_entry
//     derives it.
//   - on success: main_queue_map_change_by_name_or_clear (0x4c87a0, EDI = path, `mov edi,...` at
//     0x49d807 is still live at the call) and, for the first map_list entry whose path compares
//     equal (_stricmp, 0x628d8b), saved_game_last_mp_map_clear (0x53d5e0, cdecl) with that
//     entry's path, which writes it to lastmpmp.txt. The loop bound is map_list_count read at
//     entry (EBX).
//   - on failure: widget_play_sound_effect (0x498e90, AX = 4).
//   Returns the cache_file_exists result (AL, saved at esp+0xf).
//   0x49d850 (the `add esp,8` after the _stricmp call) was recorded by modules.json as a function
//   of its own; it is covered by this file (see out/phase4/orphans_notes.md).
// UNSURE: the name; the sound effect id 4's meaning.
// register convention: the widget as the one stack argument (`mov eax,[esp+0x4]` at entry).
//   // blam-cc: stack -> widget

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t ui_list_current;      // 0x00692c04
extern map_list_entry *map_list;     // 0x00712dcc
extern int32_t map_list_count;       // 0x00712dd0

extern uint8_t cache_file_exists(char *name, cache_file_header *header_out); // 0x442bb0, blam-cc: EAX name, ESI header_out
extern void main_queue_map_change_by_name_or_clear(char *name);              // 0x4c87a0, blam-cc: EDI name
extern void saved_game_last_mp_map_clear(const void *data);                  // 0x53d5e0, cdecl
extern void widget_play_sound_effect(int16_t effect_id);                     // 0x498e90, blam-cc: AX effect_id

// Accepts the selected entry of the multiplayer map list: if its cache file exists, queues the
// map change and remembers the map in lastmpmp.txt; otherwise plays the error sound. Returns
// whether the cache file exists.
uint8_t ui_map_select_confirm_choice(widget_instance *widget)
{
    int32_t selection = *(int16_t *)((uint8_t *)widget + 0x3c);
    int32_t count = map_list_count;
    int32_t map_index = -1;
    char *path;
    char *file_name;
    cache_file_header header;
    uint8_t exists;
    int32_t i;

    if (selection >= 0 && selection < ui_lists[ui_list_current].count) {
        map_index = ((ui_list_item *)ui_lists[ui_list_current].data)[selection].id;
    }
    path = map_list[map_index].path;

    file_name = strrchr(path, '\\');
    file_name = (file_name != 0) ? file_name + 1 : path;
    exists = cache_file_exists(file_name, &header);
    if (!exists) {
        widget_play_sound_effect(4);
        return exists;
    }

    main_queue_map_change_by_name_or_clear(path);
    for (i = 0; i < count; i++) {
        if (_stricmp(path, map_list[i].path) == 0) {
            saved_game_last_mp_map_clear(map_list[i].path);
            return exists;
        }
    }
    return exists;
}

#if 0
No Ghidra decompilation of 0x49d7c0 exists (Ghidra never created a function there). The fragment it
did create, 0x49d850 render_widget_recursive (the tail of the __stricmp loop, with unrecovered
register inputs); the rewrite above is from objdump:

undefined1 render_widget_recursive(void)

{
  int in_EAX;
  int unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  char *unaff_EDI;
  undefined4 in_stack_00000018;
  
  while( true ) {
    if (in_EAX == 0) {
      saved_game_last_mp_map_clear();
      return in_stack_00000018._3_1_;
    }
    unaff_ESI = unaff_ESI + 1;
    unaff_EBP = unaff_EBP + 0xc;
    if (unaff_EBX <= unaff_ESI) break;
    in_EAX = __stricmp(unaff_EDI,*(char **)(DAT_00712dcc + unaff_EBP));
  }
  return in_stack_00000018._3_1_;
}
#endif
