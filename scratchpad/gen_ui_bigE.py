exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'display_mode_get_current': 'extern void display_mode_get_current(rasterizer_display_mode *out); // 0x515ca0, blam-cc: EDI out',
    'saved_game_last_mp_map_read': 'extern uint8_t saved_game_last_mp_map_read(uint8_t *out_data); // 0x53d670',
    'map_list': ('extern uint8_t *map_list; // 0x00712dcc, map_list_entry[] (0xc bytes, +0 the map path)\n'
                 'extern int32_t map_list_count; // 0x00712dd0'),
    'map_list_get_friendly_level_name': 'extern void map_list_get_friendly_level_name(wchar_t *destination, char *map_path, int32_t destination_capacity); // 0x494f50, blam-cc: EAX map_path, ESI capacity',
    'growable_array_add_element': 'extern uint32_t growable_array_add_element(growable_array *array); // 0x4cf810, blam-cc: ESI array',
    'GlobalAlloc': 'extern void *GlobalAlloc(uint32_t flags, uint32_t bytes); // 0x0063a0b0 IAT',
    'ui_list_has_default': 'extern uint8_t ui_list_has_default; // 0x007192f8',
})

SPECS = [
 (0x4bb7e0, 393, "for a selected profile: builds a display mode from its words +0xa68 / +0xa6a / +0xa6c and (byte +0xa6f != 0), reads the current mode into 0x006b7010, and with the master gain dropped to 0.05, when the mode differs rebuilds the present parameters, resets the device, reads the display mode (device vtable +0x20) into 0x007c11f0, resizes the game window and clears the reset request (changed); restores the gain and stamps the widget's creation time with the performance counter in ms (inlined). 0x007196d2 is cleared. Changed: 0x006894ba = 0 while forcing single ticks, else (+0xa6f == 2); returns 1. Unchanged: sets 0x007196d2, closes the root widget (auto close 1 ms, fade 0, state 0), returns 0. Without a profile: closes the root widget and returns 0.",
  ['selected_saved_item', 'saved_item_working_copy', 'sound_master_gain', 'video_mode', 'display_mode_get_current', 'time_query_performance_counter_ms',
   'ui_flag_007196d2'],
  '    widget_instance *root;\n    int32_t changed = -1;\n\n    if ((selected_saved_item & 0xf) == 0) {\n'
  '        uint8_t *profile = saved_item_working_copy;\n        float gain = sound_master_gain;\n        rasterizer_display_mode mode;\n\n'
  '        mode.width = *(int16_t *)(profile + 0xa68);\n        mode.height = *(int16_t *)(profile + 0xa6a);\n'
  '        mode.refresh_rate = *(int16_t *)(profile + 0xa6c);\n        mode.vsync = (uint8_t)(profile[0xa6f] != 0);\n'
  '        display_mode_get_current(&ui_video_requested_display_mode_006b7010);\n        sound_set_master_gain(0.05f);\n        changed = 0;\n'
  '        if (rasterizer_display_mode_differs(&mode) != 0) {\n            d3d_present_parameters parameters;\n\n'
  '            rasterizer_build_present_parameters(&parameters, &mode);\n            rasterizer_device_reset(&parameters);\n'
  '            ((int32_t (__stdcall *)(void *, uint32_t, void *))(*(void ***)rasterizer_device)[0x20 / 4])(rasterizer_device, 0,\n'
  '                &rasterizer_desktop_display_mode);\n            changed = 1;\n'
  '            rasterizer_resize_game_window(mode.height, mode.width);\n            rasterizer_needs_reset = 0;\n        }\n'
  '        sound_set_master_gain(gain);\n        widget->creation_time = (int32_t)time_query_performance_counter_ms();\n    }\n'
  '    ui_flag_007196d2 = 0;\n    if (changed == 1) {\n        if (game_time_force_single_tick != 0) {\n            unknown_006894ba = 0;\n'
  '        } else {\n            unknown_006894ba = (uint8_t)(saved_item_working_copy[0xa6f] == 2);\n        }\n        return 1;\n    }\n'
  '    if (changed == 0) {\n        ui_flag_007196d2 = 1;\n    }\n    root = widget;\n    while (root->parent != 0) {\n        root = root->parent;\n    }\n'
  '    root->milliseconds_to_auto_close = 1;\n    root->milliseconds_auto_close_fade = 0;\n    root->state = 0;\n    return 0;\n'),
 (0x49f8f0, 394, "for a selected variant, the first spinner lists of the first four children show: byte +0x7c == 0 ? 1 : 0; dword +0x58 (2, 5, 10, 15 -> 1..4, else 0); byte +0x34 == 0 ? 1 : 0; dword +0x78 (0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68 -> 1..6, else 0). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    int32_t value;\n\n    if (variant == 0) {\n        return 0;\n    }\n    group = widget->first_child;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x7c] == 0 ? 1 : 0);\n    group = group->next_sibling;\n'
  '    value = *(int32_t *)(variant + 0x58);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 2 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)(variant[0x34] == 0 ? 1 : 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x78);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :\n'
  '        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);\n    return 1;\n'),
 (0x49d5f0, 422, "points the widget list at the multiplayer map list (0x00712dcc, 0x00712dd0 entries); when the last multiplayer map record reads, selects the entry whose path matches it case-insensitively (0 when none). The committed selection (+0x3c) takes the selection and the first visible item (+0x3e) -1. Resets the three ui lists and adds, for every map, a GlobalAlloc copy of its friendly name (0x100 characters) with the map index as id, marking the selected one as the default (0x007192f8). Returns 1.",
  ['map_list', 'saved_game_last_mp_map_read', '__stricmp', 'ui_lists', 'ui_list_current', 'ui_list_has_default',
   'map_list_get_friendly_level_name', 'growable_array_add_element', 'GlobalAlloc'],
  '    int32_t count = map_list_count;\n    char last_map[0x104];\n    uint16_t name[0x100];\n    int32_t i;\n\n'
  '    widget->list_items = map_list;\n    widget->item_count = (uint16_t)count;\n'
  '    if (saved_game_last_mp_map_read((uint8_t *)last_map) != 0) {\n        widget->selection_index = 0;\n'
  '        if (count > 0) {\n            while (__stricmp(last_map, *(char **)(map_list + widget->selection_index * 0xc)) != 0) {\n'
  '                widget->selection_index++;\n                if (widget->selection_index >= count) {\n                    break;\n                }\n            }\n        }\n'
  '        if (widget->selection_index == count) {\n            widget->selection_index = 0;\n        }\n    }\n'
  '    *(int16_t *)((uint8_t *)widget + 0x3c) = widget->selection_index;\n    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;\n'
  '    for (i = 0; i < 3; i++) {\n        ui_lists[i].element_size = 0x10;\n        ui_lists[i].count = 0;\n        ui_lists[i].data = 0;\n    }\n'
  '    ui_list_current = -1;\n    ui_list_has_default = 0;\n    for (i = 0; i < count; i++) {\n        uint8_t is_default;\n        uint32_t index;\n\n'
  '        map_list_get_friendly_level_name((wchar_t *)name, *(char **)(map_list + i * 0xc), 0x100);\n'
  '        is_default = (uint8_t)(i == widget->selection_index);\n        index = growable_array_add_element(&ui_lists[0]);\n'
  '        if (index != 0xffffffff) {\n            ui_list_item *item = (ui_list_item *)ui_lists[0].data + index;\n            uint16_t *copy;\n\n'
  '            item->data = 0;\n            copy = (uint16_t *)GlobalAlloc(0, (uint32_t)wcslen((const wchar_t *)name) * 2 + 2);\n'
  '            item->name = copy;\n            item->id = i;\n            item->is_default = is_default;\n'
  '            if (is_default) {\n                ui_list_has_default = 1;\n            }\n'
  '            wcscpy((wchar_t *)copy, (const wchar_t *)name);\n        }\n    }\n    return 1;\n'),
]
generate([s if len(s) > 5 or 'first_list_child' not in s[4] else s + (FIRST_LIST,) for s in SPECS])
