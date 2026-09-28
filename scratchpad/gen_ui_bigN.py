exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

SPECS = [
 (0x4a7880, 634, "network game options screen, per frame: the first child's spinner list 1..6 sets 0x00719208 to 0x384, 0x708, 0xa8c, 0xe10, 0x1518, 0x2328 (else 0). The second child's list picks which packed option word is edited (0x00692b0c; 1 -> 0x00879f38, else 0x00879f34); when that changes, the third child's list shows the word's low nibble (below 9, else 0). When the third list's selection differs from 0x00692b08 (or the word changed), it becomes the word's low nibble and 0x00692b08, and the bind rows are repopulated from the word. With a low nibble of 8, the lists of the fourth to ninth children fill 3-bit fields at bits 4, 7, 10, 13, 16 and 19. Tail-calls the extended description selection sync.",
  ['unknown_00719208', 'variant_globals', 'unknown_00879f34', 'unknown_00879f38', 'ui_controls_populate_bind_rows',
   'widget_extended_description_sync_selection'],
  '    static const uint32_t delays[] = {0, 0x384, 0x708, 0xa8c, 0xe10, 0x1518, 0x2328};\n'
  '    widget_instance *group = widget->first_child;\n    int16_t selection = first_list_child(group)->selection_index;\n'
  '    uint8_t changed = 0;\n    uint32_t *packed;\n    int32_t which;\n\n'
  '    unknown_00719208 = selection >= 0 && selection <= 6 ? delays[selection] : 0;\n'
  '    group = group->next_sibling;\n    which = unknown_00692b0c;\n    if (first_list_child(group)->selection_index != which) {\n'
  '        which = first_list_child(group)->selection_index;\n        unknown_00692b0c = which;\n        changed = 1;\n    }\n'
  '    packed = which == 1 ? &unknown_00879f38 : &unknown_00879f34;\n'
  '    if (changed) {\n        uint32_t value = *packed;\n\n'
  '        first_list_child(group->next_sibling)->selection_index = (int16_t)((value & 0xf) < 9 ? (value & 0xf) : 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection != variant_team_selection_00692b08 || changed) {\n'
  '        *packed = (*packed & ~0xfu) | ((uint32_t)selection & 0xf);\n        variant_team_selection_00692b08 = selection;\n'
  '        ui_controls_populate_bind_rows(widget, *packed);\n    }\n'
  '    if ((*packed & 0xf) == 8) {\n        int32_t shift;\n\n        for (shift = 4; shift <= 19; shift += 3) {\n'
  '            group = group->next_sibling;\n            selection = first_list_child(group)->selection_index;\n'
  '            *packed = (*packed & ~(7u << shift)) | (((uint32_t)(int32_t)selection << shift) & (7u << shift));\n        }\n    }\n'
  '    widget_extended_description_sync_selection(widget);\n', FIRST_LIST),
]
generate(SPECS)
