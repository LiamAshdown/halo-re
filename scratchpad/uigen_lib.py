"""Shared helpers for the ui_event / game_data_input generators."""
import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')
_src = open('scratchpad/gen_ui_small.py', encoding='utf-8').read()
_ns = {}
exec(_src[_src.index('EXT = {'):_src.index('PROFILE =')], _ns)
EXT = dict(_ns['EXT'])
exec(open('scratchpad/whereptr.py').read().split('for a in sys.argv')[0].replace('import struct, sys', 'import struct'), _ns)
rd = _ns['rd']
EVENT_TABLE = {rd(0x6927d0 + 4 * i): i for i in range(0xbe)}
GDI_TABLE = {rd(0x692b18 + 4 * i): i for i in range(0x3b)}

EXT.update({
    'network_session_create': 'extern void *network_session_create(void); // 0x4d8a80, blam-cc: EAX -> client',
    'network_game_mode': 'extern int16_t network_game_mode; // 0x00719720',
    'network_host_handoff_requested': 'extern uint8_t network_host_handoff_requested; // 0x0071c2de',
    'network_session_starting_0071c2dc': 'extern uint8_t network_session_starting_0071c2dc; // 0x0071c2dc, TYPES-GAP',
    'network_game_server_host_create': 'extern int32_t network_game_server_host_create(void); // 0x4ddd40',
    'game_variant_history_current': 'extern int32_t game_variant_history_current; // 0x00687b18',
    'game_engine_sync_variant_defaults': 'extern void game_engine_sync_variant_defaults(void); // 0x45fc80',
    'main_menu_play_title_music': 'extern void main_menu_play_title_music(void); // 0x4993e0',
    'ui_list_current': 'extern int32_t ui_list_current; // 0x00692c04',
    'ui_lists': 'extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)',
    'saved_item_select': 'extern void saved_item_select(int32_t item); // 0x495be0, blam-cc: EBX -> item',
    'quit_confirm_error': ('extern int16_t quit_confirm_error_string_index; // 0x00718fac\n'
                           'extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae\n'
                           'extern uint8_t quit_confirm_error_modal; // 0x00718fb0\n'
                           'extern uint8_t quit_confirm_error_is_error; // 0x00718fb1'),
    'widget_memory_pool': 'extern heap *widget_memory_pool; // 0x006926c4',
    'heap_reallocate': 'extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self',
    'heap_unlink_block': 'extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0',
    'saved_item_has_unsaved_changes': 'extern uint8_t saved_item_has_unsaved_changes(void); // 0x495ea0',
    'saved_item_name_changed': 'extern int32_t saved_item_name_changed(void); // 0x495c90',
    'saved_item_name_edit_begin': 'extern uint8_t saved_item_name_edit_begin(void); // 0x495cf0',
    'player_profile_save': 'extern uint8_t player_profile_save(void); // 0x495d40',
    'widget_close': 'extern void widget_close(widget_instance *widget); // 0x497c00',
    'network_player_entry_validate': 'extern char network_player_entry_validate(void *entry); // 0x4de9f0, blam-cc: EAX -> entry',
    'network_staged_message_commit': ('extern int32_t network_staged_message_commit(void *client, int16_t value); // 0x4da250, blam-cc: ECX client,\n'
                                      '    // AX value (its C still takes only the client and ignores AX: OPEN, networking phase)'),
    'time_query_performance_counter_ms': 'extern uint32_t time_query_performance_counter_ms(void); // 0x449210',
    'multiplayer_host_session_start': 'extern uint8_t multiplayer_host_session_start(void); // 0x49d210',
    'ui_widget_history': 'extern widget_history_node *ui_widget_history[3]; // 0x00718f98',
    '__stricmp': 'extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b',
    'level_select_flags_0071916b': 'extern uint8_t level_select_flags_0071916b; // 0x0071916b, TYPES-GAP',
    'unknown_00719779': 'extern char unknown_00719779[]; // 0x00719779, UNSURE: current scenario/level name buffer',
    'level_select_current_path_00719068': 'extern char level_select_current_path_00719068[0x106]; // 0x00719068, TYPES-GAP',
    'level_select_frame_00719168': 'extern int16_t level_select_frame_00719168; // 0x00719168, TYPES-GAP',
    'player_profile_set_default_audio_options': 'extern uint8_t player_profile_set_default_audio_options(void *profile); // 0x53b240, blam-cc: profile in EAX',
    'saved_player_profile_slots_handle': 'extern int32_t saved_player_profile_slots_handle; // 0x00714dd4',
    'player_profile_auto_select': 'extern void player_profile_auto_select(void); // 0x4952c0',
    'current_game_engine': 'extern void *current_game_engine; // 0x006f1d20 (game_engine_definition *)',
    'game_engine_teams_enabled_flag': 'extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc',
})

