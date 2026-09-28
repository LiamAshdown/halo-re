exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())

EXT.update({
    'hud_text_message_queue': 'extern growable_array hud_text_message_queue; // 0x006b37e8',
    'GlobalFree': 'extern void *GlobalFree(void *memory); // 0x0063a0bc IAT',
    'hud_text_message_cycle_state_00719230': 'extern int32_t hud_text_message_cycle_state_00719230; // 0x00719230, compared as a dword',
    'chimera__main_menu_music': 'extern void chimera__main_menu_music(uint8_t finalize_render_frame); // 0x4921a0, blam-cc: BL',
    'ui_restart_saved_game': 'extern uint32_t ui_restart_saved_game(void); // 0x4a1110',
    'saved_game_load_checkpoint_by_name': 'extern uint8_t saved_game_load_checkpoint_by_name(char *name); // 0x5391a0',
    'string_format_wide_va_bounded': 'extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count',
    'input_bind_scan_set_active': 'extern void input_bind_scan_set_active(uint8_t enable_scan); // 0x48b6b0, blam-cc: AL',
    'controls_gamepad_lists_refresh': 'extern void controls_gamepad_lists_refresh(widget_instance *screen); // 0x4b55d0, blam-cc: ECX',
    'controls_gamepad_widget_nodes_collect': 'extern void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen); // 0x4b5560, blam-cc: EAX out, ECX screen',
    'video_options_menu_populate': 'extern void video_options_menu_populate(uint8_t *context, uint8_t *settings); // 0x4baec0',
    'chimera__gamma': 'extern void chimera__gamma(void); // 0x5227a0',
    'chat_state_006953e8': 'extern int32_t chat_state_006953e8; // 0x006953e8, UNSURE identity',
    'controls_menu_list_mode': 'extern uint8_t controls_menu_list_mode; // 0x00719445, UNSURE name',
    'controls_selected_device': 'extern int32_t controls_selected_device; // 0x006953ec',
    'controls_device_sensitivity': ('extern uint8_t controls_device_sensitivity_a[]; // 0x007157d4, indexed by device; UNSURE name\n'
                                    'extern uint8_t controls_device_sensitivity_b[]; // 0x007157d8, indexed by device; UNSURE name'),
    'ui_flag_007196d1': 'extern uint8_t ui_flag_007196d1; // 0x007196d1, UNSURE (cleared by 0x4bb290, gates the gamma apply in 0x4bb300)',
    'rasterizer_gamma_exponent': 'extern int32_t rasterizer_gamma_exponent; // 0x0071d1e0',
    'network_host_name_field_00719238': 'extern uint16_t network_host_name_field_00719238[32]; // 0x00719238',
    'network_game_options': ('extern uint32_t network_game_option_a_00719210; // 0x00719210, TYPES-GAP\n'
                             'extern uint32_t network_game_option_b_00719214; // 0x00719214, TYPES-GAP'),
    'widget_instance_close_and_restore_previous': 'extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, blam-cc: EAX',
})

GAME = ('    uint8_t *game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8\n'
        '                  : network_client != 0 ? network_client + 0xb14 : 0;\n')
FIRST_LIST = ('static widget_instance *first_list_child(widget_instance *widget)\n{\n'
              '    widget_instance *child = widget->first_child;\n\n'
              '    while (child != 0 && child->widget_type != 2) {\n        child = child->next_sibling;\n    }\n    return child;\n}\n')

