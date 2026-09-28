exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'widget_instance_set_state_recursive': 'extern void widget_instance_set_state_recursive(widget_instance *widget, uint8_t state); // 0x498e60',
})

SET_TEXT = ('static void set_text(widget_instance *label, const uint16_t *source)\n{\n'
            '    int32_t length = (int32_t)wcslen((const wchar_t *)source);\n    uint16_t *text;\n\n'
            '    text = (uint16_t *)heap_reallocate(label->text, (uint16_t)(length * 2 + 2), widget_memory_pool);\n    label->text = text;\n'
            '    if (text != 0) {\n        wcsncpy((wchar_t *)text, (const wchar_t *)source, length);\n        text[length] = 0;\n    }\n}\n')

SPECS = [
 (0x4a5740, 906, "multiplayer lobby screen, per frame, with a server (+8) or client (+0xb14) game (the client machine word is read even without a client). Countdown label (third child, 0x20 byte text): client word +0xed8 seconds as L\"-:--\" / L\"0:%02d\" / L\"%02d:%02d\" / L\"%d:%02d:%02d\" (15 characters); 0 selects the second child's entry 1 and hides the countdown; a negative value hides both unless the game's player count (+0x1a0) is at least 2 and its byte +0x138 is not 1. The first child's name label gets L\"\": the binary wcscpy's L\"\" over the read-only literal L\"?\" at 0x0066a80c (which would fault in retail; the standalone image is writable) and then copies that. Then finds the first valid player entry (16 of 0x20 bytes from +0x1a2) of this machine; only a local player 0 entry is kept (the binary indexes a stack array by the player byte, overwriting other locals for players 1..3). Without one: blank player name, frame 0, team list selection 0. With one: its wide name (entry +0) as the player label; without teams (+0x138 == 0) frame 1, else team byte +0x1e 0 / 1 / other gives frame 5 / 4 / 3 and team selection 0 / 1 / 0. Without teams the team list is disabled (state 0, every child state 0).",
  ['network_server_pointer', 'network_client', 'widget_memory_pool', 'heap_reallocate', 'string_format_wide_va_bounded',
   'network_player_entry_validate', 'widget_instance_set_state_recursive'],
  GAME2 + '    widget_instance *first;\n    widget_instance *second;\n    widget_instance *countdown;\n    widget_instance *status;\n'
  '    widget_instance *frame;\n    widget_instance *name;\n    widget_instance *team;\n    int16_t key;\n    int32_t found = -1;\n    int32_t i;\n    uint16_t *text;\n\n'
  '    if (game == 0) {\n        return;\n    }\n    key = *(int16_t *)network_client;\n    first = widget->first_child;\n'
  '    second = first->next_sibling;\n    countdown = second->next_sibling;\n'
  '    text = (uint16_t *)heap_reallocate(countdown->text, 0x20, widget_memory_pool);\n    countdown->text = text;\n'
  '    if (text != 0) {\n        int16_t seconds = *(int16_t *)(network_client + 0xed8);\n\n'
  '        wcsncpy((wchar_t *)text, L"-:--", 0xf); // 0x0066a850\n        second->state = 1;\n        second->selection_index = 0;\n'
  '        countdown->state = 1;\n        if (seconds == 0) {\n            second->selection_index = 1;\n            countdown->state = 0;\n'
  '        } else if (seconds > 0) {\n            if (seconds < 60) {\n'
  '                string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"0:%02d", (int32_t)seconds);\n'
  '            } else if (seconds < 3600) {\n'
  '                string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"%02d:%02d", seconds / 60, seconds % 60);\n'
  '            } else {\n                int32_t hours = seconds / 3600;\n                int32_t minutes = (seconds - hours * 3600) / 60;\n\n'
  '                string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"%d:%02d:%02d", hours, minutes,\n'
  '                    seconds - (hours * 60 + minutes) * 60);\n            }\n'
  '        } else if (*(int16_t *)(game + 0x1a0) < 2 || game[0x138] == 1) {\n            second->state = 0;\n            countdown->state = 0;\n        }\n'
  '        text[0xf] = 0;\n    }\n'
  '    status = first->first_child;\n    frame = status->next_sibling;\n    name = frame->next_sibling;\n'
  '    for (i = 0; i < 0x10; i++) {\n        uint8_t *entry = game + 0x1a2 + i * 0x20;\n\n'
  '        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key) {\n'
  '            if ((int8_t)entry[0x1d] == 0) {\n                found = i;\n            }\n            break;\n        }\n    }\n'
  '    set_text(status, (const uint16_t *)L"");\n    frame->background_bitmap_frame = 0;\n'
  '    frame = name->first_child;\n    team = frame->next_sibling->next_sibling;\n'
  '    if (game[0x138] == 0) {\n        widget_instance *child;\n\n        team->state = 0;\n'
  '        for (child = team->first_child; child != 0; child = child->next_sibling) {\n'
  '            widget_instance_set_state_recursive(child, 0);\n        }\n    }\n'
  '    if (found == -1) {\n        frame->background_bitmap_frame = 0;\n'
  '        text = (uint16_t *)heap_reallocate(frame->next_sibling->text, 2, widget_memory_pool);\n        frame->next_sibling->text = text;\n'
  '        if (text != 0) {\n            text[0] = 0;\n        }\n        team->selection_index = 0;\n        return;\n    }\n'
  '    set_text(frame->next_sibling, (const uint16_t *)(game + 0x1a2 + found * 0x20));\n'
  '    if (game[0x138] == 0) {\n        frame->background_bitmap_frame = 1;\n        return;\n    }\n'
  '    switch ((int8_t)game[0x1a2 + found * 0x20 + 0x1e]) {\n    case 0:\n        frame->background_bitmap_frame = 5;\n        team->selection_index = 0;\n        break;\n'
  '    case 1:\n        frame->background_bitmap_frame = 4;\n        team->selection_index = 1;\n        break;\n'
  '    default:\n        frame->background_bitmap_frame = 3;\n        team->selection_index = 0;\n        break;\n    }\n', SET_TEXT),
]
generate(SPECS)
