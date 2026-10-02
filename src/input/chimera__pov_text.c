// chimera__pov_text  (Ghidra: already named, Chimera signature)
// address 0x4914c0, size 194 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Builds the display text for a joystick
// POV-hat input, combining the tag gamepad name, axis index, and compass direction."; objdump of
// 0x4914c0..0x491581 confirms all three parameters are plain stack arguments (pov_index,
// direction_index, out_text), that entry 2 of controls_gamepad_names is always used as the "%s"
// pov name (mirroring chimera__button_text's entry 0 and chimera__axis_text's entry 1), and that
// the direction text is looked up from the SAME controls_gamepad_names tag at index
// direction_index + 3, not from the separate axis/pov direction-name tags used elsewhere in this
// module.
// UNSURE: the call to string_format_wide_va_bounded also sets EDX=0xe (14), not exposed by the
// established (dest, format, ...) declaration; see chimera__button_text.c for the same note.
// register convention: pov_index, direction_index, out_text as three ordinary stack parameters

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

extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index tag_lookup(tag_group group, char *path); // cache module, 0x442550
extern uint16_t missing_string_text[];                  // 0x00671fac, L"<missing string>" (the string itself, not a pointer)
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count

// Builds "<gamepad pov name><pov_index + 1> <gamepad direction name>" (e.g. "Pov1 north") into
// out_text (14 wide characters, always null-terminated), using controls_gamepad_names entry 2 as
// the pov name and entry (direction_index + 3) as the direction name.
void chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *pov_name;
    uint16_t *direction_name;
    int16_t direction_entry_index;

    tag_id = tag_lookup(0x75737472, // "ustr"
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names");

    pov_name = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if ((int32_t)list->strings.count > 2) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[2];
            if ((int32_t)entry->string.size > 0) {
                pov_name = (uint16_t *)entry->string.pointer;
                pov_name[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }

    direction_entry_index = direction_index + 3;
    direction_name = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (direction_entry_index >= 0 && direction_entry_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[direction_entry_index];
            if ((int32_t)entry->string.size > 0) {
                direction_name = (uint16_t *)entry->string.pointer;
                direction_name[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }

    string_format_wide_va_bounded(0xe, out_text, (const uint16_t *)L"%s%d %s", pov_name, pov_index + 1, direction_name);
    out_text[0xd] = 0;
}

#if 0
Original Ghidra decompilation (0x4914c0):

void chimera__pov_text(short param_1,short param_2,int param_3)

{
  uint *puVar1;
  int *piVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  undefined **ppuVar7;
  undefined **ppuVar8;

  uVar6 = tag_lookup(
                    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names"
                    );
  iVar5 = DAT_0087bc14;
  ppuVar8 = &PTR_DAT_00671fac;
  if ((uVar6 != 0xffffffff) &&
     (piVar2 = *(int **)((uVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 2 < *piVar2)) {
    iVar3 = piVar2[1];
    uVar4 = *(uint *)(iVar3 + 0x28);
    if (0 < (int)uVar4) {
      ppuVar8 = *(undefined ***)(iVar3 + 0x34);
      *(undefined2 *)((int)ppuVar8 + ((uVar4 & 0xfffffffe) - 2)) = 0;
    }
  }
  param_2 = param_2 + 3;
  ppuVar7 = &PTR_DAT_00671fac;
  if (((uVar6 != 0xffffffff) &&
      (piVar2 = *(int **)((uVar6 & 0xffff) * 0x20 + 0x14 + iVar5), -1 < param_2)) &&
     ((int)param_2 < *piVar2)) {
    puVar1 = (uint *)(piVar2[1] + param_2 * 0x14);
    uVar6 = *puVar1;
    if (0 < (int)uVar6) {
      ppuVar7 = (undefined **)puVar1[3];
      *(undefined2 *)((int)ppuVar7 + ((uVar6 & 0xfffffffe) - 2)) = 0;
    }
  }
  string_format_wide_va_bounded(param_3,L"%s%d %s",ppuVar8,param_1 + 1,ppuVar7);
  *(undefined2 *)(param_3 + 0x1a) = 0;
  return;
}
#endif
