// game_engine_get_default_multiplayer_string  (Ghidra: FUN_0045ce90; named for what it does --
// looks up one localized string out of the "ui\multiplayer_game_text" unicode_string_list tag)
// address 0x45ce90, size 68 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md summary ("Looks up a default string from the
// ui\multiplayer_game_text string list, or returns an empty fallback string if the tag can't be
// found"); disassembly of the two call sites inside game_engine_build_end_game_result_text
// (0x45d39d/0x45d3dd, `mov edx,0x3f` / `mov edx,0x40` immediately before falling through into
// this function with EDX untouched) shows the string index really does arrive live in EDX/DX,
// confirming the register this function's own decompile never names because its body never
// re-reads it directly (it only forwards it to text_string_list_get_string).
// register convention: string index in DX (in_DX, inferred -- see UNSURE below).
//   // blam-cc: DX -> string_index
// UNSURE: this function's own decompilation shows zero parameters at all (`FUN_0045ce90(void)`);
// the DX register is never named inside its body because it is only ever consumed by the tail
// call to text_string_list_get_string(tag_id, index), whose calling convention (established
// elsewhere as ECX=tag_id, DX=index) is what pins the missing argument here.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "fn_game.h"
#include <wchar.h>

extern datum_index tag_lookup(tag_group group, char *path); // cache module, 0x442550
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0
    // blam-cc: ECX -> tag_id, DX -> index

extern wchar_t empty_string; // 0x00660c34, a single L'\0'

// blam-cc: DX -> string_index
// Returns string `string_index` out of the "ui\multiplayer_game_text" unicode_string_list tag,
// or a pointer to a shared empty string if that tag isn't loaded.
wchar_t *game_engine_get_default_multiplayer_string(int16_t string_index)
{
    datum_index tag_id;

    tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text"); // 'ustr' unicode_string_list
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, string_index);
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