SPECS = [
 (0x4a39e0, 129, "for a selected profile: the first spinner list (type 2) under the grandparent's first child gives a selection clamped to 0..4 stored as profile byte +0xfc0; the words of 0x00719210 / 0x00719214 go to +0x1002 / +0x1004. Returns 1, or 0 without a profile. (No spinner list: the binary reads through null; so does this.)",
  ['selected_saved_item', 'saved_item_working_copy', 'network_game_options'],
  PROFILE + '    int16_t selection;\n\n    if (profile == 0) {\n        return 0;\n    }\n'
  '    selection = first_list_child(widget->parent->parent->first_child)->selection_index;\n'
  '    if (selection < 0) {\n        selection = 0;\n    } else if (selection > 4) {\n        selection = 4;\n    }\n'
  '    profile[0xfc0] = (uint8_t)selection;\n    *(uint16_t *)(profile + 0x1002) = (uint16_t)network_game_option_a_00719210;\n'
  '    *(uint16_t *)(profile + 0x1004) = (uint16_t)network_game_option_b_00719214;\n    return 1;\n', FIRST_LIST),
 (0x4a3d40, 74, 'resets the hud text message queue (first two dwords -1, GlobalFree the data and clear it); a positive message cycle state (0x00719230) restarts the main menu music with (state == 2); the state becomes 0; returns 1.',
  ['hud_text_message_queue', 'GlobalFree', 'hud_text_message_cycle_state_00719230', 'chimera__main_menu_music'],
  '    hud_text_message_queue.element_size = -1;\n    hud_text_message_queue.count = -1;\n'
  '    if (hud_text_message_queue.data != 0) {\n        GlobalFree(hud_text_message_queue.data);\n        hud_text_message_queue.data = 0;\n    }\n'
  '    if (hud_text_message_cycle_state_00719230 > 0) {\n        chimera__main_menu_music((uint8_t)(hud_text_message_cycle_state_00719230 == 2));\n    }\n'
  '    hud_text_message_cycle_state_00719230 = 0;\n    return 1;\n'),
 (0x4a4110, 124, "finds the widget among its parent's first four children (not found: returns 1). When the parent's committed selection (+0x3c) already equals that position: a position below 4 becomes the pending difficulty (when not negative) with sound 2, then the saved game restarts (0x4a1110). The parent's committed selection becomes the position; returns 1.",
  ['pending_difficulty', 'widget_play_sound_effect', 'ui_restart_saved_game'],
  '    widget_instance *parent = widget->parent;\n    widget_instance *child = parent->first_child;\n    int16_t *committed = (int16_t *)((uint8_t *)parent + 0x3c);\n'
  '    int32_t i;\n\n    for (i = 0; child != widget; i++) {\n        if (i + 1 >= 4) {\n            return 1;\n        }\n        child = child->next_sibling;\n    }\n'
  '    if (*committed == i) {\n        if (i < 4) {\n            if (i >= 0) {\n                pending_difficulty = (int16_t)i;\n            }\n'
  '            widget_play_sound_effect(2);\n        }\n        ui_restart_saved_game();\n    }\n    *committed = (int16_t)i;\n    return 1;\n'),
 (0x4a4580, 72, "formats checkpoints\\\\<name> from the data (+0x48) of the ui list item at the widget's committed selection into the pending delete name (0x00718fd0); returns 1. (Out of range: the binary formats from address 0x48; so does this.)",
  ['ui_list_current', 'ui_lists', 'pending_delete_saved_game_name_00718fd0'],
  '    uint8_t *data = (uint8_t *)list_item_data(*(int16_t *)((uint8_t *)widget + 0x3c));\n\n'
  '    sprintf(pending_delete_saved_game_name_00718fd0, "checkpoints\\\\%s", (char *)(data + 0x48)); // format at 0x0066a4dc\n    return 1;\n', LIST_DATA),
 (0x4a45f0, 83, "formats checkpoints\\\\<name> from the data (+0x48) of the ui list item at the widget's committed selection into a local and returns the checkpoint load result (0x5391a0).",
  ['ui_list_current', 'ui_lists', 'saved_game_load_checkpoint_by_name'],
  '    uint8_t *data = (uint8_t *)list_item_data(*(int16_t *)((uint8_t *)widget + 0x3c));\n    char name[0x40];\n\n'
  '    sprintf(name, "checkpoints\\\\%s", (char *)(data + 0x48)); // format at 0x0066a4dc\n    return saved_game_load_checkpoint_by_name(name);\n', LIST_DATA),
 (0x4a4af0, 110, "when the parent is the grandparent's first child, opens the virtual keyboard on 0x00719238 (0x40 characters, field kind 0xd) and on success sets the edit field to 4 and returns 1; when the parent is that first child's next sibling, the subname (0x007191f0, 0x12, 0xc) with edit field 0; otherwise 0.",
  ['network_host_name_field_00719238', 'network_host_subname_007191f0', 'network_host_edit_field_00719410', 'virtual_keyboard_open'],
  '    widget_instance *first = widget->parent->parent->first_child;\n\n'
  '    if (first == widget->parent && virtual_keyboard_open(network_host_name_field_00719238, 0x40, 0xd) != 0) {\n'
  '        network_host_edit_field_00719410 = 4;\n        return 1;\n    }\n'
  '    if (first->next_sibling == widget->parent && virtual_keyboard_open(network_host_subname_007191f0, 0x12, 0xc) != 0) {\n'
  '        network_host_edit_field_00719410 = 0;\n        return 1;\n    }\n    return 0;\n'),
 (0x4a6880, 103, "with a focused child: sums, over the children before the focused one, the item count (+0x48) of each child's first spinner list (type 2), plus the focused child's list selection (+0x40), into the extended description's background frame (+0x58). Without a focused child the binary stores the low word of its own stack argument (the widget pointer); kept.",
  [],
  '    widget_instance *focused = widget->focused_child;\n    widget_instance *label = widget->extended_description;\n'
  '    widget_instance *child;\n    int16_t sum = 0;\n\n'
  '    if (focused == 0) {\n        label->background_bitmap_frame = (int16_t)(uintptr_t)widget;\n        return;\n    }\n'
  '    for (child = widget->first_child; child != 0; child = child->next_sibling) {\n        widget_instance *list = first_list_child(child);\n\n'
  '        if (child == focused) {\n            sum = (int16_t)(sum + list->selection_index);\n            break;\n        }\n'
  '        sum = (int16_t)(sum + list->item_count);\n    }\n    label->background_bitmap_frame = sum;\n', FIRST_LIST),
 (0x4a6a60, 69, 'for a selected profile, grows the widget text (+0x3c) to 0x18 bytes in the widget pool and copies up to 0xb characters of the profile name (0x00714e82) into it, terminated at [0xb].',
  ['selected_saved_item', 'saved_item_working_copy', 'widget_memory_pool', 'heap_reallocate'],
  '    uint16_t *text;\n\n    if ((selected_saved_item & 0xf) != 0) {\n        return;\n    }\n'
  '    text = (uint16_t *)heap_reallocate(widget->text, 0x18, widget_memory_pool);\n    widget->text = text;\n'
  '    if (text != 0) {\n        wcsncpy((wchar_t *)text, (const wchar_t *)(saved_item_working_copy + 2), 0xb);\n        text[0xb] = 0;\n    }\n'),
 (0x4a6ab0, 71, 'for a selected variant, grows the widget text (+0x3c) to 0x30 bytes in the widget pool and copies up to 0x17 characters of the variant name (0x00714e80) into it, terminated at [0x17].',
  ['selected_saved_item', 'saved_item_working_copy', 'widget_memory_pool', 'heap_reallocate'],
  '    uint16_t *text;\n\n    if ((selected_saved_item & 0xf) != 1) {\n        return;\n    }\n'
  '    text = (uint16_t *)heap_reallocate(widget->text, 0x30, widget_memory_pool);\n    widget->text = text;\n'
  '    if (text != 0) {\n        wcsncpy((wchar_t *)text, (const wchar_t *)saved_item_working_copy, 0x17);\n        text[0x17] = 0;\n    }\n'),
 (0x4a6e90, 105, 'with a server (+8) or client (+0xb14) game, grows the widget text to 0x10 bytes and formats its dword +0x15c with L"%d" (0x006607a0, 7 characters), terminated at [7].',
  ['network_server_pointer', 'network_client', 'widget_memory_pool', 'heap_reallocate', 'string_format_wide_va_bounded'],
  GAME + '    uint16_t *text;\n\n    if (game == 0) {\n        return;\n    }\n'
  '    text = (uint16_t *)heap_reallocate(widget->text, 0x10, widget_memory_pool);\n    widget->text = text;\n'
  '    if (text != 0) {\n        string_format_wide_va_bounded(7, text, (const uint16_t *)L"%d", *(int32_t *)(game + 0x15c));\n'
  '        text[7] = 0;\n    }\n'),
 (0x4a6f00, 132, 'with a server (+8) or client (+0xb14) game, its dword +0x134 1..5 sets the selection (+0x40) to 0x16, 0x18, 0x17 + (dword +0x190 == 2), 0x17, 0x19; anything else 0x18.',
  ['network_server_pointer', 'network_client'],
  GAME + '\n    if (game == 0) {\n        return;\n    }\n    switch (*(int32_t *)(game + 0x134)) {\n'
  '    case 1:\n        widget->selection_index = 0x16;\n        break;\n    case 3:\n'
  '        widget->selection_index = (int16_t)(0x17 + (*(int32_t *)(game + 0x190) == 2));\n        break;\n'
  '    case 4:\n        widget->selection_index = 0x17;\n        break;\n    case 5:\n        widget->selection_index = 0x19;\n        break;\n'
  '    default:\n        widget->selection_index = 0x18;\n        break;\n    }\n'),
 (0x4a7180, 117, 'with a server (+8) or client (+0xb14) game, its dword +0x134 1..5 sets the background frame (+0x58) to 0, 2, 3, 1, 4; anything else 5.',
  ['network_server_pointer', 'network_client'],
  GAME + '\n    if (game == 0) {\n        return;\n    }\n    switch (*(int32_t *)(game + 0x134)) {\n'
  '    case 1:\n        widget->background_bitmap_frame = 0;\n        break;\n    case 2:\n        widget->background_bitmap_frame = 2;\n        break;\n'
  '    case 3:\n        widget->background_bitmap_frame = 3;\n        break;\n    case 4:\n        widget->background_bitmap_frame = 1;\n        break;\n'
  '    case 5:\n        widget->background_bitmap_frame = 4;\n        break;\n    default:\n        widget->background_bitmap_frame = 5;\n        break;\n    }\n'),
 (0x4a7210, 106, 'with a server (+8) or client (+0xb14) game, grows the widget text to 8 bytes and formats its int16 +0x1a0 with L"%d" (3 characters), terminated at [3].',
  ['network_server_pointer', 'network_client', 'widget_memory_pool', 'heap_reallocate', 'string_format_wide_va_bounded'],
  GAME + '    uint16_t *text;\n\n    if (game == 0) {\n        return;\n    }\n'
  '    text = (uint16_t *)heap_reallocate(widget->text, 8, widget_memory_pool);\n    widget->text = text;\n'
  '    if (text != 0) {\n        string_format_wide_va_bounded(3, text, (const uint16_t *)L"%d", (int32_t)*(int16_t *)(game + 0x1a0));\n'
  '        text[3] = 0;\n    }\n'),
 (0x4a7280, 95, 'for a selected variant, its game type (0x00714eb0 = working copy +0x30) 1..5 sets the selection (+0x40) to 3..7; anything else 8.',
  ['selected_saved_item', 'saved_item_working_copy'],
  '    int32_t type;\n\n    if ((selected_saved_item & 0xf) != 1) {\n        return;\n    }\n    type = *(int32_t *)(saved_item_working_copy + 0x30);\n'
  '    widget->selection_index = (int16_t)(type >= 1 && type <= 5 ? type + 2 : 8);\n'),
 (0x4b52f0, 81, "finds the widget among the eight siblings starting at its parent's third child (not found: returns 1); the position goes to 0x006953e8, key binding scan starts, and byte +0x54 (text pulse) is set on the widget's second child, and on the first two children of its third child. Returns 1.",
  ['chat_state_006953e8', 'input_bind_scan_set_active'],
  '    widget_instance *child = widget->parent->first_child->next_sibling->next_sibling;\n    widget_instance *second;\n    widget_instance *third;\n'
  '    int32_t i;\n\n    for (i = 0; child != widget; i++) {\n        if (i + 1 >= 8) {\n            return 1;\n        }\n        child = child->next_sibling;\n    }\n'
  '    chat_state_006953e8 = i;\n    input_bind_scan_set_active(1);\n'
  '    second = child->first_child->next_sibling;\n    third = second->next_sibling;\n    *((uint8_t *)second + 0x54) = 1;\n'
  '    *((uint8_t *)third->first_child + 0x54) = 1;\n    *((uint8_t *)third->first_child->next_sibling + 0x54) = 1;\n    return 1;\n'),
 (0x4b5350, 70, "in controls list mode (0x00719445): hides the third child (state 0, hidden 1), focuses and shows the second (state 1, hidden 0), leaves list mode, returns 1. Otherwise closes the widget restoring the previous one, *out_handled = 1, returns 1.",
  ['controls_menu_list_mode', 'widget_instance_close_and_restore_previous'],
  '    if (controls_menu_list_mode != 0) {\n        widget_instance *second = widget->first_child->next_sibling;\n'
  '        widget_instance *third = second->next_sibling;\n\n        third->state = 0;\n        third->hidden = 1;\n'
  '        widget->focused_child = second;\n        second->state = 1;\n        second->hidden = 0;\n        controls_menu_list_mode = 0;\n        return 1;\n    }\n'
  '    widget_instance_close_and_restore_previous(widget);\n    *out_handled = 1;\n    return 1;\n'),
 (0x4b54c0, 154, "for a selected profile: the first spinner list under the grandparent's first child and under that child's next sibling give sensitivity a / b (selection + 1) of the selected device. Then, at the great-great-grandparent: hides its third child, focuses and shows its second, leaves controls list mode. Returns 1.",
  ['selected_saved_item', 'controls_selected_device', 'controls_device_sensitivity', 'controls_menu_list_mode'],
  '    widget_instance *screen;\n    widget_instance *second;\n    widget_instance *third;\n\n'
  '    if ((selected_saved_item & 0xf) == 0) {\n        widget_instance *group = widget->parent->parent->first_child;\n        int32_t device = controls_selected_device;\n\n'
  '        controls_device_sensitivity_a[device] = (uint8_t)(first_list_child(group)->selection_index + 1);\n'
  '        controls_device_sensitivity_b[device] = (uint8_t)(first_list_child(group->next_sibling)->selection_index + 1);\n    }\n'
  '    screen = widget->parent->parent->parent->parent;\n    second = screen->first_child->next_sibling;\n    third = second->next_sibling;\n'
  '    third->state = 0;\n    third->hidden = 1;\n    screen->focused_child = second;\n    second->state = 1;\n    second->hidden = 0;\n'
  '    controls_menu_list_mode = 0;\n    return 1;\n', FIRST_LIST),
 (0x4b5ce0, 134, "refreshes the gamepad lists and collects the 17 screen nodes; nodes 15 and 14 show frame 1 when focused; node 16 gets selection 0 and state 1 when node 0 is focused, selection 1 and state 1 when node 5 is, else state 0.",
  ['controls_gamepad_lists_refresh', 'controls_gamepad_widget_nodes_collect'],
  '    widget_instance *nodes[17];\n    widget_instance *focused;\n\n    controls_gamepad_lists_refresh(widget);\n'
  '    controls_gamepad_widget_nodes_collect(nodes, widget);\n'
  '    nodes[15]->background_bitmap_frame = (int16_t)(widget->focused_child == nodes[15]);\n'
  '    nodes[14]->background_bitmap_frame = (int16_t)(widget->focused_child == nodes[14]);\n    focused = widget->focused_child;\n'
  '    if (focused == nodes[0]) {\n        nodes[16]->selection_index = 0;\n        nodes[16]->state = 1;\n    } else if (focused == nodes[5]) {\n'
  '        nodes[16]->selection_index = 1;\n        nodes[16]->state = 1;\n    } else {\n        nodes[16]->state = 0;\n    }\n'),
 (0x4bb290, 102, "clears 0x007196d1. With 0x007196d2 set: closes the root widget (auto close 1 ms, fade 0, state 0), clears 0x007196d2, returns 1. Otherwise a selected profile populates the video options menu from the working copy (returns 1); else 0.",
  ['ui_flag_007196d1', 'ui_flag_007196d2', 'selected_saved_item', 'saved_item_working_copy', 'video_options_menu_populate'],
  '    ui_flag_007196d1 = 0;\n    if (ui_flag_007196d2 != 0) {\n        widget_instance *root = widget;\n\n'
  '        while (root->parent != 0) {\n            root = root->parent;\n        }\n        root->milliseconds_to_auto_close = 1;\n'
  '        root->milliseconds_auto_close_fade = 0;\n        root->state = 0;\n        ui_flag_007196d2 = 0;\n        return 1;\n    }\n'
  '    if ((selected_saved_item & 0xf) != 0) {\n        return 0;\n    }\n    video_options_menu_populate((uint8_t *)widget, saved_item_working_copy);\n    return 1;\n'),
 (0x4bb300, 83, "unless 0x007196d1 is set, the profile's gamma byte (+0xa76; the binary reads address 0xa76 without a profile) becomes the rasterizer gamma exponent and the gamma is applied. Then empties the lists (+0x44) of the second child of the first child and of the second child of the second child's first child. Returns 1.",
  ['ui_flag_007196d1', 'selected_saved_item', 'saved_item_working_copy', 'rasterizer_gamma_exponent', 'chimera__gamma'],
  '    widget_instance *first = widget->first_child;\n\n    if (ui_flag_007196d1 == 0) {\n' + PROFILE.replace('    ', '        ', 1) + '\n'
  '        rasterizer_gamma_exponent = profile[0xa76];\n        chimera__gamma();\n    }\n'
  '    first->first_child->next_sibling->list_items = 0;\n    first->next_sibling->first_child->next_sibling->list_items = 0;\n    return 1;\n'),
]
generate(SPECS)

