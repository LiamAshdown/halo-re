exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
_g = open(r'C:\Users\Liam-\halo-re\scratchpad\gen_ui_bigG.py', encoding='utf-8').read()
exec(_g[_g.index('POP_HISTORY = ('):_g.index('SPECS = [')])

TIMES = '    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};\n'
SEL = ('    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n')

SPECS = [
 (0x49e7e0, 573, "for a selected variant, from the grandparent's first six spinner lists: 0 / 1 set byte +0x7c to 0 / 1; 0 / 1 set byte +0x7e to 1 / 0; 0 / 1 set byte +0x7d to 0 / 1; 0..4 set dword +0x58 to 5, 10, 15, 25, 50; 0 / 1 set byte +0x34 to 1 / 0; 0..6 set dword +0x78 to 0, 0x4650 .. 0x13c68 (other selections leave them); then pops the grandparent controller's widget history node (inlined free). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy', 'ui_widget_history', 'widget_memory_pool', 'heap_unlink_block'],
  VARIANT + '    static const int32_t lives[] = {5, 10, 15, 25, 50};\n' + TIMES +
  '    widget_instance *parent = widget->parent->parent;\n    widget_instance *group;\n    int16_t selection;\n\n'
  '    if (variant == 0) {\n        return 0;\n    }\n    group = parent->first_child;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x7c] = (uint8_t)selection;\n    }\n' + SEL +
  '    if (selection == 0 || selection == 1) {\n        variant[0x7e] = (uint8_t)(selection == 0);\n    }\n' + SEL +
  '    if (selection == 0 || selection == 1) {\n        variant[0x7d] = (uint8_t)selection;\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 4) {\n        *(int32_t *)(variant + 0x58) = lives[selection];\n    }\n' + SEL +
  '    if (selection == 0 || selection == 1) {\n        variant[0x34] = (uint8_t)(selection == 0);\n    }\n'
  '    selection = first_list_child(group->next_sibling)->selection_index;\n'
  '    if (selection >= 0 && selection <= 6) {\n        *(int32_t *)(variant + 0x78) = times[selection];\n    }\n'
  '    widget_history_pop(parent->controller_index);\n    return 1;\n', FIRST_LIST + '\n' + POP_HISTORY),
 (0x49f030, 630, "for a selected variant, the inverse of 0x4a02a0 from the grandparent's first eight spinner lists: 0..3 set dword +0x50 to 0, 1, 3, 5; 0..5 set float +0x54 to 0.5, 1, 1.5, 2, 3, 4; 0 / 1 clear / set flag bit 3 (+0x38); 0..3 set dword +0x48 to 0, 0x96, 0x12c, 0x1c2; the same for +0x44; 0 / 1 set byte +0x40 to 1 / 0; 0 / 1 set / clear flag bit 4; and, when an eighth child exists, 0..3 set dword +0x4c like +0x48. Other selections leave them. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    static const int32_t kills[] = {0, 1, 3, 5};\n    static const float scales[] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};\n'
  '    static const int32_t times[] = {0, 0x96, 0x12c, 0x1c2};\n    widget_instance *group;\n    uint32_t *flags;\n    int16_t selection;\n\n'
  '    if (variant == 0) {\n        return 0;\n    }\n    flags = (uint32_t *)(variant + 0x38);\n    group = widget->parent->parent->first_child;\n'
  '    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 3) {\n        *(int32_t *)(variant + 0x50) = kills[selection];\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 5) {\n        *(float *)(variant + 0x54) = scales[selection];\n    }\n' + SEL +
  '    if (selection == 0) {\n        *flags &= ~8u;\n    } else if (selection == 1) {\n        *flags |= 8;\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 3) {\n        *(int32_t *)(variant + 0x48) = times[selection];\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 3) {\n        *(int32_t *)(variant + 0x44) = times[selection];\n    }\n' + SEL +
  '    if (selection == 0 || selection == 1) {\n        variant[0x40] = (uint8_t)(selection == 0);\n    }\n' + SEL +
  '    if (selection == 0) {\n        *flags |= 0x10;\n    } else if (selection == 1) {\n        *flags &= ~0x10u;\n    }\n'
  '    group = group->next_sibling;\n    if (group != 0) {\n        selection = first_list_child(group)->selection_index;\n'
  '        if (selection >= 0 && selection <= 3) {\n            *(int32_t *)(variant + 0x4c) = times[selection];\n        }\n    }\n    return 1;\n', FIRST_LIST),
]
generate(SPECS)
