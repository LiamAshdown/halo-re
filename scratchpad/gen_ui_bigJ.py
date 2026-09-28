exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py', 'gen_ui_bigE.py',
          'gen_ui_bigF.py', 'gen_ui_bigH.py', 'gen_ui_bigI.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

CLICK = ('static void row_clicked(widget_instance *list, int32_t row, int32_t old_committed, uint8_t double_click)\n{\n'
         '    widget_play_sound_effect(2);\n    *(int16_t *)((uint8_t *)list + 0x3c) = (int16_t)row;\n'
         '    if (double_click && old_committed == row) {\n        ui_input_event queued;\n\n        memset(&queued, 0, sizeof(queued));\n'
         '        queued.kind = 5;\n        input_queue_push_event(0, &queued);\n    }\n}\n')

SPECS = [
 (0x49f680, 549, "for a selected variant, the first spinner lists of the first six children show: byte +0x7c == 0; dword +0x80 0x708 / 0xe10 / 0x1518 / 0x2328 / 0x4650 as 1..5 (else 0); byte +0x7e == 0; byte +0x7f == 0; dword +0x58 3 / 5 / 10 / 15 as 1..4 (else 0); dword +0x78 0x4650 .. 0x13c68 as 1..6 (else 0). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    int32_t value;\n\n    if (variant == 0) {\n        return 0;\n    }\n    group = widget->first_child;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x7c] == 0);\n    group = group->next_sibling;\n'
  '    value = *(int32_t *)(variant + 0x80);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 0x708 ? 1 : value == 0xe10 ? 2 : value == 0x1518 ? 3 :\n'
  '        value == 0x2328 ? 4 : value == 0x4650 ? 5 : 0);\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x7e] == 0);\n    group = group->next_sibling;\n'
  '    first_list_child(group)->selection_index = (int16_t)(variant[0x7f] == 0);\n    group = group->next_sibling;\n'
  '    value = *(int32_t *)(variant + 0x58);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 3 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);\n'
  '    group = group->next_sibling;\n    value = *(int32_t *)(variant + 0x78);\n'
  '    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :\n'
  '        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);\n    return 1;\n'),
 (0x4a3540, 577, "a click on a row of a scrolling list (the parent): the definition's row count (+0x3e0, less one with a spinner-list header row) decides whether the list overflows (definition flag bit 3 clear and item_count above rows - 1), leaving rows - 3 (arrows shown) or rows - 1 visible data rows (at most item_count). The widget's position among the parent's children (the last child is never matched) maps to a data row starting at the first visible item (+0x3e). The up arrow slot (first child, or second with a header) scrolls up by the visible rows (floor 0); the down arrow slot (second to last child) scrolls down (capped at item_count - rows); both with sound 2. Other rows (or any row when not overflowing) play sound 2 and commit the row (+0x3c); a double click (event kind 3 with code 0 or 0xc, more than 250 ms after the widget's creation time, on the already committed row) pushes an event of kind 5 into queue 0. Returns 1.",
  ['tag_instances', 'time_query_performance_counter_ms', 'widget_play_sound_effect', 'input_queue'],
  '    widget_instance *list = widget->parent;\n    uint8_t *definition = (uint8_t *)tag_instances[list->definition & 0xffff].data;\n'
  '    int32_t rows = *(int32_t *)(definition + 0x3e0);\n    int32_t first_visible = *(int16_t *)((uint8_t *)list + 0x3e);\n'
  '    int32_t committed = *(int16_t *)((uint8_t *)list + 0x3c);\n    widget_instance *child = list->first_child;\n'
  '    uint8_t header = (uint8_t)(child != 0 && child->first_child != 0 && child->first_child->widget_type == 2);\n'
  '    uint8_t double_click = 0;\n    uint8_t fits;\n    int32_t shown;\n    int32_t row;\n    int32_t position;\n\n'
  '    if (event[0] == 3 && (((uint8_t *)event)[4] == 0 || ((uint8_t *)event)[4] == 0xc) &&\n'
  '        time_query_performance_counter_ms() - (uint32_t)widget->creation_time > 0xfa) {\n        double_click = 1;\n    }\n'
  '    if (header) {\n        rows--;\n    }\n'
  '    fits = (uint8_t)((*(uint8_t *)(definition + 0x150) & 8) != 0 || (int32_t)list->item_count <= rows - 1);\n'
  '    shown = rows - (fits ? 1 : 3);\n    if (shown > (int32_t)list->item_count) {\n        shown = list->item_count;\n    }\n'
  '    row = first_visible;\n    for (position = 0; child != 0 && child->next_sibling != 0; position++, row++, child = child->next_sibling) {\n'
  '        if (child == widget) {\n            break;\n        }\n'
  '        if ((position == 0 && (header || !fits)) || (position == 1 && !fits && header)) {\n            row--;\n        }\n    }\n'
  '    if (child == 0 || child->next_sibling == 0) {\n        return 1;\n    }\n'
  '    if ((position == 0 && !header) || (position == 1 && header)) {\n        if (fits) {\n            row_clicked(list, row, committed, double_click);\n            return 1;\n        }\n'
  '        first_visible = first_visible + 1 - shown;\n        if (first_visible < 0) {\n            first_visible = 0;\n        }\n'
  '        *(int16_t *)((uint8_t *)list + 0x3e) = (int16_t)first_visible;\n        widget_play_sound_effect(2);\n        return 1;\n    }\n'
  '    if (child->next_sibling->next_sibling == 0 && !fits) {\n        int32_t last = first_visible + shown - 1;\n\n'
  '        if (last >= (int32_t)list->item_count - shown) {\n            last = (int32_t)list->item_count - shown;\n        }\n'
  '        *(int16_t *)((uint8_t *)list + 0x3e) = (int16_t)last;\n        widget_play_sound_effect(2);\n        return 1;\n    }\n'
  '    row_clicked(list, row, committed, double_click);\n    return 1;\n', CLICK),
]
generate([s if len(s) > 5 or 'first_list_child' not in s[4] else s + (FIRST_LIST,) for s in SPECS])
