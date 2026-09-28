exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'variant_globals': ('extern uint8_t variant_teams_enabled_0071920c; // 0x0071920c, UNSURE name (variant byte +0x34 != 0)\n'
                        'extern int32_t variant_team_selection_00692b08; // 0x00692b08, UNSURE name\n'
                        'extern int32_t unknown_00692b0c; // 0x00692b0c, UNSURE'),
    'ui_controls_populate_bind_rows': 'extern void ui_controls_populate_bind_rows(widget_instance *widget, uint32_t packed); // 0x4a3180',
    'controls_build_device_label_table': 'extern void controls_build_device_label_table(void); // 0x4b4890',
    'controls_binding_list_refresh_rows': 'extern int32_t controls_binding_list_refresh_rows(widget_instance *widget, int32_t page); // 0x4b4790, blam-cc: EAX widget',
    'controls_device_labels': ('extern int32_t controls_device_label_count; // 0x00719440\n'
                               'extern uint8_t controls_device_labels[]; // 0x006932e8'),
    'saved_game_create_custom_variant': 'extern uint32_t saved_game_create_custom_variant(uint32_t param_1, uint16_t *name); // 0x53bb50, blam-cc: ECX name too',
    'ui_list_get': ('extern int32_t ui_list_get_id(int32_t index); // 0x4a7c80, blam-cc: EDX index\n'
                    'extern void *ui_list_get_data(int32_t index); // 0x4a7c50, blam-cc: EDX index'),
    'game_engine_variant_defaults_classic_slayer': 'extern void *game_engine_variant_defaults_classic_slayer(void *out); // 0x463c40',
})

