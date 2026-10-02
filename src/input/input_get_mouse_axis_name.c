// input_get_mouse_axis_name  (Ghidra: already named)
// address 0x491010, size 163 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: phase-4 review, body checked against `objdump -d` of 0x491010..0x4910b2. Reuses the
// controls_mouse_button_names tag (entries 8+ are the axis names, following the 8 button
// entries) and appends a direction suffix from input_get_axis_direction_name. objdump shows
// axis_index arriving as a normal stack argument ([esp+4] at entry) and a second stack argument
// ([esp+8], a direction byte) that Ghidra's pseudo-C dropped entirely; that direction byte is
// normalized to 0/1 (`setne al`) and passed to input_get_axis_direction_name in EAX, matching
// that function's own in_AX register convention. The output buffer arrives in ESI.
// register convention: axis_index and direction on the stack; out_name in ESI (unaff_ESI)

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

// blam-cc: axis_index and direction on the stack, out_name in ESI
// Builds the display name for mouse axis axis_index (0..2) by combining the tag-provided axis
// name (controls_mouse_button_names entries 8+) with a trailing direction suffix ("+"/"-"),
// into out_name (33 wide characters, always null-terminated).
void input_get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;
    int16_t lookup_index;
    uint16_t direction_name[9];

    tag_id = tag_lookup(0x75737472, // "ustr"
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_mouse_button_names");
    lookup_index = axis_index + 8;
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (lookup_index >= 0 && lookup_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[lookup_index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x21);
    input_get_axis_direction_name(direction != 0, direction_name);
    wcscat((wchar_t *)out_name, L" ");
    wcscat((wchar_t *)out_name, (const wchar_t *)direction_name);
    out_name[0x20] = 0;
}

#if 0
Original Ghidra decompilation (0x491010):

void input_get_mouse_axis_name(short param_1)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  undefined **_Source;
  wchar_t *unaff_ESI;
  wchar_t local_14 [10];

  uVar3 = tag_lookup(
                    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_mouse_button_names"
                    );
  param_1 = param_1 + 8;
  _Source = &PTR_DAT_00671fac;
  if (((uVar3 != 0xffffffff) &&
      (piVar2 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), -1 < param_1)) &&
     ((int)param_1 < *piVar2)) {
    puVar1 = (uint *)(piVar2[1] + param_1 * 0x14);
    uVar3 = *puVar1;
    if (0 < (int)uVar3) {
      _Source = (undefined **)puVar1[3];
      *(undefined2 *)((int)_Source + ((uVar3 & 0xfffffffe) - 2)) = 0;
    }
  }
  _wcsncpy(unaff_ESI,(wchar_t *)_Source,0x21);
  input_get_axis_direction_name();
  _wcscat(unaff_ESI,L" ");
  _wcscat(unaff_ESI,local_14);
  unaff_ESI[0x20] = L'\0';
  return;
}

Disassembly (objdump -d, 0x491010..0x4910b2), confirming the second (direction) stack argument:

00491010 <input_get_mouse_axis_name>:
  491024: mov    0x24(%esp),%ecx     ; ecx = axis_index ([esp+4] at entry)
  49102b: add    $0x8,%ecx
  ...
  49107c: mov    cl,[esp+0x30]       ; cl = direction ([esp+8] at entry)
  491082: test   cl,cl
  491084: setne  al                 ; eax = (direction != 0)
  491087: lea    ebx,[esp+0x14]      ; direction_name buffer
  49108b: call   0x491180            ; input_get_axis_direction_name(eax, ebx)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
