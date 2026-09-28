exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'video_resolutions': ('extern int32_t video_resolution_count; // 0x007196cc\n'
                          'extern video_resolution video_resolutions[0x20]; // 0x006b6690\n'
                          'extern int32_t video_gamma_setting; // 0x00695464, UNSURE name (its low byte is stored)'),
})

CLAMP = ('static uint8_t clamp_selection(widget_instance *group, int16_t maximum)\n{\n'
         '    int16_t selection = first_list_child(group)->selection_index;\n\n'
         '    return (uint8_t)(selection < 0 ? 0 : selection > maximum ? maximum : selection);\n}\n')

SPECS = [
 (0x4bb360, 622, "for a selected profile (else returns 1), from the grandparent's children's spinner lists: resolution (out of range -> 0) width / height words into +0xa68 / +0xa6a, its refresh rate (out of range -> 0) into +0xa6c, vsync clamped 0..2 into +0xa6f, three flags (+0xa70..+0xa72 = selection != 0), two more clamped 0..2 (+0xa73 / +0xa74), the gamma byte (0x00695464) into +0xa76. Returns whether that mode differs from the current one; when it does not, closes the grandparent restoring the previous widget. Either way plays sound 3 and sets 0x007196d1.",
  ['selected_saved_item', 'saved_item_working_copy', 'video_resolutions', 'video_mode', 'widget_instance_close_and_restore_previous',
   'widget_play_sound_effect', 'ui_flag_007196d1'],
  PROFILE + '    widget_instance *screen = widget->parent->parent;\n    uint8_t result = 1;\n\n    if (profile != 0) {\n'
  '        widget_instance *group = screen->first_child;\n        rasterizer_display_mode mode;\n        int32_t resolution;\n        int32_t refresh;\n\n'
  '        resolution = first_list_child(group)->selection_index;\n        if (resolution < 0 || resolution >= video_resolution_count) {\n            resolution = 0;\n        }\n'
  '        *(uint16_t *)(profile + 0xa68) = (uint16_t)video_resolutions[resolution].width;\n'
  '        *(uint16_t *)(profile + 0xa6a) = (uint16_t)video_resolutions[resolution].height;\n'
  '        group = group->next_sibling;\n        refresh = first_list_child(group)->selection_index;\n'
  '        if (refresh < 0 || (uint32_t)refresh >= video_resolutions[resolution].refresh_rate_count) {\n            refresh = 0;\n        }\n'
  '        *(uint16_t *)(profile + 0xa6c) = (uint16_t)video_resolutions[resolution].refresh_rates[refresh];\n'
  '        group = group->next_sibling;\n        profile[0xa6f] = clamp_selection(group, 2);\n'
  '        group = group->next_sibling;\n        profile[0xa70] = (uint8_t)(first_list_child(group)->selection_index != 0);\n'
  '        group = group->next_sibling;\n        profile[0xa71] = (uint8_t)(first_list_child(group)->selection_index != 0);\n'
  '        group = group->next_sibling;\n        profile[0xa72] = (uint8_t)(first_list_child(group)->selection_index != 0);\n'
  '        group = group->next_sibling;\n        profile[0xa73] = clamp_selection(group, 2);\n'
  '        group = group->next_sibling;\n        profile[0xa74] = clamp_selection(group, 2);\n'
  '        profile[0xa76] = (uint8_t)video_gamma_setting;\n'
  '        mode.width = *(int16_t *)(profile + 0xa68);\n        mode.height = *(int16_t *)(profile + 0xa6a);\n'
  '        mode.refresh_rate = *(int16_t *)(profile + 0xa6c);\n        mode.vsync = (uint8_t)(profile[0xa6f] != 0);\n'
  '        result = rasterizer_display_mode_differs(&mode);\n        if (result == 0) {\n'
  '            widget_instance_close_and_restore_previous(screen);\n        }\n    }\n'
  '    widget_play_sound_effect(3);\n    ui_flag_007196d1 = 1;\n    return result;\n', FIRST_LIST + '\n' + CLAMP),
]
generate(SPECS)
