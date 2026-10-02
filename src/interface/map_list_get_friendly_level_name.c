// map_list_get_friendly_level_name  (Ghidra: map_list_get_friendly_level_name, already named)
// address 0x494f50, size 145 bytes
// name confidence: 0.75   rewrite confidence: 0.4
// evidence: out/phase4/interface_functions.md "Resolves the display-friendly multiplayer level
// name for a map, using the built-in map-list tag when the map is a known one, else its
// filename."; src/game/game_engine_get_default_multiplayer_string.c's confirmed
// text_string_list_get_string(datum_index tag_id, int32_t index) signature; tag_lookup's 'ustr'
// group convention (src/game/game_engine_build_end_game_result_text.c).
// register convention: destination buffer as the recognized stack parameter (param_1), map path
// in EAX (in_EAX), destination capacity in ESI (unaff_ESI).
// blam-cc: EAX -> map_path, ESI -> destination_capacity, stack -> destination
// UNSURE: string_convert_ascii_to_unicode (0x557990, per symbols/review_queue.txt: "computes
// strlen of a narrow source, clamps to the destination capacity in EDI, widens each byte") is
// called here with zero visible arguments in both fallback branches; its destination pointer
// register is not attested anywhere, so it is declared with the (destination, source,
// destination_capacity) shape that description implies and called with this function's own
// (destination, capacity) pair, which is the only destination/capacity this function has.

// Phase-4 review: a known map draws string map_list[index].map_id, not string index (objdump
// 0x494f82); the fallback is string_convert_ascii_to_unicode (EAX dest, EDI bytes, EBX source).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern int32_t map_list_find_known_map_index(char *map_path); // 0x494ff0
extern uint16_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0; blam-cc: ECX -> tag, DX -> index
extern map_list_entry *map_list; // 0x00712dcc
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source); // 0x557990, 8-bit to wide copy
    // blam-cc: EAX -> dest, EDI -> dest_bytes, EBX -> source (review queue name string_convert_ascii_to_unicode)

// blam-cc: EAX -> map_path, ESI -> destination_capacity, stack -> destination
// Resolves the friendly display name for `map_path` into `destination` (capacity
// `destination_capacity` wide characters): if it matches one of the built-in multiplayer maps
// (index 0..18 into the "ui\shell\main_menu\mp_map_list" unicode_string_list tag), copies that
// localized string; otherwise falls back to the map's own filename (the part of the path after
// the last backslash, or the whole path if there is none), converted from ASCII to unicode.
void map_list_get_friendly_level_name(wchar_t *destination, char *map_path,
                                       int32_t destination_capacity)
{
    datum_index map_list_tag;
    int32_t index;
    wchar_t *source;
    char *filename;

    map_list_tag = tag_lookup(0x75737472, (char *)"ui\\shell\\main_menu\\mp_map_list"); // 'ustr'
    index = map_list_find_known_map_index(map_path);
    if (-1 < index && index < 0x13 && index != -1) {
        // objdump 0x494f82..0x494f91: the string index is map_list[index].map_id, not index
        source = (wchar_t *)text_string_list_get_string(map_list_tag, (int16_t)map_list[index].map_id);
        wcsncpy(destination, source, destination_capacity - 1);
        destination[destination_capacity - 1] = L'\0';
        return;
    }

    filename = strrchr(map_path, '\\');
    if (filename != (char *)0) {
        string_convert_ascii_to_unicode((uint16_t *)destination, destination_capacity * 2, filename + 1);
        return;
    }
    string_convert_ascii_to_unicode((uint16_t *)destination, destination_capacity * 2, map_path);
}

#if 0
Original Ghidra decompilation (0x494f50):

void map_list_get_friendly_level_name(wchar_t *param_1)

{
  char *in_EAX;
  int iVar1;
  wchar_t *_Source;
  char *pcVar2;
  int unaff_ESI;

  tag_lookup("ui\\shell\\main_menu\\mp_map_list");
  iVar1 = map_list_find_known_map_index();
  if (((-1 < iVar1) && (iVar1 < 0x13)) && (iVar1 != -1)) {
    _Source = (wchar_t *)text_string_list_get_string();
    _wcsncpy(param_1,_Source,unaff_ESI - 1);
    param_1[unaff_ESI + -1] = L'\0';
    return;
  }
  pcVar2 = _strrchr(in_EAX,0x5c);
  if (pcVar2 != (char *)0x0) {
    FUN_00557990();
    return;
  }
  FUN_00557990();
  return;
}
#endif