# 0x4a8310: the default ui list row formatter handed to ui_list_widget_rebuild_rows (0x4a7db0).
p = 'src/interface/ui_list_default_item_format.c'
assert not os.path.exists(p)
open(p, 'w', encoding='utf-8').write('''// ui_list_default_item_format  (not a Ghidra function; the row formatter the list builders hand to
//   ui_list_widget_rebuild_rows 0x4a7db0, pushed as an immediate at 0x4a4667, 0x4a6827, 0x4a8377, 0x4a84e7,
//   0x4a8567 and 0x4a8607; no C existed, so its callers' references were unbound)
// address 0x4a8310, size 77 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x4a8310..0x4a835c: copies up to 0x3f characters of the name of the current
//   ui list's item at item_index (the empty string at 0x00660c34 when out of range) into the row buffer,
//   terminates it at [0x3f] and returns whether the name is non-empty. list_items is not read.
// blam-cc: stack -> item_buffer, item_index, list_items (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include <wchar.h>

extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)

uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items)
{
    uint16_t *out = (uint16_t *)item_buffer;
    const uint16_t *name = (const uint16_t *)L"";

    if (item_index >= 0 && item_index < ui_lists[ui_list_current].count) {
        name = ((ui_list_item *)ui_lists[ui_list_current].data)[item_index].name;
    }
    wcsncpy((wchar_t *)out, (const wchar_t *)name, 0x3f);
    out[0x3f] = 0;
    return (uint8_t)(out[0] != 0);
}
''')
print('wrote', p)
