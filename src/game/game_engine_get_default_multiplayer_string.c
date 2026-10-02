// game_engine_get_default_multiplayer_string  (Ghidra: FUN_0045ce90; symbols/review_queue_mech.txt names it get_place_string)
// address 0x45ce90, size 68 bytes
// VERIFIED against disassembly 0x45ce90..0x45ced4 (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.95
// evidence: the body reads the scoreboard entry's place (+0x18) and looks up string 0x24 + min(place & 0x7f, 15) of the
// "ui\multiplayer_game_text" unicode_string_list tag ("1st", "2nd", ...); the callers (0x45d3bf, 0x45d3ff) pass a
// pointer to their local scoreboard_entry copy in EAX (`lea eax,[esp+0x18]`) and use the result as the first %s of the player result line.
// FIXED 2026-09-30: the earlier rewrite was a generic "lookup string N in DX" helper, which is NOT what this address does (the
// "mov edx,0x3f / 0x40" the draft cited are direct text_string_list_get_string calls inside the caller). The function is really
// get_place_string; the historical name is kept only so existing references keep resolving. The callers that wanted a
// generic index lookup now use a file-local helper.
// register convention: scoreboard_entry pointer in EAX; the final text_string_list_get_string is a tail call (ECX tag, EDX index).
//   // blam-cc: EAX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern datum_index tag_lookup(tag_group group, char *path); // cache module, 0x442550, blam-cc: EDI group
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0
    // blam-cc: ECX -> tag_id, DX -> index

extern wchar_t empty_string; // 0x00660c34, a single L'\0'

// blam-cc: EAX -> entry
// Returns the localized place string (0x24 + clamped place) for a scoreboard entry, or a pointer to a shared empty string if the
// "ui\multiplayer_game_text" tag isn't loaded.
wchar_t *game_engine_get_default_multiplayer_string(const scoreboard_entry *entry)
{
    datum_index tag_id;
    int32_t place = entry->place & 0x7f;    // and eax,0x7f
    int32_t index = (place > 0xf) ? 0xf : place; // cmp eax,0xf / jg

    tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text"); // 'ustr' unicode_string_list
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, (int16_t)(index + 0x24));
}

#if 0
Original Ghidra decompilation (0x45ce90), from tools/pack.py 0x45ce90:

undefined * FUN_0045ce90(void)

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
