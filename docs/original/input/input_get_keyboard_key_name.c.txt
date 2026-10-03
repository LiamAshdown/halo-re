// input_get_keyboard_key_name  (Ghidra: already named)
// address 0x490e30, size 109 bytes
// name confidence: 0.65   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Fetches the display name string for a given
// keyboard key index from the keyboard-button-names tag."; types/tags.h UnicodeStringList /
// UnicodeStringListString (TagReflexive strings, each a TagDataOffset); types/cache.h
// tag_instance.data at +0x14 confirmed by every other tag walk in this module.
// register convention: key index in AX (in_AX), output buffer in EBX (unaff_EBX)

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index tag_lookup(tag_group group, char *path); // cache module, 0x442550
extern uint16_t missing_string_text[];                  // 0x00671fac, L"<missing string>" (the string itself, not a pointer)

// blam-cc: key_index in EAX, out_name in EBX
// Fetches the display name of keyboard key key_index from the
// controls_keyboard_button_names UnicodeStringList tag into out_name (24 wide characters,
// always null-terminated). Falls back to missing_string_text when the tag is missing or
// key_index is out of range.
void input_get_keyboard_key_name(int16_t key_index, uint16_t *out_name)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    tag_id = tag_lookup(0x75737472, // "ustr"
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_keyboard_button_names");
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (key_index >= 0 && key_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[key_index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x18);
    out_name[0x17] = 0;
}

#if 0
Original Ghidra decompilation (0x490e30):

void input_get_keyboard_key_name(void)

{
  uint *puVar1;
  int *piVar2;
  short in_AX;
  uint uVar3;
  undefined **_Source;
  wchar_t *unaff_EBX;

  uVar3 = tag_lookup(
                    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_keyboard_button_names"
                    );
  _Source = &PTR_DAT_00671fac;
  if (((uVar3 != 0xffffffff) &&
      (piVar2 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), -1 < in_AX)) &&
     ((int)in_AX < *piVar2)) {
    puVar1 = (uint *)(piVar2[1] + in_AX * 0x14);
    uVar3 = *puVar1;
    if (0 < (int)uVar3) {
      _Source = (undefined **)puVar1[3];
      *(undefined2 *)((int)_Source + ((uVar3 & 0xfffffffe) - 2)) = 0;
    }
  }
  _wcsncpy(unaff_EBX,(wchar_t *)_Source,0x18);
  unaff_EBX[0x17] = L'\0';
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
