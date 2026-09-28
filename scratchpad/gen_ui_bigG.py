exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py', 'gen_ui_bigC2.py', 'gen_ui_bigD.py', 'gen_ui_bigE.py', 'gen_ui_bigF.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

MAPS = ['beavercreek', 'sidewinder', 'damnation', 'ratrace', 'prisoner', 'hangemhigh', 'chillout', 'carousel',
        'boardingaction', 'bloodgulch', 'wizard', 'putput', 'longest']

POP_HISTORY = ('static void widget_history_pop(int16_t controller)\n{\n    widget_history_node *node;\n\n'
               '    if (controller == -1) {\n        controller = 0;\n    }\n    node = ui_widget_history[controller];\n    if (node != 0) {\n'
               '        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);\n        uint32_t size = block->size & 0x7fffffff;\n\n'
               '        ui_widget_history[controller] = node->next;\n        heap_unlink_block(block, widget_memory_pool);\n'
               '        widget_memory_pool->bytes_allocated -= (int32_t)size;\n        widget_memory_pool->allocation_count -= 1;\n    }\n}\n')

SPECS = [
 (0x4a6fa0, 471, "the same map path match as 0x4a6b70 (" + ', '.join(MAPS) + " -> 0..12, icefields 0xd, else 0x13), stored as the background frame (+0x58).",
  ['network_server_pointer', 'network_client', 'strstr'],
  GAME2 + '    static const char *const maps[] = {\n        ' +
  ',\n        '.join(', '.join('"%s"' % m for m in MAPS[k:k + 5]) for k in range(0, len(MAPS), 5)) + '\n    };\n    int16_t i;\n\n'
  '    if (game == 0) {\n        return;\n    }\n    for (i = 0; i < (int16_t)(sizeof(maps) / sizeof(maps[0])); i++) {\n'
  '        if (strstr((char *)(game + 0x84), maps[i]) != 0) {\n            widget->background_bitmap_frame = i;\n            return;\n        }\n    }\n'
  '    widget->background_bitmap_frame = (int16_t)(strstr((char *)(game + 0x84), "icefields") != 0 ? 0xd : 0x13);\n'),
 (0x49e5d0, 477, "for a selected variant, from the grandparent's first four spinner lists: 0 / 1 set byte +0x7c to 1 / 0; 0..4 set dword +0x58 to 1, 2, 5, 10, 15; 0 / 1 set byte +0x34 to 1 / 0; 0..6 set dword +0x78 to 0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68 (other selections leave them). Then pops the grandparent controller's widget history node (the block free is inlined here; same effect as 0x4a1b00). Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy', 'ui_widget_history', 'widget_memory_pool', 'heap_unlink_block'],
  VARIANT + '    static const int32_t lives[] = {1, 2, 5, 10, 15};\n'
  '    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};\n'
  '    widget_instance *parent = widget->parent->parent;\n    widget_instance *group;\n    int16_t selection;\n\n'
  '    if (variant == 0) {\n        return 0;\n    }\n    group = parent->first_child;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x7c] = (uint8_t)(selection == 0);\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection >= 0 && selection <= 4) {\n        *(int32_t *)(variant + 0x58) = lives[selection];\n    }\n'
  '    group = group->next_sibling;\n    selection = first_list_child(group)->selection_index;\n'
  '    if (selection == 0 || selection == 1) {\n        variant[0x34] = (uint8_t)(selection == 0);\n    }\n'
  '    selection = first_list_child(group->next_sibling)->selection_index;\n'
  '    if (selection >= 0 && selection <= 6) {\n        *(int32_t *)(variant + 0x78) = times[selection];\n    }\n'
  '    widget_history_pop(parent->controller_index);\n    return 1;\n', FIRST_LIST + '\n' + POP_HISTORY),
]
generate(SPECS)
