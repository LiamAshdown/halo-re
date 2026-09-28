exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
load_ext(r'C:\Users\Liam-\halo-re\scratchpad\gen_ui_mid2.py')
load_ext(r'C:\Users\Liam-\halo-re\scratchpad\gen_ui_bigA.py')

EXT.update({
    'resolution_selection_00719204': 'extern int32_t resolution_selection_00719204; // 0x00719204',
    'network_capability_flag_006894a2': 'extern uint8_t network_capability_flag_006894a2; // 0x006894a2',
    'quality_selection_00692b04': 'extern int32_t quality_selection_00692b04; // 0x00692b04',
    'network_game_start_new_server_with_name_and_password': 'extern uint8_t network_game_start_new_server_with_name_and_password(uint32_t param_1, uint16_t *name, uint16_t *password); // 0x4e4150',
    'saved_game_allocate_new_slot': 'extern void saved_game_allocate_new_slot(uint16_t *out_name); // 0x53ca80, blam-cc: EBX out_name',
    'saved_game_create_default_profile': 'extern uint32_t saved_game_create_default_profile(uint16_t *name); // 0x539ab0, blam-cc: ECX name (the pushed slot is not read)',
})

SPECS = [
 (0x4a3790, 215, "for a selected variant: the first spinner lists of the first three children show byte +0x6c (0..3, else 1), dword +0x70 (0x96 -> 1, 0x12c -> 2, 0x1c2 -> 3, else 0) and byte +0x74 != 0. Returns 1 (AL keeps the 1 loaded before the test).",
  ['selected_saved_item', 'saved_item_working_copy'],
  '    uint8_t *variant = saved_item_working_copy;\n    widget_instance *group;\n    int32_t time;\n\n'
  '    if ((selected_saved_item & 0xf) != 1) {\n        return 1;\n    }\n    group = widget->first_child;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x6c] <= 3 ? variant[0x6c] : 1);\n    group = group->next_sibling;\n'
  '    time = *(int32_t *)(variant + 0x70);\n'
  '    first_list_child(group)->selection_index = (int16_t)(time == 0x96 ? 1 : time == 0x12c ? 2 : time == 0x1c2 ? 3 : 0);\n'
  '    first_list_child(group->next_sibling)->selection_index = (int16_t)(variant[0x74] != 0);\n    return 1;\n'),
 (0x4a3870, 221, "for a selected variant, the inverse of 0x4a3790 from the grandparent's first three children: byte +0x6c = selection (0..3, else 1), dword +0x70 = 0x96 / 0x12c / 0x1c2 for selection 1..3 (else 0), byte +0x74 = (selection == 1). Returns 1.",
  ['selected_saved_item', 'saved_item_working_copy'],
  '    uint8_t *variant = saved_item_working_copy;\n    widget_instance *group;\n    int16_t selection;\n\n'
  '    if ((selected_saved_item & 0xf) != 1) {\n        return 1;\n    }\n    group = widget->parent->parent->first_child;\n'
  '    selection = first_list_child(group)->selection_index;\n    variant[0x6c] = (uint8_t)(selection >= 0 && selection <= 3 ? selection : 1);\n'
  '    group = group->next_sibling;\n    switch (first_list_child(group)->selection_index) {\n'
  '    case 1:\n        *(int32_t *)(variant + 0x70) = 0x96;\n        break;\n    case 2:\n        *(int32_t *)(variant + 0x70) = 0x12c;\n        break;\n'
  '    case 3:\n        *(int32_t *)(variant + 0x70) = 0x1c2;\n        break;\n    default:\n        *(int32_t *)(variant + 0x70) = 0;\n        break;\n    }\n'
  '    variant[0x74] = (uint8_t)(first_list_child(group->next_sibling)->selection_index == 1);\n    return 1;\n'),
 (0x4a0700, 224, "for a selected variant: the first spinner list of the first child shows dword +0x3c (1 or 2, else 0); that of the second shows 2 when flag bit 0 (+0x38) is clear, else flag bit 6; that of the third shows 1 when flag bit 1 is clear, else 0. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    int32_t value;\n    uint32_t flags;\n\n    if (variant == 0) {\n        return 0;\n    }\n'
  '    group = widget->first_child;\n    value = *(int32_t *)(variant + 0x3c);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 1 || value == 2 ? value : 0);\n    group = group->next_sibling;\n'
  '    flags = *(uint32_t *)(variant + 0x38);\n'
  '    first_list_child(group)->selection_index = (int16_t)((flags & 1) == 0 ? 2 : (flags >> 6) & 1);\n'
  '    first_list_child(group->next_sibling)->selection_index = (int16_t)(((*(uint32_t *)(variant + 0x38) >> 1) & 1) == 0 ? 1 : 0);\n    return 1;\n'),
 (0x4a2f10, 225, "with a current profile handle (0x00714dd4 != -1): selects it, copies the host name (0x00719170) and subname (0x007191f0) into the profile (+0xd8c / +0xeac; the binary also calls wcslen on each, unused), when not saving copies 0x00719204 into +0xebf, when 0x006894a2 is set stores the quality selection clamped to 0..4 in +0xfc0, then saves when changed or else drops the selection. While saving (0x00719010) returns 0; otherwise starts a new server with that name and password and returns the high byte of the entry ECX, which the dispatchers (0x497c9a, 0x49a4be) load with the table index below 0xbe: always 0.",
  ['saved_player_profile_slots_handle', 'saved_item_select', 'selected_saved_item', 'saved_item_working_copy', 'network_host_name_00719170',
   'network_host_subname_007191f0', 'save_in_progress_00719010', 'resolution_selection_00719204', 'network_capability_flag_006894a2',
   'quality_selection_00692b04', 'saved_item_has_unsaved_changes', 'player_profile_save', 'network_game_start_new_server_with_name_and_password'],
  '    int32_t handle = saved_player_profile_slots_handle;\n\n    if (handle != -1) {\n        uint8_t *profile;\n\n'
  '        saved_item_select(handle);\n        profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;\n'
  '        wcscpy((wchar_t *)(profile + 0xd8c), (const wchar_t *)network_host_name_00719170);\n'
  '        wcscpy((wchar_t *)(profile + 0xeac), (const wchar_t *)network_host_subname_007191f0);\n'
  '        if (save_in_progress_00719010 == 0) {\n            profile[0xebf] = (uint8_t)resolution_selection_00719204;\n        }\n'
  '        if (network_capability_flag_006894a2 != 0) {\n            int32_t quality = quality_selection_00692b04;\n\n'
  '            profile[0xfc0] = (uint8_t)(quality < 0 ? 0 : quality > 4 ? 4 : quality);\n        }\n'
  '        if (saved_item_has_unsaved_changes() != 0) {\n            player_profile_save();\n        } else {\n            selected_saved_item = -1;\n        }\n    }\n'
  '    if (save_in_progress_00719010 != 0) {\n        return 0;\n    }\n'
  '    network_game_start_new_server_with_name_and_password(0, network_host_name_00719170, network_host_subname_007191f0);\n    return 0;\n'),
 (0x49f470, 228, "for a selected variant, from the grandparent's first three children's spinner lists: selection 0..2 sets dword +0x3c to 0..2; the second list's 0 / 1 / 2 set flags (+0x38) to (flags & ~0x40) | 1, flags | 0x41, flags & ~0x41; the third list's 0 / 1 set / clear flag bit 1. Other selections leave the value. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    uint32_t *flags;\n\n    if (variant == 0) {\n        return 0;\n    }\n    flags = (uint32_t *)(variant + 0x38);\n'
  '    group = widget->parent->parent->first_child;\n    switch (first_list_child(group)->selection_index) {\n'
  '    case 0:\n        *(int32_t *)(variant + 0x3c) = 0;\n        break;\n    case 1:\n        *(int32_t *)(variant + 0x3c) = 1;\n        break;\n'
  '    case 2:\n        *(int32_t *)(variant + 0x3c) = 2;\n        break;\n    }\n    group = group->next_sibling;\n'
  '    switch (first_list_child(group)->selection_index) {\n    case 0:\n        *flags = (*flags & ~0x40u) | 1;\n        break;\n'
  '    case 1:\n        *flags |= 0x41;\n        break;\n    case 2:\n        *flags &= ~0x41u;\n        break;\n    }\n'
  '    switch (first_list_child(group->next_sibling)->selection_index) {\n    case 0:\n        *flags |= 2;\n        break;\n'
  '    case 1:\n        *flags &= ~2u;\n        break;\n    }\n    return 1;\n'),
 (0x4a6d50, 230, "with a server (+8) or client (+0xb14) game, its dword +0x134 picks the selection (+0x40): 1 -> byte +0x180 == 1 ? (dword +0x184 ? 0x1c : 0x1d) : (dword +0x184 ? 0x1e : 3); 2 -> 4; 3 -> dword +0x190 1 / 2 give 0x1f / 0x20, else 5; 4 -> 6; 5 -> dword +0x180 == 2 ? 0x21 : 7; else 8.",
  ['network_server_pointer', 'network_client'],
  GAME2 + '\n    if (game == 0) {\n        return;\n    }\n    switch (*(int32_t *)(game + 0x134)) {\n    case 1:\n'
  '        if (game[0x180] == 1) {\n            widget->selection_index = (int16_t)(*(int32_t *)(game + 0x184) != 0 ? 0x1c : 0x1d);\n'
  '        } else {\n            widget->selection_index = (int16_t)(*(int32_t *)(game + 0x184) != 0 ? 0x1e : 3);\n        }\n        break;\n'
  '    case 2:\n        widget->selection_index = 4;\n        break;\n    case 3:\n'
  '        switch (*(int32_t *)(game + 0x190)) {\n        case 1:\n            widget->selection_index = 0x1f;\n            break;\n'
  '        case 2:\n            widget->selection_index = 0x20;\n            break;\n        default:\n            widget->selection_index = 5;\n            break;\n        }\n        break;\n'
  '    case 4:\n        widget->selection_index = 6;\n        break;\n'
  '    case 5:\n        widget->selection_index = (int16_t)(*(int32_t *)(game + 0x180) == 2 ? 0x21 : 7);\n        break;\n'
  '    default:\n        widget->selection_index = 8;\n        break;\n    }\n'),
 (0x4a1480, 233, "picks a free saved game name for slot event word 1 (-1 means 0) and creates a default profile with it (0x539ab0; the pushed slot is not read). On success selects it, loads it for player 0, copies up to 0xb characters of the name into the profile name (+2, terminated at +0x18) and returns the result of opening the virtual keyboard on it (0x18 characters, field kind 8) when nonzero. Without a selected profile the selection is dropped. Every failure raises quit confirm error 0x25 (when none is up), plays sound 4 and returns 0.",
  ['saved_game_allocate_new_slot', 'saved_game_create_default_profile', 'saved_item_select', 'selected_saved_item', 'saved_item_working_copy',
   'player_profile_load', 'virtual_keyboard_open', 'quit_confirm_error', 'widget_play_sound_effect'],
  '    uint16_t name[0x82];\n    uint32_t handle;\n\n    saved_game_allocate_new_slot(name);\n'
  '    if (name[0] != 0) {\n        handle = saved_game_create_default_profile(name);\n        if (handle != 0xffffffff) {\n'
  '            uint8_t *profile;\n\n            saved_item_select((int32_t)handle);\n'
  '            profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;\n            player_profile_load(0, profile, (int32_t)handle);\n'
  '            if (profile != 0) {\n                uint8_t opened;\n\n                wcsncpy((wchar_t *)(profile + 2), (const wchar_t *)name, 0xb);\n'
  '                *(uint16_t *)(profile + 0x18) = 0;\n                opened = virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8);\n'
  '                if (opened != 0) {\n                    return opened;\n                }\n            } else {\n                selected_saved_item = -1;\n            }\n        }\n    }\n'
  '    if (quit_confirm_error_string_index == -1) {\n' + QUIT_ERR % '0x25' + '    }\n    widget_play_sound_effect(4);\n    return 0;\n'),
 (0x4a0e90, 241, "for a selected profile: the first spinner list of the first child writes profile byte +0x12d (selection 0..3), that of the second byte +0x12c (selection 0..4); other selections leave them. Returns 1, or 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy'],
  PROFILE + '    int16_t selection;\n\n    if (profile == 0) {\n        return 0;\n    }\n'
  '    selection = first_list_child(widget->first_child)->selection_index;\n    if (selection >= 0 && selection <= 3) {\n        profile[0x12d] = (uint8_t)selection;\n    }\n'
  '    selection = first_list_child(widget->first_child->next_sibling)->selection_index;\n    if (selection >= 0 && selection <= 4) {\n        profile[0x12c] = (uint8_t)selection;\n    }\n    return 1;\n'),
]
for s in SPECS:
    pass
generate([s if len(s) > 5 or 'first_list_child' not in s[4] else s + (FIRST_LIST,) for s in SPECS])
