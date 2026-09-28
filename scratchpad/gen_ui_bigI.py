exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py', 'gen_ui_bigE.py',
          'gen_ui_bigF.py', 'gen_ui_bigH.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'variant_carousel_slots': 'extern uint8_t variant_carousel_slots[0x1d4]; // 0x00879d60 (variant_carousel_slot[3])',
    'playlist_profiles': ('extern uint8_t playlist_profiles_need_defaults; // 0x0069e8d0\n'
                          'extern void playlist_profile_create_default_profiles_on_disk(void); // 0x53bc70'),
    'saved_game_enumerate_by_type': 'extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count); // 0x53c4e0, blam-cc: EBX capacity_and_count',
    'saved_game_last_mp_variant_read': 'extern uint8_t saved_game_last_mp_variant_read(uint8_t *out_data); // 0x53d3f0',
    'saved_game_find_by_name': 'extern int32_t saved_game_find_by_name(char *name, int16_t type); // 0x53d4a0',
    'ui_list_add_entry': 'extern void ui_list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob, uint32_t data_size, uint8_t is_default); // 0x4a7ba0, blam-cc: EAX group_index, CL is_default',
})

SPECS = [
 (0x49fad0, 497, "for a selected variant, the first spinner lists of the first six children show: byte +0x7c == 1; byte +0x7e 0 / 1 as 1 / 0 (other values leave the list); byte +0x7d == 1; dword +0x58 10 / 15 / 25 / 50 as 1..4 (else 0); byte +0x34 == 0; dword +0x78 0x4650 .. 0x13c68 as 1..6 (else 0). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    int32_t value;\n\n    if (variant == 0) {\n        return 0;\n    }\n    group = widget->first_child;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x7c] == 1);\n    group = group->next_sibling;\n'
  '    if (variant[0x7e] == 0) {\n        first_list_child(group)->selection_index = 1;\n    } else if (variant[0x7e] == 1) {\n'
  '        first_list_child(group)->selection_index = 0;\n    }\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x7d] == 1);\n    group = group->next_sibling;\n'
  '    value = *(int32_t *)(variant + 0x58);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 10 ? 1 : value == 15 ? 2 : value == 25 ? 3 : value == 50 ? 4 : 0);\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)(variant[0x34] == 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x78);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :\n'
  '        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);\n    return 1;\n'),
 (0x49d8b0, 509, "builds the saved variant list: forgets the cached profile slot, fills the variant carousel slots (0x00879d60, 0x1d4 bytes) with 0xff, grows the widget list (+0x44) to 0x190 bytes, creates the default playlist profiles once (0x0069e8d0), enumerates up to 100 variant handles (type 1, built-in) into it, pads to at least three with -1 and stores the count (+0x48). Resets the three ui lists; when the last multiplayer variant record reads and names a saved variant, selects its row. Each row: -1 applies the current custom variant (0x463b90); otherwise a loaded variant is added as a ui list item in group 0 (no team list child) or, with a spinner list first grandchild, group 0 / 1 / 2 by flag bits 8 / 7 (+0x38), named by the variant, id = row, data = the 0x98 byte variant, default when it is the last one. Finally committed selection (+0x3c) = selection and first visible item (+0x3e) = -1; returns 1.",
  ['profile_slot_lookup_cache_00692ac8', 'variant_carousel_slots', 'widget_memory_pool', 'heap_reallocate', 'playlist_profiles',
   'saved_game_enumerate_by_type', 'ui_lists', 'ui_list_current', 'ui_list_has_default', 'saved_game_last_mp_variant_read',
   'saved_game_find_by_name', 'game_engine_apply_current_custom_variant', 'saved_game_get_variant', 'ui_list_add_entry'],
  '    uint8_t grouped = (uint8_t)(widget->first_child != 0 && widget->first_child->first_child != 0 &&\n'
  '        widget->first_child->first_child->widget_type == 2);\n    int32_t *handles;\n    uint16_t count = 0x64;\n'
  '    int32_t last = -1;\n    char last_name[0x100];\n    uint32_t variant[0x26];\n    int32_t i;\n\n'
  '    profile_slot_lookup_cache_00692ac8 = -1;\n    memset(variant_carousel_slots, 0xff, sizeof(variant_carousel_slots));\n'
  '    handles = (int32_t *)heap_reallocate(widget->list_items, 0x190, widget_memory_pool);\n    widget->list_items = handles;\n'
  '    if (handles != 0) {\n        if (playlist_profiles_need_defaults == 1) {\n            playlist_profile_create_default_profiles_on_disk();\n'
  '            playlist_profiles_need_defaults = 0;\n        }\n        saved_game_enumerate_by_type(1, handles, 1, &count);\n'
  '        for (; count < 3; count++) {\n            handles[count] = -1;\n        }\n        widget->item_count = count;\n'
  '        for (i = 0; i < 3; i++) {\n            ui_lists[i].element_size = 0x10;\n            ui_lists[i].count = 0;\n            ui_lists[i].data = 0;\n        }\n'
  '        ui_list_current = -1;\n        ui_list_has_default = 0;\n'
  '        if (saved_game_last_mp_variant_read((uint8_t *)last_name) != 0) {\n            last = saved_game_find_by_name(last_name, 1);\n'
  '            if (last != -1) {\n                uint16_t row;\n\n                for (row = 0; row < count; row++) {\n'
  '                    if (handles[row] == last) {\n                        widget->selection_index = (int16_t)row;\n                        break;\n                    }\n                }\n            }\n        }\n'
  '        for (i = 0; i < count; i++) {\n            if (handles[i] == -1) {\n                game_engine_apply_current_custom_variant();\n'
  '            } else if (saved_game_get_variant(handles[i], variant) != 0) {\n                int32_t group = 0;\n\n'
  '                if (grouped) {\n                    uint32_t flags = variant[0x38 / 4];\n\n'
  '                    group = (flags & 0x100) != 0 ? 0 : (flags & 0x80) != 0 ? 1 : 2;\n                }\n'
  '                ui_list_add_entry(group, (const uint16_t *)variant, i, variant, 0x98, (uint8_t)(last == handles[i]));\n            }\n        }\n    }\n'
  '    *(int16_t *)((uint8_t *)widget + 0x3c) = widget->selection_index;\n    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;\n    return 1;\n'),
]
generate([s if len(s) > 5 or 'first_list_child' not in s[4] else s + (FIRST_LIST,) for s in SPECS])
