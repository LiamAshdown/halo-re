// hud_get_message_string  (Ghidra: hud_get_message_string, already named)
// address 0x4aa3f0, size 67 bytes
// name confidence: 0.5 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.9
// phase-4 review: checked against objdump 0x4aa3f0..0x4aa432; the tail jump to 0x5578c0 passes
// the string list tag in ECX and the index in DX (the first rewrite dropped both).
// evidence: types/interface.h hud_globals_tag_data (0x0071941c); types/tags.h HUDGlobals::
// item_message_text is the TagDependency at offset 0x94, whose tag_id sub-field (the last 4
// bytes of the 16 byte TagDependency) lands exactly on the `+0xa0` read here; types/cache.h
// tag_instance for the tag lookup idiom.
// register convention: message index in EDX (in_EDX, unresolved register read).
//   // blam-cc: message_index -> EDX

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t *empty_wide_string_pointer; // 0x00692d7c, UNSURE: fallback value

extern uint16_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0; blam-cc: ECX, DX

// blam-cc: message_index -> EDX
// Resolves a weapon-HUD message index to its localized string out of the current hud_globals
// tag's item_message_text string list, or the shared empty string if the index is out of range.
uint16_t *hud_get_message_string(int32_t message_index)
{
    HUDGlobals *hud_globals = (HUDGlobals *)hud_globals_tag_data;
    int32_t string_list_tag_id = *(int32_t *)&hud_globals->item_message_text.tag_id;

    if (string_list_tag_id != -1) {
        int32_t *string_list_tag_data = (int32_t *)tag_instances[string_list_tag_id & 0xffff].data;
        if (string_list_tag_data != 0 && message_index > -1 && message_index < *string_list_tag_data) {
            return text_string_list_get_string((datum_index)string_list_tag_id, (int16_t)message_index); // tail jump, ECX tag, DX index
        }
    }
    return empty_wide_string_pointer;
}

#if 0
Original Ghidra decompilation (0x4aa3f0):

undefined * hud_get_message_string(void)

{
  int *piVar1;
  undefined *puVar2;
  int in_EDX;

  if ((((*(uint *)(DAT_0071941c + 0xa0) != 0xffffffff) &&
       (piVar1 = *(int **)((*(uint *)(DAT_0071941c + 0xa0) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
       piVar1 != (int *)0x0)) && (-1 < in_EDX)) && (in_EDX < *piVar1)) {
    puVar2 = (undefined *)text_string_list_get_string();
    return puVar2;
  }
  return PTR_DAT_00692d7c;
}
#endif
