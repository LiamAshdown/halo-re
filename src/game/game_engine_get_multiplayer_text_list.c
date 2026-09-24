// game_engine_get_multiplayer_text_list  (Ghidra: game_engine_get_multiplayer_text_list,
// already named)
// address 0x4633f0, size 130 bytes
// name confidence: 0.55   rewrite confidence: 0.5
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

extern wchar_t empty_string; // 0x00660c34

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0

wchar_t *game_engine_get_multiplayer_text_list(void)
{
    datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, 0); // UNSURE: index elided by Ghidra
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
