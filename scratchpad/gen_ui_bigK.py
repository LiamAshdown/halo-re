exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())

TIMEMAP = 'value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0'

SPECS = [
 (0x4a02a0, 630, "for a selected variant, the first spinner lists of the first eight children show: dword +0x50 1 / 3 / 5 as 1..3 (else 0); trunc(float +0x54 * -10.0) -10 / -15 / -20 / -30 / -40 as 1..5 (else 0); flag bit 3 (+0x38); dword +0x48 0x96 / 0x12c / 0x1c2 as 1..3 (else 0); the same for +0x44; byte +0x40 == 0; flag bit 4 clear; dword +0x4c as +0x48. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    int32_t value;\n    uint32_t flags;\n\n    if (variant == 0) {\n        return 0;\n    }\n'
  '    flags = *(uint32_t *)(variant + 0x38);\n    group = widget->first_child;\n    value = *(int32_t *)(variant + 0x50);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 1 ? 1 : value == 3 ? 2 : value == 5 ? 3 : 0);\n'
  '    group = group->next_sibling;\n    value = (int32_t)((double)*(float *)(variant + 0x54) * -10.0); // fmul by -10.0f (0x00672da4), _ftol\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == -10 ? 1 : value == -15 ? 2 : value == -20 ? 3 :\n'
  '        value == -30 ? 4 : value == -40 ? 5 : 0);\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)((flags >> 3) & 1);\n    group = group->next_sibling;\n'
  '    value = *(int32_t *)(variant + 0x48);\n    first_list_child(group)->selection_index = (int16_t)(' + TIMEMAP + ');\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x44);\n    first_list_child(group)->selection_index = (int16_t)(' + TIMEMAP + ');\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)(variant[0x40] == 0);\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)(((flags >> 4) & 1) == 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x4c);\n    first_list_child(group)->selection_index = (int16_t)(' + TIMEMAP + ');\n'
  '    return 1;\n', FIRST_LIST),
]
generate(SPECS)
