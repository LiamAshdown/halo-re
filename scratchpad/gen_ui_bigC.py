exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'saved_game_get_variant': 'extern uint8_t saved_game_get_variant(int32_t handle, void *out); // 0x53bee0',
    'saved_game_get_directory_by_handle': 'extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: EAX handle, ESI out_directory',
    'saved_game_last_mp_variant_clear': 'extern void saved_game_last_mp_variant_clear(const void *data); // 0x53d360',
    'game_variant_saved_default': ('extern uint8_t game_variant_saved_default[0x98]; // 0x00714de0 (game_variant)\n'
                                   'extern uint8_t game_variant_saved_default_valid; // 0x00714e78'),
    'widget_close_all': 'extern void widget_close_all(void); // 0x498650',
    'game_engine_begin_end_game_sequence': 'extern void game_engine_begin_end_game_sequence(void); // 0x45fd90',
    'sound_master_gain': ('extern float sound_master_gain; // 0x007252ac\n'
                          'extern void sound_set_master_gain(float gain); // 0x548590'),
    'video_mode': ('extern rasterizer_display_mode ui_video_requested_display_mode_006b7010; // 0x006b7010, UNSURE name (the video options screen\'s pick)\n'
                   'extern uint8_t unknown_006894ba; // 0x006894ba\n'
                   'extern int32_t game_time_force_single_tick; // 0x007196d8\n'
                   'extern d3d_display_mode rasterizer_desktop_display_mode; // 0x007c11f0\n'
                   'extern uint8_t rasterizer_needs_reset; // 0x0071d16d\n'
                   'extern void *rasterizer_device; // 0x0071d174\n'
                   'extern uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested); // 0x515d10, blam-cc: EDI\n'
                   'extern void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source); // 0x515fc0, blam-cc: EAX source\n'
                   'extern uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters); // 0x515d90\n'
                   'extern void rasterizer_resize_game_window(int32_t height, int32_t width); // 0x515b20, blam-cc: EAX height, ECX width'),
})