SPECS = [
 (0x4a33a0, 365, "for a selected variant: 0x0071920c = (byte +0x34 != 0), 0x00692b0c = 0, dwords +0x60 / +0x64 / +0x68 go to 0x00879f34 / 0x00879f38 / 0x00719208, and the bind rows are populated from +0x60 (0x4a3180). The first child's spinner list shows +0x68 (0, 0x384, 0x708, 0xa8c, 0xe10, 0x1518, 0x2328 -> 0..6, else 0); the second child is shown (hidden 0, state 1) when 0x0071920c is set, else hidden (state 0); the third child's list shows the low nibble of 0x00879f34 when below 9, else 0; 0x00692b08 takes the second child's list selection. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy', 'variant_globals', 'unknown_00879f34', 'unknown_00879f38', 'unknown_00719208', 'ui_controls_populate_bind_rows'],
  VARIANT + '    widget_instance *first;\n    widget_instance *second;\n    widget_instance *second_list;\n    int32_t time;\n\n'
  '    if (variant == 0) {\n        return 0;\n    }\n    time = *(int32_t *)(variant + 0x68);\n'
  '    variant_teams_enabled_0071920c = (uint8_t)(variant[0x34] != 0);\n    unknown_00692b0c = 0;\n'
  '    unknown_00879f34 = *(uint32_t *)(variant + 0x60);\n    unknown_00879f38 = *(uint32_t *)(variant + 0x64);\n    unknown_00719208 = (uint32_t)time;\n'
  '    ui_controls_populate_bind_rows(widget, *(uint32_t *)(variant + 0x60));\n    first = widget->first_child;\n'
  '    first_list_child(first)->selection_index = (int16_t)(time == 0x384 ? 1 : time == 0x708 ? 2 : time == 0xa8c ? 3 :\n'
  '        time == 0xe10 ? 4 : time == 0x1518 ? 5 : time == 0x2328 ? 6 : 0);\n'
  '    second = first->next_sibling;\n    second_list = first_list_child(second);\n    if (variant_teams_enabled_0071920c != 0) {\n'
  '        second->hidden = 0;\n        second->state = 1;\n    } else {\n        second->hidden = 1;\n        second->state = 0;\n    }\n'
  '    first_list_child(second->next_sibling)->selection_index = (int16_t)((unknown_00879f34 & 0xf) < 9 ? (unknown_00879f34 & 0xf) : 0);\n'
  '    variant_team_selection_00692b08 = second_list->selection_index;\n    return 1;\n', FIRST_LIST),
 (0x4b4980, 366, "resets 0x006953e8 to -1, hides the third child (state 0, hidden 1), focuses and shows the second, leaves controls list mode. For a selected profile (else returns 0) copies its controls section back into the live block at 0x006b3a48 (the inverse of 0x4b4af0), clears 0x00719444, rebuilds the device label table and points the second child's nested list (first -> first -> next) at it (selection 0, 0x00719440 items, data 0x006932e8), then refreshes the binding rows from page 0. Returns 1.",
  ['chat_state_006953e8', 'controls_menu_list_mode', 'selected_saved_item', 'saved_item_working_copy', 'input_controls_live_006b3a48',
   'ui_flag_00719444', 'controls_build_device_label_table', 'controls_device_labels', 'controls_binding_list_refresh_rows'],
  '    widget_instance *second = widget->first_child->next_sibling;\n    widget_instance *third = second->next_sibling;\n'
  '    uint8_t *live = input_controls_live_006b3a48;\n    uint8_t *profile;\n    widget_instance *list;\n\n'
  '    chat_state_006953e8 = -1;\n    third->state = 0;\n    third->hidden = 1;\n    widget->focused_child = second;\n'
  '    second->state = 1;\n    second->hidden = 0;\n    controls_menu_list_mode = 0;\n'
  '    if ((selected_saved_item & 0xf) != 0) {\n        return 0;\n    }\n    profile = saved_item_working_copy;\n'
  '    memcpy(live + 0x220, profile + 0x134, 0xda);\n    memcpy(live + 0x10, profile + 0x20e, 0x10);\n'
  '    memcpy(live + 0x880, profile + 0x21e, 0xc);\n    memcpy(live + 0x380, profile + 0x22a, 0x100);\n'
  '    memcpy(live + 0x0, profile + 0x32a, 0x10);\n    memcpy(live + 0x20, profile + 0x33a, 0x200);\n'
  '    memcpy(live + 0x480, profile + 0x53a, 0x400);\n    ui_flag_00719444 = 0;\n'
  '    memcpy(live + 0x2fc, profile + 0x956, 4);\n    memcpy(live + 0x88c, profile + 0x95a, 4);\n'
  '    controls_build_device_label_table();\n    list = second->first_child->first_child->next_sibling;\n'
  '    list->selection_index = 0;\n    list->item_count = (uint16_t)controls_device_label_count;\n    list->list_items = controls_device_labels;\n'
  '    controls_binding_list_refresh_rows(second, 0);\n    return 1;\n'),
 (0x4a1310, 368, "picks a free saved game name and creates a custom variant with it for the widget's controller (0x53bb50); on success selects it and, when it is a variant, fills the working copy from the grandparent list's selected item data (or the classic slayer defaults when that item has no id), clears the word at +0x94, copies up to 0x17 characters of the name (terminated at +0x2e), clears flag bits 7 and 8 (+0x38) and opens the virtual keyboard on the name (0x30 characters, field kind 9). Opened (1): edit field 2, clears the last multiplayer variant record of its directory when found, returns 1. Any other nonzero keyboard result is returned. Every failure (a non-variant selection is also dropped) raises quit confirm error 0x26 when none is up, sound 4, returns 0.",
  ['saved_game_allocate_new_slot', 'saved_game_create_custom_variant', 'saved_item_select', 'selected_saved_item', 'saved_item_working_copy',
   'ui_list_get', 'game_engine_variant_defaults_classic_slayer', 'virtual_keyboard_open', 'network_host_edit_field_00719410',
   'saved_game_get_directory_by_handle', 'saved_game_last_mp_variant_clear', 'quit_confirm_error', 'widget_play_sound_effect'],
  '    widget_instance *list = widget->parent->parent;\n    uint16_t name[0x80];\n    uint8_t scratch[0x100];\n    uint32_t handle;\n\n'
  '    saved_game_allocate_new_slot(name);\n    if (name[0] != 0) {\n'
  '        handle = saved_game_create_custom_variant((uint32_t)(uint16_t)widget->controller_index, name);\n        if (handle != 0xffffffff) {\n'
  '            saved_item_select((int32_t)handle);\n            if ((selected_saved_item & 0xf) == 1) {\n'
  '                int32_t id = ui_list_get_id(*(int16_t *)((uint8_t *)list + 0x3c));\n                const void *source;\n                uint8_t opened;\n\n'
  '                source = id != -1 ? ui_list_get_data(id) : game_engine_variant_defaults_classic_slayer(scratch);\n'
  '                memcpy(saved_item_working_copy, source, 0x98);\n                *(uint16_t *)(saved_item_working_copy + 0x94) = 0;\n'
  '                wcsncpy((wchar_t *)saved_item_working_copy, (const wchar_t *)name, 0x17);\n'
  '                *(uint16_t *)(saved_item_working_copy + 0x2e) = 0;\n'
  '                *(uint32_t *)(saved_item_working_copy + 0x38) &= 0xfffffe7f;\n'
  '                opened = virtual_keyboard_open((uint16_t *)saved_item_working_copy, 0x30, 9);\n'
  '                if (opened == 1) {\n                    network_host_edit_field_00719410 = 2;\n'
  '                    if (saved_game_get_directory_by_handle((int32_t)handle, (char *)scratch) != 0) {\n'
  '                        saved_game_last_mp_variant_clear(scratch);\n                    }\n                    return 1;\n                }\n'
  '                if (opened != 0) {\n                    return opened;\n                }\n            } else {\n                selected_saved_item = -1;\n            }\n        }\n    }\n'
  '    if (quit_confirm_error_string_index == -1) {\n' + QUIT_ERR % '0x26' + '    }\n    widget_play_sound_effect(4);\n    return 0;\n'),
]
generate(SPECS)