PROFILE = '    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;\n'
VARIANT = '    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;\n'
LIST_ID = ('static int32_t list_item_id(int16_t index)\n{\n'
           '    if (index >= 0 && index < ui_lists[ui_list_current].count) {\n'
           '        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;\n    }\n    return -1;\n}\n')
LIST_DATA = ('static void *list_item_data(int16_t index)\n{\n'
             '    if (index >= 0 && index < ui_lists[ui_list_current].count) {\n'
             '        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].data;\n    }\n    return 0;\n}\n')
FIRST_LIST = ('static widget_instance *first_list_child(widget_instance *widget)\n{\n'
              '    widget_instance *child = widget->first_child;\n\n'
              '    while (child != 0 && child->widget_type != 2) {\n        child = child->next_sibling;\n    }\n    return child;\n}\n')
GAME2 = ('    uint8_t *game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8\n'
         '                  : network_client != 0 ? network_client + 0xb14 : 0;\n')
ROOT_CLOSE = ('    root = widget;\n    while (root->parent != 0) {\n        root = root->parent;\n    }\n'
              '    widget_close(root);\n    *out_handled = 1;\n')
QUIT_ERR = ('        quit_confirm_error_string_index = %s;\n        quit_confirm_error_unknown_ae = -1;\n'
            '        quit_confirm_error_modal = 1;\n        quit_confirm_error_is_error = 0;\n')

HDR_EV = '''// ui_event_%(a)x  (not a Ghidra function; ui_event_function_table[%(i)d])
// address 0x%(a)x, size %(s)d bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x%(slot)08x (index %(i)d); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_%(a)x.
%(note)s// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL
'''
HDR_GDI = '''// ui_game_data_input_%(a)x  (not a Ghidra function; game_data_input_function_table[%(i)d])
// address 0x%(a)x, size %(s)d bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x%(slot)08x (index %(i)d); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_%(a)x.
%(note)s// blam-cc: stack -> widget (cdecl)
'''

def note(a, s, text):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (a, a + s - 1, text), 113)
    return ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))

def generate(specs):
    """specs: (addr, size, note, externs, body[, helpers]) -- kind taken from the table the address sits in."""
    n = 0
    for spec in specs:
        a, s, text, ext, body = spec[:5]
        helpers = spec[5] if len(spec) > 5 else ''
        if a in EVENT_TABLE:
            i = EVENT_TABLE[a]
            name = 'ui_event_%x' % a
            hdr = HDR_EV % dict(a=a, i=i, s=s, slot=0x6927d0 + 4 * i, note=note(a, s, text))
            sig = 'uint8_t %s(widget_instance *widget, int16_t *event, uint8_t *out_handled)' % name
        else:
            i = GDI_TABLE[a]
            name = 'ui_game_data_input_%x' % a
            hdr = HDR_GDI % dict(a=a, i=i, s=s, slot=0x692b18 + 4 * i, note=note(a, s, text))
            sig = 'void %s(widget_instance *widget)' % name
        inc = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "cache.h"\n#include "interface.h"\n'
        if 'd3d_' in body or 'rasterizer_display_mode' in body:
            inc += '#include "rasterizer.h"\n'
        if 'memset' in body or 'memcpy' in body:
            inc += '#include <string.h>\n'
        if 'wcs' in body:
            inc += '#include <wchar.h>\n'
        if 'sprintf' in body:
            inc += '#include <stdio.h>\n'
        ex = ''.join(EXT[e] + '\n' for e in ext)
        src = hdr + '\n' + inc + ('\n' + ex if ex else '') + ('\n' + helpers if helpers else '') + '\n' + sig + '\n{\n' + body + '}\n'
        p = 'src/interface/%s.c' % name
        assert not os.path.exists(p), p
        open(p, 'w', encoding='utf-8').write(src)
        n += 1
    print('wrote', n)


def load_ext(path):
    """Merge the first EXT.update({...}) block of another generator script."""
    t = open(path, encoding='utf-8').read()
    i = t.index('EXT.update({')
    j = t.index('\n})\n', i) + 3
    exec(t[i:j], globals())