SPECS = [
 (0x49dab0, 258, "reads the widget list (+0x44) at the ui list id of its committed selection (index -1 when out of range). -1: sound 4, returns 0. Not negative: raises quit confirm error 0x1f when none is up, sound 4, returns 0. Negative: loads that saved variant (0x98 bytes; failure returns 0), clears the last multiplayer variant record of its directory when found, copies it over the saved default variant (0x00714de0) and marks it valid; in network_game_mode 2 then makes sure the variant history has an entry, closes every widget, begins the end game sequence and returns 0; else returns 1.",
  ['ui_list_current', 'ui_lists', 'quit_confirm_error', 'widget_play_sound_effect', 'saved_game_get_variant', 'saved_game_get_directory_by_handle',
   'saved_game_last_mp_variant_clear', 'game_variant_saved_default', 'network_game_mode', 'game_engine_ensure_variant_history_has_entry',
   'widget_close_all', 'game_engine_begin_end_game_sequence'],
  '    uint32_t variant[0x26];\n    char directory[0x100];\n    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));\n'
  '    int32_t item = ((int32_t *)widget->list_items)[id];\n\n    if (item == -1 || item >= 0) {\n'
  '        if (item != -1 && quit_confirm_error_string_index == -1) {\n' + QUIT_ERR.replace('        ', '            ') % '0x1f' + '        }\n'
  '        widget_play_sound_effect(4);\n        return 0;\n    }\n    if (saved_game_get_variant(item, variant) == 0) {\n        return 0;\n    }\n'
  '    if (saved_game_get_directory_by_handle(item, directory) != 0) {\n        saved_game_last_mp_variant_clear(directory);\n    }\n'
  '    memcpy(game_variant_saved_default, variant, sizeof(variant));\n    game_variant_saved_default_valid = 1;\n'
  '    if (network_game_mode != 2) {\n        return 1;\n    }\n    game_engine_ensure_variant_history_has_entry();\n    widget_close_all();\n'
  '    game_engine_begin_end_game_sequence();\n    return 0;\n', LIST_ID),
 (0x4bb970, 263, "unless 0x007196d2 is set: stores the requested display mode's width, height and refresh rate words in the selected profile (+0xa68 / +0xa6a / +0xa6c; the binary writes through null without a profile), with vsync sets +0xa6f to 1 + (0x006894ba != 0); 0x006894ba becomes 0 while forcing single ticks, else (+0xa6f == 2). With the master gain dropped to 0.05, when the mode differs from the current one rebuilds the present parameters, resets the device, reads the display mode (device vtable +0x20) into 0x007c11f0, resizes the game window and clears the reset request; then restores the gain. Returns 1.",
  ['ui_flag_007196d2', 'selected_saved_item', 'saved_item_working_copy', 'sound_master_gain', 'video_mode'],
  '    float gain;\n    uint8_t *profile;\n\n    if (ui_flag_007196d2 != 0) {\n        return 1;\n    }\n    gain = sound_master_gain;\n'
  '    profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;\n'
  '    *(uint16_t *)(profile + 0xa68) = (uint16_t)ui_video_requested_display_mode_006b7010.width;\n'
  '    *(uint16_t *)(profile + 0xa6c) = (uint16_t)ui_video_requested_display_mode_006b7010.refresh_rate;\n'
  '    *(uint16_t *)(profile + 0xa6a) = (uint16_t)ui_video_requested_display_mode_006b7010.height;\n'
  '    if (ui_video_requested_display_mode_006b7010.vsync != 0) {\n        profile[0xa6f] = (uint8_t)((unknown_006894ba != 0) + 1);\n    }\n'
  '    if (game_time_force_single_tick != 0) {\n        unknown_006894ba = 0;\n    } else {\n        unknown_006894ba = (uint8_t)(profile[0xa6f] == 2);\n    }\n'
  '    sound_set_master_gain(0.05f);\n    if (rasterizer_display_mode_differs(&ui_video_requested_display_mode_006b7010) != 0) {\n'
  '        d3d_present_parameters parameters;\n\n        rasterizer_build_present_parameters(&parameters, &ui_video_requested_display_mode_006b7010);\n'
  '        rasterizer_device_reset(&parameters);\n'
  '        ((int32_t (__stdcall *)(void *, uint32_t, void *))(*(void ***)rasterizer_device)[0x20 / 4])(rasterizer_device, 0,\n'
  '            &rasterizer_desktop_display_mode);\n'
  '        rasterizer_resize_game_window(ui_video_requested_display_mode_006b7010.height, ui_video_requested_display_mode_006b7010.width);\n'
  '        rasterizer_needs_reset = 0;\n    }\n    sound_set_master_gain(gain);\n    return 1;\n'),
 (0x4a0d60, 303, "for a selected profile, the first spinner lists of the first five children show: byte +0x12f == 0 ? 1 : 0; byte +0x12e 1..10 as 0..9 (else 0); byte +0x130 == 1; byte +0x131 == 0; byte +0x132 == 0. Returns 1, or 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy'],
  PROFILE + '    widget_instance *group;\n    uint8_t value;\n\n    if (profile == 0) {\n        return 0;\n    }\n    group = widget->first_child;\n'
  '    first_list_child(group)->selection_index = (int16_t)(profile[0x12f] == 0 ? 1 : 0);\n    group = group->next_sibling;\n'
  '    value = profile[0x12e];\n    first_list_child(group)->selection_index = (int16_t)(value > 0 && value <= 10 ? value - 1 : 0);\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)(profile[0x130] == 1);\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)(profile[0x131] == 0);\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)(profile[0x132] == 0);\n    return 1;\n'),
 (0x4a0fb0, 305, "for a selected profile, the inverse of 0x4a0d60: selection 0 / 1 of the first list sets byte +0x12f to 1 / 0; 0..9 of the second sets +0x12e to selection + 1; 0 / 1 of the third sets +0x130 to 0 / 1; of the fourth +0x131 to 1 / 0; of the fifth +0x132 to 1 / 0. Other selections leave them. Returns 1, or 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy'],
  PROFILE + '    widget_instance *group;\n    int16_t selection;\n\n    if (profile == 0) {\n        return 0;\n    }\n    group = widget->first_child;\n'
  '    selection = first_list_child(group)->selection_index;\n    if (selection == 0 || selection == 1) {\n        profile[0x12f] = (uint8_t)(selection == 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 9) {\n        profile[0x12e] = (uint8_t)(selection + 1);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n    if (selection == 0 || selection == 1) {\n        profile[0x130] = (uint8_t)selection;\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n    if (selection == 0 || selection == 1) {\n        profile[0x131] = (uint8_t)(selection == 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n    if (selection == 0 || selection == 1) {\n        profile[0x132] = (uint8_t)(selection == 0);\n    }\n    return 1;\n'),
 (0x49f300, 312, "for a selected variant, from the grandparent's first three spinner lists: 0 / 1 set / clear flag bit 2 (+0x38); 0..13 store dword +0x5c; 0 / 1 clear / set flag bit 5. Other selections leave them. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    uint32_t *flags;\n    int16_t selection;\n\n    if (variant == 0) {\n        return 0;\n    }\n'
  '    flags = (uint32_t *)(variant + 0x38);\n    group = widget->parent->parent->first_child;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0) {\n        *flags |= 4;\n    } else if (selection == 1) {\n        *flags &= ~4u;\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 0xd) {\n        *(int32_t *)(variant + 0x5c) = selection;\n    }\n'
  '    selection = first_list_child(group->next_sibling)->selection_index;\n'
  '    if (selection == 0) {\n        *flags &= ~0x20u;\n    } else if (selection == 1) {\n        *flags |= 0x20;\n    }\n    return 1;\n'),
]
generate([s if len(s) > 5 or 'first_list_child' not in s[4] else s + (FIRST_LIST,) for s in SPECS])
