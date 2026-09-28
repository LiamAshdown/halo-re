exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
_g = open(r'C:\Users\Liam-\halo-re\scratchpad\gen_ui_bigG.py', encoding='utf-8').read()
exec(_g[_g.index('POP_HISTORY = ('):_g.index('SPECS = [')])

SEL = ('    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n')
TIMEVIEW = ('value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :\n'
            '        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0')

SPECS = [
 (0x49fd30, 693, "for a selected variant, the first spinner lists of the first nine children show: dword +0x84 (0..3, else 0); dword +0x88 (0..3, else 0); dword +0x80 0 / 1 / 2 as 1 / 0 / 2 (else 0); dword +0x8c 1 / 2 as 1 / 2 (else 0); byte +0x7c == 0; dword +0x90 1..16 as 0..15 (else 0); dword +0x58 2 / 5 / 10 / 15 as 1..4 (else 0); byte +0x34 == 0; dword +0x78 0x4650 .. 0x13c68 as 1..6 (else 0). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    int32_t value;\n\n    if (variant == 0) {\n        return 0;\n    }\n    group = widget->first_child;\n'
  '    value = *(int32_t *)(variant + 0x84);\n    first_list_child(group)->selection_index = (int16_t)((uint32_t)value <= 3 ? value : 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x88);\n'
  '    first_list_child(group)->selection_index = (int16_t)((uint32_t)value <= 3 ? value : 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x80);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 0 ? 1 : value == 2 ? 2 : 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x8c);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 1 ? 1 : value == 2 ? 2 : 0);\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)(variant[0x7c] == 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x90);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value > 0 && value <= 0x10 ? value - 1 : 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x58);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 2 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)(variant[0x34] == 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x78);\n'
  '    first_list_child(group)->selection_index = (int16_t)(' + TIMEVIEW + ');\n    return 1;\n', FIRST_LIST),
 (0x49ea50, 786, "for a selected variant, the inverse of 0x49fd30 from the grandparent's first nine spinner lists: 0..3 store dwords +0x84 and +0x88; 0 / 1 / 2 set +0x80 to 1 / 0 / 2; 0 / 1 / 2 set +0x8c to 0 / 1 / 2; 0 / 1 set byte +0x7c to 1 / 0; 0..15 set +0x90 to selection + 1; 0..4 set +0x58 to 1, 2, 5, 10, 15; 0 / 1 set byte +0x34 to 1 / 0; 0..6 set +0x78 to 0, 0x4650 .. 0x13c68 (other selections leave them); then pops the grandparent controller's widget history node (inlined free). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy', 'ui_widget_history', 'widget_memory_pool', 'heap_unlink_block'],
  VARIANT + '    static const int32_t lives[] = {1, 2, 5, 10, 15};\n'
  '    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};\n'
  '    widget_instance *parent = widget->parent->parent;\n    widget_instance *group;\n    int16_t selection;\n\n'
  '    if (variant == 0) {\n        return 0;\n    }\n    group = parent->first_child;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 3) {\n        *(int32_t *)(variant + 0x84) = selection;\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 3) {\n        *(int32_t *)(variant + 0x88) = selection;\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 2) {\n        *(int32_t *)(variant + 0x80) = selection == 0 ? 1 : selection == 1 ? 0 : 2;\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 2) {\n        *(int32_t *)(variant + 0x8c) = selection;\n    }\n' + SEL +
  '    if (selection == 0 || selection == 1) {\n        variant[0x7c] = (uint8_t)(selection == 0);\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 0xf) {\n        *(int32_t *)(variant + 0x90) = selection + 1;\n    }\n' + SEL +
  '    if (selection >= 0 && selection <= 4) {\n        *(int32_t *)(variant + 0x58) = lives[selection];\n    }\n' + SEL +
  '    if (selection == 0 || selection == 1) {\n        variant[0x34] = (uint8_t)(selection == 0);\n    }\n'
  '    selection = first_list_child(group->next_sibling)->selection_index;\n'
  '    if (selection >= 0 && selection <= 6) {\n        *(int32_t *)(variant + 0x78) = times[selection];\n    }\n'
  '    widget_history_pop(parent->controller_index);\n    return 1;\n', FIRST_LIST + '\n' + POP_HISTORY),
]
generate(SPECS)
