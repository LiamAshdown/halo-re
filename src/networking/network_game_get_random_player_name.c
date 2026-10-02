// network_game_get_random_player_name  (Ghidra: network_game_get_random_player_name, already
// named)
// address 0x4dea80, size 112 bytes
// name confidence: 0.7   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Looks up the 'ui\random_player_names' tag and,
// if it has entries, returns a randomly selected default player name; otherwise returns the
// built-in fallback name string." tag_instances (0x0087bc14) and its `(index*0x20+0x14)`
// definition-pointer idiom match the same pattern already established throughout src/ai (e.g.
// actor_apply_unit_definition_properties.c); neither `tag_instance` nor the unicode_string_list
// tag body has a declared type anywhere in types/tags.h, so this rewrite keeps the raw-offset
// form rather than inventing one.
// UNSURE: text_string_list_get_string's index argument is elided at this call site (matching the
// widespread "tag_lookup/text_string_list_get_string pair called with no visible index" issue
// already documented in src/game/game_engine_build_end_game_result_text.c); reconstructed as the
// LCG-derived random value, modulo the list's own count, though that modulo is not directly
// visible in the decompile either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern wchar_t empty_string; // 0x00660c34, the built-in fallback string
extern void *tag_instances;      // 0x0087bc14, UNSURE type: raw tag_instance array, see header
extern random_seed effect_random_seed; // 0x00719cd4, types/math.h

extern int32_t tag_lookup(const char *path); // 0x442550, cache module
extern wchar_t *text_string_list_get_string(int32_t tag_index, int32_t string_index); // 0x5578c0

wchar_t *network_game_get_random_player_name(void)
{
    uint32_t tag_id;
    void *definition;

    tag_id = tag_lookup("ui\\random_player_names");
    if (tag_id != 0xffffffff) {
        definition = *(void **)((uint8_t *)tag_instances + (tag_id & 0xffff) * 0x20 + 0x14);
        if (definition != 0 && *(int32_t *)definition != 0) {
            effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
            return text_string_list_get_string((int32_t)tag_id, 0); // UNSURE: index elided
        }
    }
    return &empty_string;
}

#if 0
Original Ghidra decompilation (0x4dea80):

undefined * network_game_get_random_player_name(void)

{
  uint uVar1;
  undefined *puVar2;

  uVar1 = tag_lookup("ui\\random_player_names");
  if ((uVar1 != 0xffffffff) && (*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) != 0)) {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    puVar2 = (undefined *)text_string_list_get_string();
    return puVar2;
  }
  return &DAT_00660c34;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
