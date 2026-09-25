// input_print_bound_controls  (Ghidra: input_print_bound_controls, already named)
// address 0x48bea0, size 2352 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: objdump-confirmed field addresses match the binding-table externs also used by
//   input_apply_control_binding.c / input_refresh_last_used_binding.c. console_printf_verbose
//   (0x496a80, already rewritten in src/interface/) takes a ColorARGB* color in EAX (NULL here,
//   confirmed by `xor eax,eax` immediately before every call site in this function) and the
//   format plus varargs on the stack; wcslen is wcslen (see
//   src/input/input_keyboard_key_name_to_index.c). The repeated "look up a controls_* UnicodeStringList
//   tag, take one entry, force-null-terminate its last even byte" block and the repeated
//   "narrow a wide buffer to ASCII, substituting a space for any non-ASCII code unit" block are
//   each factored into one static helper here (lookup_named_string / narrow_copy); both keep the
//   exact per-call parameters (tag path, entry index, buffer length) the binary uses, including
//   the per-call details (checked line by line against objdump 0x48bea0..0x48c7ef in the
//   phase-4 review, which corrected three drifts of the first rewrite):
//     - gamepad axis lines read controls_axis_direction_names entry 0 for [axis][0] and entry
//       1 for [axis][1] (0x48c3f6 / 0x48c575 test count > 0 / > 1), and [axis][0] prints first;
//     - the gamepad generic label reads controls_gamepad_names entry 0 for a button and entry 1
//       for an axis (the number is appended separately via "%s%d");
//     - the axis scan is bounded by the device axis_count (+0x234) and the POV scan by
//       pov_count (+0x23c); only the button scan uses button_count (+0x238);
//     - mouse axis [axis][0] prints first, named with direction argument 1, then [axis][1]
//       with 0;
//     - every wide buffer gets the binary terminator before narrowing (mouse button and
//       gamepad button [0x17], direction name [8], axis text [0x18]).
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "cache.h"
#include "input.h"

#include <wchar.h>
#include <string.h>

extern input_device input_devices[8];            // 0x006b1868
extern int32_t joystick_slot_devices[4];         // 0x006b2ce8

extern int16_t keyboard_bindings[k_control_keyboard_key_count];                    // 0x00710330
extern int16_t mouse_button_bindings[k_control_mouse_button_count];                // 0x0071040a
extern int16_t mouse_axis_bindings[k_control_mouse_axis_count][2];                 // 0x0071041a
extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x00710426
extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2];  // 0x00710536
extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x00710736

extern char input_action_names[k_input_action_count][0x10]; // 0x0065b730

extern tag_instance *tag_instances;                            // 0x0087bc14
extern datum_index tag_lookup(tag_group group, char *path);    // cache module, 0x442550
extern uint16_t missing_string_text[];                  // 0x00671fac, L"<missing string>" (the string itself, not a pointer)

extern void input_get_keyboard_key_name(int16_t key_index, uint16_t *out_name);           // this module, 0x490e30
extern void input_get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name); // this module, 0x491010
extern void chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text);     // this module, 0x4914c0
extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...);  // 0x557910
extern void console_printf_verbose(ColorARGB *color, char *format, ...);                   // interface module, 0x496a80

#define k_gamepad_names_tag_path \
    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names"
#define k_mouse_button_names_tag_path \
    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_mouse_button_names"
#define k_axis_direction_names_tag_path \
    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_axis_direction_names"

// Looks up the "ustr" tag at tag_path and returns UnicodeStringListString entry[index]'s text,
// or missing_string_text if the tag is missing, index is out of range, or the entry is empty.
// Matches the inline tag walk this function repeats for the mouse-button and both gamepad name
// lookups: on a non-empty entry it also force-null-terminates the string's last even byte in
// place (the same mutation input_get_mouse_axis_name.c and input_get_keyboard_key_name.c already
// perform for the same tag family).
static uint16_t *lookup_named_string(const char *tag_path, int32_t index)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    source = missing_string_text;
    tag_id = tag_lookup(0x75737472, // "ustr"
        (char *)tag_path);
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (index >= 0 && index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    return source;
}

