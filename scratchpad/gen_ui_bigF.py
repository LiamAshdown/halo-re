exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py', 'gen_ui_bigE.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'tag_instances': 'extern tag_instance *tag_instances; // 0x0087bc14',
    'missing_string_text': 'extern uint16_t missing_string_text[]; // 0x00671fac, L"<missing string>"',
    'strstr': 'extern char *strstr(const char *haystack, const char *needle); // 0x625430',
})

CLAMP = ('static uint8_t clamp_selection(widget_instance *group, int16_t maximum)\n{\n'
         '    int16_t selection = first_list_child(group)->selection_index;\n\n'
         '    return (uint8_t)(selection < 0 ? 0 : selection > maximum ? maximum : selection);\n}\n')

MAPS = ['beavercreek', 'sidewinder', 'damnation', 'ratrace', 'prisoner', 'hangemhigh', 'chillout', 'carousel',
        'boardingaction', 'bloodgulch', 'wizard', 'putput', 'longest']

SPECS = [
 (0x4a24c0, 465, "for a selected profile, from the grandparent's first seven children's spinner lists: bytes +0xb78 / +0xb79 / +0xb7a = selection clamped to 0..10, +0xb7c = (selection != 0), +0xb7d = selection clamped to 0..2, +0xb7b = 1 when the sixth list's selection is at least 1 and +0xb7c is set (else 0), +0xb7f = seventh clamped to 0..2. Returns 1, or 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy'],
  PROFILE + '    widget_instance *group;\n    int16_t selection;\n\n    if (profile == 0) {\n        return 0;\n    }\n'
  '    group = widget->parent->parent->first_child;\n    profile[0xb78] = clamp_selection(group, 10);\n'
  '    group = group->next_sibling;\n    profile[0xb79] = clamp_selection(group, 10);\n'
  '    group = group->next_sibling;\n    profile[0xb7a] = clamp_selection(group, 10);\n'
  '    group = group->next_sibling;\n    profile[0xb7c] = (uint8_t)(first_list_child(group)->selection_index != 0);\n'
  '    group = group->next_sibling;\n    profile[0xb7d] = clamp_selection(group, 2);\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    profile[0xb7b] = (uint8_t)(selection >= 1 && profile[0xb7c] != 0);\n'
  '    group = group->next_sibling;\n    profile[0xb7f] = clamp_selection(group, 2);\n    return 1;\n', FIRST_LIST + '\n' + CLAMP),
 (0x4a0860, 454, "resets the three ui lists; for a selected profile clamps its colour word +0x11a to 0..0x11 and makes it the selection (+0x40), committed selection (+0x3c) and first visible item -1 (+0x3e). Grows the widget list (+0x44) to 0x12 bytes (index i holds i) and adds 0x12 ui list items named from the colors_list unicode string list (ui\\\\shell\\\\main_menu\\\\settings_select\\\\player_setup\\\\player_profile_edit\\\\color_edit\\\\colors_list; each present string is forced terminated, missing ones read L\"<missing string>\"), GlobalAlloc copies with id i, the selection marked default. item_count becomes 0x12. Returns 1.",
  ['ui_lists', 'ui_list_current', 'ui_list_has_default', 'selected_saved_item', 'saved_item_working_copy', 'widget_memory_pool', 'heap_reallocate',
   'tag_lookup', 'tag_instances', 'missing_string_text', 'growable_array_add_element', 'GlobalAlloc'],
  PROFILE + '    uint8_t *indices;\n    datum_index strings;\n    int32_t i;\n\n'
  '    for (i = 0; i < 3; i++) {\n        ui_lists[i].element_size = 0x10;\n        ui_lists[i].count = 0;\n        ui_lists[i].data = 0;\n    }\n'
  '    ui_list_current = -1;\n    ui_list_has_default = 0;\n    if (profile != 0) {\n        int16_t colour = *(int16_t *)(profile + 0x11a);\n\n'
  '        colour = (int16_t)(colour < 0 ? 0 : colour > 0x11 ? 0x11 : colour);\n        *(int16_t *)(profile + 0x11a) = colour;\n'
  '        widget->selection_index = colour;\n        *(int16_t *)((uint8_t *)widget + 0x3c) = *(int16_t *)(profile + 0x11a);\n'
  '        *(int16_t *)((uint8_t *)widget + 0x3e) = -1;\n    }\n'
  '    indices = (uint8_t *)heap_reallocate(widget->list_items, 0x12, widget_memory_pool);\n    widget->list_items = indices;\n'
  '    if (indices == 0) {\n        return 1;\n    }\n'
  '    strings = tag_lookup(0x75737472, "ui\\\\shell\\\\main_menu\\\\settings_select\\\\player_setup\\\\player_profile_edit\\\\color_edit\\\\colors_list"); // \'ustr\', 0x0066a578\n'
  '    for (i = 0; i < 0x12; i++) {\n        uint16_t *text = missing_string_text;\n        uint8_t is_default;\n        uint32_t index;\n\n'
  '        ((uint8_t *)widget->list_items)[i] = (uint8_t)i;\n        if (strings != 0xffffffff) {\n'
  '            uint8_t *list = (uint8_t *)tag_instances[strings & 0xffff].data;\n\n'
  '            if (i < *(int32_t *)list) {\n                uint8_t *element = *(uint8_t **)(list + 4) + i * 0x14;\n'
  '                uint32_t size = *(uint32_t *)element;\n\n                if ((int32_t)size > 0) {\n'
  '                    text = *(uint16_t **)(element + 0xc);\n                    text[(size >> 1) - 1] = 0;\n                }\n            }\n        }\n'
  '        is_default = (uint8_t)(i == widget->selection_index);\n        index = growable_array_add_element(&ui_lists[0]);\n'
  '        if (index != 0xffffffff) {\n            ui_list_item *item = (ui_list_item *)ui_lists[0].data + index;\n            uint16_t *copy;\n\n'
  '            item->data = 0;\n            copy = (uint16_t *)GlobalAlloc(0, (uint32_t)wcslen((const wchar_t *)text) * 2 + 2);\n'
  '            item->name = copy;\n            item->id = i;\n            item->is_default = is_default;\n'
  '            if (is_default) {\n                ui_list_has_default = 1;\n            }\n'
  '            wcscpy((wchar_t *)copy, (const wchar_t *)text);\n        }\n    }\n    widget->item_count = 0x12;\n    return 1;\n'),
 (0x4a6b70, 471, "with a server (+8) or client (+0xb14) game, the first of " + ', '.join(MAPS) + " found (strstr) in its map path (+0x84) gives selection (+0x40) 0..12; icefields gives 0xd; anything else 0x13.",
  ['network_server_pointer', 'network_client', 'strstr'],
  GAME2 + '    static const char *const maps[] = {\n        ' +
  ',\n        '.join(', '.join('"%s"' % m for m in MAPS[k:k + 5]) for k in range(0, len(MAPS), 5)) + '\n    };\n    int16_t i;\n\n'
  '    if (game == 0) {\n        return;\n    }\n    for (i = 0; i < (int16_t)(sizeof(maps) / sizeof(maps[0])); i++) {\n'
  '        if (strstr((char *)(game + 0x84), maps[i]) != 0) {\n            widget->selection_index = i;\n            return;\n        }\n    }\n'
  '    widget->selection_index = (int16_t)(strstr((char *)(game + 0x84), "icefields") != 0 ? 0xd : 0x13);\n'),
]
generate(SPECS)
