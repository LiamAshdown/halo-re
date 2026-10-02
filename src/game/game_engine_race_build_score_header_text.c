// game_engine_race_build_score_header_text  (not a Ghidra function; the race game engine definition's +0x58 slot (build_score_header_text); no C existed, so that
//   stored pointer trapped as unlisted_46eaf0)
// address 0x46eaf0, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46eaf0..0x46eb4a: copies string 0xb2 (the variant dword +0x7c is 2) or 0x19 of
//   ui\multiplayer_game_text into the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern game_variant game_engine_variant; // 0x006f1c88
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0, blam-cc: ECX list, EDX index

static uint16_t *multiplayer_text(int16_t index)
{
    datum_index list = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text"); // 'ustr', 0x00660c38

    return list == 0xffffffff ? (uint16_t *)L"" : text_string_list_get_string(list, index);
}

wchar_t *game_engine_race_build_score_header_text(wchar_t *buffer)
{
    wcscpy(buffer, (const wchar_t *)multiplayer_text((int16_t)(game_engine_variant.engine.race.race_type == 2 ? 0xb2 : 0x19)));
    return buffer;
}