// Narrows up to max_len-1 wide characters of `wide` into `out` (ASCII, always null-terminated),
// substituting a space for any code unit whose high byte is nonzero. Leaves `out` untouched if
// the wide string is max_len characters or longer, matching the binary's own (never-hit in
// practice, given the buffer sizes used) behaviour.
static void narrow_copy(char *out, const uint16_t *wide, uint32_t max_len)
{
    uint32_t length;
    uint32_t i;

    length = (uint32_t)wcslen((const wchar_t *)wide);
    if (length < max_len) {
        for (i = 0; i < length; i++) {
            out[i] = ((const uint8_t *)wide)[i * 2 + 1] == 0 ? ((const char *)wide)[i * 2] : ' ';
        }
        out[i] = '\0';
    }
}

// Fills action_name (16 bytes) with the display name of action_index, or "none" when unbound.
static void action_name_copy(char *action_name, int16_t action_index)
{
    const char *name = (action_index == (int16_t)k_input_unbound) ? "none" : input_action_names[action_index];
    strncpy(action_name, name, 0x10);
}

// Console/debug routine that lists every currently bound keyboard, mouse, and joystick input
// alongside the game control it triggers, one console_printf_verbose line per bound input.
void input_print_bound_controls(void)
{
    int16_t key_index;
    int16_t button_index;
    int16_t axis_index;
    int32_t slot;
    int32_t device;
    int32_t button_count;
    int32_t axis_count;
    int32_t pov_count;
    int32_t i;
    int32_t octant;
    char action_name[0x10];
    uint16_t wide_name[0x21];
    uint16_t wide_name2[0x21];
    wchar_t formatted[0x19];
    char ascii[0x21];

    // Keyboard
    for (key_index = 0; key_index < (int16_t)k_control_keyboard_key_count; key_index++) {
        if (keyboard_bindings[key_index] != k_input_unbound) {
            action_name_copy(action_name, keyboard_bindings[key_index]);
            input_get_keyboard_key_name(key_index, wide_name);
            narrow_copy(ascii, wide_name, 0x18);
            console_printf_verbose(0, "%s key bound to %s", ascii, action_name);
        }
    }

    // Mouse buttons
    for (button_index = 0; button_index < k_control_mouse_button_count; button_index++) {
        if (mouse_button_bindings[button_index] != k_input_unbound) {
            action_name_copy(action_name, mouse_button_bindings[button_index]);
            // 0x48c019: wcsncpy(buffer, name, 0x18) and a terminator at [0x17] before narrowing
            wcsncpy((wchar_t *)wide_name, (const wchar_t *)lookup_named_string(k_mouse_button_names_tag_path, button_index), 0x18);
            wide_name[0x17] = 0;
            narrow_copy(ascii, wide_name, 0x18);
            console_printf_verbose(0, "%s mouse button bound to %s", ascii, action_name);
        }
    }

    // Mouse axes: [axis][0] first, named with direction argument 1, then [axis][1] with 0
    // (0x48c090: [edi-2] then push 1; 0x48c135..: [edi] then push 0)
    for (axis_index = 0; axis_index < k_control_mouse_axis_count; axis_index++) {
        if (mouse_axis_bindings[axis_index][0] != k_input_unbound) {
            action_name_copy(action_name, mouse_axis_bindings[axis_index][0]);
            input_get_mouse_axis_name(axis_index, 1, wide_name);
            narrow_copy(ascii, wide_name, 0x21);
            console_printf_verbose(0, "%s mouse axis bound to %s", ascii, action_name);
        }
        if (mouse_axis_bindings[axis_index][1] != k_input_unbound) {
            action_name_copy(action_name, mouse_axis_bindings[axis_index][1]);
            input_get_mouse_axis_name(axis_index, 0, wide_name);
            narrow_copy(ascii, wide_name, 0x21);
            console_printf_verbose(0, "%s mouse axis bound to %s", ascii, action_name);
        }
    }

    // Gamepads
    for (slot = 0; slot < k_control_gamepad_count; slot++) {
        device = joystick_slot_devices[slot];
        button_count = 0;
        axis_count = 0;
        pov_count = 0;
        if (device != -1) {
            button_count = input_devices[device].button_count;
            axis_count = input_devices[device].axis_count;
            pov_count = input_devices[device].pov_count;
        }

        for (i = 0; i < button_count; i++) {
            if (gamepad_button_bindings[slot][i] != k_input_unbound) {
                action_name_copy(action_name, gamepad_button_bindings[slot][i]);
                string_format_wide_va_bounded((wchar_t *)wide_name, L"%s%d",
                                               lookup_named_string(k_gamepad_names_tag_path, 0), i + 1);
                wide_name[0x17] = 0;
                narrow_copy(ascii, wide_name, 0x18);
                console_printf_verbose(0, "%s on gamepad %d bound to %s", ascii, slot, action_name);
            }
        }

        // axes: [axis][0] first with direction name entry 0, then [axis][1] with entry 1; the
        // axis label is gamepad name entry 1 (0x48c3f6 / 0x48c575)
        for (i = 0; i < axis_count; i++) {
            if (gamepad_axis_bindings[slot][i][0] != k_input_unbound) {
                action_name_copy(action_name, gamepad_axis_bindings[slot][i][0]);
                wcsncpy((wchar_t *)wide_name2, (const wchar_t *)lookup_named_string(k_axis_direction_names_tag_path, 0), 9);
                wide_name2[8] = 0;
                string_format_wide_va_bounded(formatted, L"%s%d %s",
                                               lookup_named_string(k_gamepad_names_tag_path, 1), i + 1, wide_name2);
                formatted[0x18] = 0;
                narrow_copy(ascii, (const uint16_t *)formatted, 0x19);
                console_printf_verbose(0, "%s on gamepad %d bound to %s", ascii, slot, action_name);
            }
            if (gamepad_axis_bindings[slot][i][1] != k_input_unbound) {
                action_name_copy(action_name, gamepad_axis_bindings[slot][i][1]);
                wcsncpy((wchar_t *)wide_name2, (const wchar_t *)lookup_named_string(k_axis_direction_names_tag_path, 1), 9);
                wide_name2[8] = 0;
                string_format_wide_va_bounded(formatted, L"%s%d %s",
                                               lookup_named_string(k_gamepad_names_tag_path, 1), i + 1, wide_name2);
                formatted[0x18] = 0;
                narrow_copy(ascii, (const uint16_t *)formatted, 0x19);
                console_printf_verbose(0, "%s on gamepad %d bound to %s", ascii, slot, action_name);
            }
        }

        for (i = 0; i < pov_count; i++) {
            for (octant = 0; octant < k_control_gamepad_pov_direction_count; octant++) {
                if (gamepad_pov_bindings[slot][i][octant] != k_input_unbound) {
                    action_name_copy(action_name, gamepad_pov_bindings[slot][i][octant]);
                    chimera__pov_text((int16_t)i, (int16_t)octant, wide_name);
                    narrow_copy(ascii, wide_name, 0xe);
                    console_printf_verbose(0, "%s on gamepad %d bound to %s", ascii, slot, action_name);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x48bea0): see tools/pack.py 0x48bea0 for the full 250-line
listing (elided here for length). Summary of the structure this rewrite follows, section by
section, with the exact addresses referenced:

- keyboard loop: &DAT_00710330 .. 0x71040a, input_get_keyboard_key_name, "%s key bound to %s"
- mouse button loop: &DAT_0071040a .. 0x71041a, inline "controls_mouse_button_names" tag walk
  indexed by button index, "%s mouse button bound to %s"
- mouse axis loop: &DAT_0071041a/&DAT_0071041c .. 0x710428 (direction 2 then direction 1 per
  axis), input_get_mouse_axis_name, "%s mouse axis bound to %s"
- per-gamepad-slot loop (local_1c4 0..3), reading button_count/axis_count/pov_count from
  input_devices via joystick_slot_devices, but looping the axis and POV scans to button_count:
    - buttons: &DAT_00710426, inline "controls_gamepad_names" tag walk at entry 0, "%s%d" via
      string_format_wide_va_bounded, "%s on gamepad %d bound to %s"
    - axes (direction 2 via &DAT_00710538, then direction 1 via &DAT_00710536): inline
      "controls_axis_direction_names" tag walk always at entry 0, inline "controls_gamepad_names"
      tag walk at entry 1, "%s%d %s" via string_format_wide_va_bounded, same print format
    - povs: &DAT_00710736, chimera__pov_text, same print format

Every action-name lookup (if bound-action != 0x7fff, strncpy from either the literal none or
DAT_0065b730 + action*0x10, 0x10 bytes) is action_name_copy() here; the dead
already-known-not-0x7fff branch that also tests for 0x7fff is folded into one ternary. Every
wcslen-then-narrow-to-ASCII block (copy while length is below the buffer size, space out any
non-ASCII code unit, null terminate) is narrow_copy() here.
#endif
