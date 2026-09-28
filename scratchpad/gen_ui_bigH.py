exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py', 'gen_ui_bigE.py', 'gen_ui_bigF.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)
_g = open(r'C:\Users\Liam-\halo-re\scratchpad\gen_ui_bigG.py', encoding='utf-8').read()
exec(_g[_g.index('POP_HISTORY = ('):_g.index('SPECS = [')])

EXT.update({
    'ui_cursor': ('extern int32_t ui_cursor_x; // 0x00718f84\n'
                  'extern int32_t ui_cursor_y; // 0x00718f88'),
    'input_queue': ('extern uint8_t input_event_queue_active; // 0x00712cc0\n'
                    'extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // 0x492340, blam-cc: EAX, EDI'),
})

TIMES = '    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};\n'

ABS = ('static void widget_absolute_origin(widget_instance *widget, int16_t *x, int16_t *y)\n{\n'
       '    *x = 0;\n    *y = 0;\n    for (; widget != 0; widget = widget->parent) {\n'
       '        *x = (int16_t)(*x + widget->local_x);\n        *y = (int16_t)(*y + widget->local_y);\n    }\n}\n')

SPECS = [
 (0x49e300, 626, "for a selected variant, from the grandparent's first six spinner lists: 0 / 1 set byte +0x7c to 1 / 0; 0..5 set dword +0x80 to 0, 0x708, 0xe10, 0x1518, 0x2328, 0x4650; 0 / 1 set byte +0x7e to 1 / 0; the same for +0x7f; 0..4 set dword +0x58 to 1, 3, 5, 10, 15; 0..6 set dword +0x78 to 0, 0x4650 .. 0x13c68 (other selections leave them); then pops the grandparent controller's widget history node (the block free inlined, as 0x4a1b00). Returns 1, also without a variant (then nothing is popped).",
  ['selected_saved_item', 'saved_item_working_copy', 'ui_widget_history', 'widget_memory_pool', 'heap_unlink_block'],
  VARIANT + '    static const int32_t delays[] = {0, 0x708, 0xe10, 0x1518, 0x2328, 0x4650};\n'
  '    static const int32_t lives[] = {1, 3, 5, 10, 15};\n' + TIMES +
  '    widget_instance *parent = widget->parent->parent;\n    widget_instance *group;\n    int16_t selection;\n\n'
  '    if (variant == 0) {\n        return 1;\n    }\n    group = parent->first_child;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x7c] = (uint8_t)(selection == 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 5) {\n        *(int32_t *)(variant + 0x80) = delays[selection];\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x7e] = (uint8_t)(selection == 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x7f] = (uint8_t)(selection == 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 4) {\n        *(int32_t *)(variant + 0x58) = lives[selection];\n    }\n'
  '    selection = first_list_child(group->next_sibling)->selection_index;\n'
  '    if (selection >= 0 && selection <= 6) {\n        *(int32_t *)(variant + 0x78) = times[selection];\n    }\n'
  '    widget_history_pop(parent->controller_index);\n    return 1;\n', FIRST_LIST + '\n' + POP_HISTORY),
 (0x49edc0, 557, "for a selected variant, from the grandparent's first five spinner lists: 0..2 set dword +0x80 to 0..2; 0..2 set dword +0x7c to 0..2; 0..5 set dword +0x58 to 1, 3, 5, 10, 15, 25; 0 / 1 set byte +0x34 to 1 / 0; 0..6 set dword +0x78 to 0, 0x4650 .. 0x13c68 (other selections leave them); then pops the grandparent controller's widget history node (inlined free). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy', 'ui_widget_history', 'widget_memory_pool', 'heap_unlink_block'],
  VARIANT + '    static const int32_t lives[] = {1, 3, 5, 10, 15, 25};\n' + TIMES +
  '    widget_instance *parent = widget->parent->parent;\n    widget_instance *group;\n    int16_t selection;\n\n'
  '    if (variant == 0) {\n        return 0;\n    }\n    group = parent->first_child;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 2) {\n        *(int32_t *)(variant + 0x80) = selection;\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 2) {\n        *(int32_t *)(variant + 0x7c) = selection;\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 5) {\n        *(int32_t *)(variant + 0x58) = lives[selection];\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x34] = (uint8_t)(selection == 0);\n    }\n'
  '    selection = first_list_child(group->next_sibling)->selection_index;\n'
  '    if (selection >= 0 && selection <= 6) {\n        *(int32_t *)(variant + 0x78) = times[selection];\n    }\n'
  '    widget_history_pop(parent->controller_index);\n    return 1;\n', FIRST_LIST + '\n' + POP_HISTORY),
 (0x4a1dc0, 545, "mouse click on a spinner: with the widget's absolute origin, a cursor (0x00718f84 / 0x00718f88) inside the definition's left arrow box (+0x174 top, +0x178 bottom / right; the left edge is 0) steps the selection back (wrapping to item_count - 1), sets scroll +0x42 = -4 and +0x54 = -1 and plays sound 1; inside the right arrow box (+0x17c top / left, +0x180 bottom; right edge 640) steps it forward (wrapping to 0) with +0x42 = 4, +0x54 = 1, sound 1. Otherwise, over a child's bounds (definition +0x24 top / left, +0x28 bottom / right at its absolute origin) pushes an input event (kind 3, code 0, pressed 1) into queue 0 when the queue is up. Returns 1.",
  ['tag_instances', 'ui_cursor', 'widget_play_sound_effect', 'input_queue'],
  '    uint8_t *definition = (uint8_t *)tag_instances[widget->definition & 0xffff].data;\n    int32_t x = ui_cursor_x;\n    int32_t y = ui_cursor_y;\n'
  '    int16_t origin_x;\n    int16_t origin_y;\n    widget_instance *child;\n\n    widget_absolute_origin(widget, &origin_x, &origin_y);\n'
  '    if (x >= 0 && x <= (int16_t)(*(int16_t *)(definition + 0x17a) + origin_x) &&\n'
  '        y >= (int16_t)(*(int16_t *)(definition + 0x174) + origin_y) && y <= (int16_t)(*(int16_t *)(definition + 0x178) + origin_y)) {\n'
  '        int32_t selection = widget->selection_index - 1;\n\n        if (selection < 0) {\n            selection = widget->item_count - 1;\n        }\n'
  '        if (selection != widget->selection_index) {\n            widget->selection_index = (int16_t)selection;\n            widget->scroll_blink = -4;\n'
  '            widget->unknown_54 = -1;\n            widget_play_sound_effect(1);\n        }\n        return 1;\n    }\n'
  '    if (x >= (int16_t)(*(int16_t *)(definition + 0x17e) + origin_x) && x <= 0x280 &&\n'
  '        y >= (int16_t)(*(int16_t *)(definition + 0x17c) + origin_y) && y <= (int16_t)(*(int16_t *)(definition + 0x180) + origin_y)) {\n'
  '        int32_t selection = widget->selection_index + 1;\n\n        if (selection >= widget->item_count) {\n            selection = 0;\n        }\n'
  '        if (selection != widget->selection_index) {\n            widget->selection_index = (int16_t)selection;\n            widget->scroll_blink = 4;\n'
  '            widget->unknown_54 = 1;\n            widget_play_sound_effect(1);\n        }\n        return 1;\n    }\n'
  '    for (child = widget->first_child; child != 0; child = child->next_sibling) {\n'
  '        uint8_t *bounds = (uint8_t *)tag_instances[child->definition & 0xffff].data;\n        int16_t cx;\n        int16_t cy;\n\n'
  '        widget_absolute_origin(child, &cx, &cy);\n'
  '        if (x >= (int16_t)(*(int16_t *)(bounds + 0x26) + cx) && x <= (int16_t)(*(int16_t *)(bounds + 0x2a) + cx) &&\n'
  '            y >= (int16_t)(*(int16_t *)(bounds + 0x24) + cy) && y <= (int16_t)(*(int16_t *)(bounds + 0x28) + cy)) {\n'
  '            if (input_event_queue_active != 0) {\n                ui_input_event queued;\n\n                memset(&queued, 0, sizeof(queued));\n'
  '                queued.kind = 3;\n                queued.code = 0;\n                queued.pressed = 1;\n                input_queue_push_event(0, &queued);\n            }\n'
  '            return 1;\n        }\n    }\n    return 1;\n', ABS),
]
generate([s if len(s) > 5 or 'first_list_child' not in s[4] else s + (FIRST_LIST,) for s in SPECS])
