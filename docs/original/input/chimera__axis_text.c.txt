// chimera__axis_text  (Ghidra: already named, Chimera signature)
// address 0x491340, size 155 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Builds the display text for a joystick axis,
// combining the tag axis name, axis index, and direction suffix."; objdump of
// 0x491340..0x4913da confirms all three parameters are plain stack arguments (axis_index,
// direction, out_text) and that -- like chimera__button_text -- the controls_gamepad_names
// tag's entry 1 (not axis_index) is always used as the "%s" axis name; axis_index only supplies
// the "%d" suffix (+1). The direction->index mapping into input_get_axis_direction_name is
// inverted from input_get_mouse_axis_name's (`sete` here vs `setne` there): direction == 0 asks
// for direction index 1, any other value asks for index 0.
// UNSURE: the call to string_format_wide_va_bounded also sets EDX=0x18 (24), not exposed by the
// established (dest, format, ...) declaration; see chimera__button_text.c for the same note.
// register convention: axis_index, direction, out_text as three ordinary stack parameters

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
extern void input_get_axis_direction_name(int16_t direction_index, uint16_t *out_name); // this module, 0x491180
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count

// Builds "<gamepad axis name><axis_index + 1> <direction>" (e.g. "Axis1 +") into out_text (25
// wide characters, always null-terminated), using entry 1 of the controls_gamepad_names tag as
// the axis name.
void chimera__axis_text(int16_t axis_index, uint8_t direction, uint16_t *out_text)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;
    uint16_t direction_name[9];

    tag_id = tag_lookup(0x75737472, // "ustr"
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names");
    input_get_axis_direction_name(direction == 0 ? 1 : 0, direction_name);

    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if ((int32_t)list->strings.count > 1) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[1];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    string_format_wide_va_bounded(0x18, out_text, (const uint16_t *)L"%s%d %s", source, axis_index + 1, direction_name);
    out_text[0x18] = 0;
}

#if 0
Original Ghidra decompilation (0x491340):

void chimera__axis_text(short param_1,undefined4 param_2,int param_3)

{
  int *piVar1;
  int iVar2;
  uint uVar3;
  undefined **ppuVar4;
  undefined1 local_34 [52];

  uVar3 = tag_lookup(
                    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names"
                    );
  input_get_axis_direction_name();
  ppuVar4 = &PTR_DAT_00671fac;
  if ((uVar3 != 0xffffffff) &&
     (piVar1 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 1 < *piVar1)) {
    iVar2 = piVar1[1];
    uVar3 = *(uint *)(iVar2 + 0x14);
    if (0 < (int)uVar3) {
      ppuVar4 = *(undefined ***)(iVar2 + 0x20);
      *(undefined2 *)((int)ppuVar4 + ((uVar3 & 0xfffffffe) - 2)) = 0;
    }
  }
  string_format_wide_va_bounded(param_3,L"%s%d %s",ppuVar4,param_1 + 1,local_34);
  *(undefined2 *)(param_3 + 0x30) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
