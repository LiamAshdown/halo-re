// chimera__button_text  (Ghidra: already named, Chimera signature)
// address 0x491270, size 108 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Builds the display text for a joystick
// button using the gamepad-names tag."; objdump of 0x491270..0x4912db confirms button_index
// arrives as the single stack argument ([esp+4] at entry), the output buffer in EBX, and (unlike
// the other get_*_name functions in this module) the controls_gamepad_names tag's first entry
// (index 0, not button_index) is always used as the "%s" -- the button number itself is only the
// "%d" appended after it.
// UNSURE: the call to string_format_wide_va_bounded also sets EDX=0x17 (23, the same
// bound as this module's other 24-wide-character buffers), which the established
// (dest, format, ...) declaration used across the codebase does not expose as a parameter.
// register convention: button_index on the stack; out_text in EBX (unaff_EBX)

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
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count

// blam-cc: button_index on the stack, out_text in EBX
// Builds "<gamepad button name><button_index + 1>" (e.g. "Button1") into out_text (24 wide
// characters, always null-terminated), using entry 0 of the controls_gamepad_names tag as the
// name.
void chimera__button_text(int16_t button_index, uint16_t *out_text)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    tag_id = tag_lookup(0x75737472, // "ustr"
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names");
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if ((int32_t)list->strings.count > 0) {
            entry = (UnicodeStringListString *)list->strings.pointer;
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    string_format_wide_va_bounded(0x17, out_text, (const uint16_t *)L"%s%d", source, button_index + 1);
    out_text[0x17] = 0;
}

#if 0
Original Ghidra decompilation (0x491270):

void chimera__button_text(void)

{
  int *piVar1;
  uint *puVar2;
  uint uVar3;
  int unaff_EBX;

  uVar3 = tag_lookup(
                    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names"
                    );
  if ((uVar3 != 0xffffffff) &&
     (piVar1 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *piVar1)) {
    puVar2 = (uint *)piVar1[1];
    uVar3 = *puVar2;
    if (0 < (int)uVar3) {
      *(undefined2 *)((puVar2[3] - 2) + (uVar3 & 0xfffffffe)) = 0;
    }
  }
  string_format_wide_va_bounded();
  *(undefined2 *)(unaff_EBX + 0x2e) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
