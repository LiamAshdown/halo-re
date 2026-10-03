// game_engine_get_multiplayer_text_list  (Ghidra: game_engine_get_multiplayer_text_list,
// already named)
// address 0x4633f0, size 130 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Returns the string-list data for the
// ui\multiplayer_game_text tag (or a built-in fallback list) used for multiplayer HUD/game text
// strings"); tag_lookup and text_string_list_get_string signatures already established in this
// batch's game_engine_build_kill_feed_message_text.c.
// UNSURE: text_string_list_get_string is called here with no visible index argument; modeled
// with index 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern wchar_t empty_string; // 0x00660c34

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0

// FIXED 2026-09-28 from objdump 0x4633f0..0x463471: the function takes the rank from
//   game_engine_compare_score_to_others on the stack (flag bits in the low word, the place in the high word) and
//   returns ui\multiplayer_game_text string 0x66 + index: 0x23 for flags 4|1, 0x21 / 0x22 for flag 4 in
//   place 0 / 1, 0x20 for flag 2, else the place (+0x10 with flag 1). Every caller is a build_message_text slot.
// blam-cc: stack -> rank
wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank)
{
    int16_t place = (int16_t)(rank >> 16);
    int32_t index;
    datum_index tag_id;

    if ((rank & 4) != 0 && (rank & 1) != 0) {
        index = 0x23;
    } else if ((rank & 4) != 0 && place == 0) {
        index = 0x21;
    } else if ((rank & 4) != 0 && place == 1) {
        index = 0x22;
    } else if ((rank & 2) != 0) {
        index = 0x20;
    } else {
        index = place;
        if ((rank & 1) != 0) {
            index += 0x10;
        }
    }
    tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, (int16_t)(index + 0x66));
}

#if 0
Original Ghidra decompilation (0x4633f0), from tools/pack.py 0x4633f0:

undefined * game_engine_get_multiplayer_text_list(void)

{
  int iVar1;
  undefined *puVar2;

  iVar1 = tag_lookup("ui\\multiplayer_game_text");
  if (iVar1 == -1) {
    return &DAT_00660c34;
  }
  puVar2 = (undefined *)text_string_list_get_string();
  return puVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
