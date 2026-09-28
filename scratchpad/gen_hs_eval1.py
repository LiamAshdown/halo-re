exec(open(r'C:\Users\Liam-\halo-re\scratchpad\hsgen_lib.py').read())

R0 = 'hs_thread_return(0, thread_index);\n'
SHORT = lambda e: 'hs_thread_return((int32_t)(uint16_t)(%s), thread_index);\n' % e
BOOL = lambda e: 'hs_thread_return((int32_t)(uint8_t)(%s), thread_index);\n' % e
K_TURN = ('static const uint32_t k_turn_rate_display_bits = 0x431f27aa; // 0x00672c10, about 159.155 (1000 / 2pi)\n')

def simple_call(addr, size, decl, call, note=None):
    emit(addr, size, note or 'evaluates the arguments and calls %s; returns 0.' % call.split('(')[0], decl, call + ';\n' + R0)

def noarg_call(addr, size, decl, call):
    emit(addr, size, 'calls %s; returns 0.' % call.split('(')[0], decl, call + ';\n' + R0, args=False)

out = []
emit(0x47a860, 70, 'hs_object_set_health_fraction (0x488600) with the object and the real.',
     'extern void hs_object_set_health_fraction(datum_index object_index, float fraction); // 0x488600, blam-cc: EAX object, stack fraction\n',
     'hs_object_set_health_fraction((datum_index)arguments[0], *(float *)&arguments[1]);\n' + R0)
simple_call(0x47abc0, 65, 'extern void hs_objects_delete_by_type(uint32_t tag_id); // 0x4887d0, blam-cc: ESI tag_id\n',
            'hs_objects_delete_by_type((uint32_t)arguments[0])')
emit(0x47ac10, 82, 'looks up the named gain (0x488b10) and, when found, stores the real in it; returns 0.',
     'extern float *hs_sound_get_gain_reference(char *name); // 0x488b10, blam-cc: EAX name\n',
     'float *gain = hs_sound_get_gain_reference((char *)arguments[0]);\n\nif (gain != 0) {\n    *gain = *(float *)&arguments[1];\n}\n' + R0)
emit(0x47ac70, 95, 'returns the named gain (0x488b10) as a real, 0.0 when unknown.',
     'extern float *hs_sound_get_gain_reference(char *name); // 0x488b10, blam-cc: EAX name\n',
     'float *gain = hs_sound_get_gain_reference((char *)arguments[0]);\n\n'
     'hs_thread_return(gain != 0 ? *(int32_t *)gain : 0, thread_index);\n')
emit(0x47acd0, 18, 'sets hs_reload_pending (0x006b14a8); returns 0.', 'extern uint8_t hs_reload_pending; // 0x006b14a8\n',
     'hs_reload_pending = 1;\n' + R0, args=False)
noarg_call(0x47acf0, 16, 'extern void hs_doc(void); // 0x484270\n', 'hs_doc()')
emit(0x47ae60, 84, 'returns countdown timer digit (0x5400c0) of the short argument as a short.',
     'extern int16_t numeric_countdown_timer_get_digit(int16_t digit_index); // 0x5400c0, blam-cc: AX\n',
     SHORT('numeric_countdown_timer_get_digit((int16_t)arguments[0])'))
emit(0x47aee0, 18, 'sets numeric_countdown_timer_running (0x00721e54); returns 0.',
     'extern uint8_t numeric_countdown_timer_running; // 0x00721e54\n', 'numeric_countdown_timer_running = 1;\n' + R0, args=False)
noarg_call(0x47b230, 16, 'extern void objects_dump_memory(void); // 0x4fa500\n', 'objects_dump_memory()')
emit(0x47b700, 83, 'returns the scenery\'s remaining animation frames (0x4fa9b0) as a short.',
     'extern uint32_t object_animation_get_frames_remaining(uint32_t object_index); // 0x4fa9b0, blam-cc: EAX\n',
     SHORT('object_animation_get_frames_remaining((uint32_t)arguments[0])'))
for addr, bit, name_note, set_when_true in ((0x47b810, 0x400000, 'unit_can_blink: true clears unit flag 0x400000 (+0x204), false sets it', False),
                                            (0x47bcb0, 0x4000, 'unit_aim_without_turning: true sets unit flag 0x4000 (+0x204), false clears it', True)):
    emit(addr, 130, name_note + '; a none unit is ignored. Returns 0.', X['object_data'],
         'if (arguments[0] != -1) {\n    ' + UNIT_FLAGS.strip() + '\n\n'
         '    if (%s(uint8_t)arguments[1]) {\n        *(uint32_t *)(unit + 0x204) |= 0x%x;\n    } else {\n'
         '        *(uint32_t *)(unit + 0x204) &= ~0x%xu;\n    }\n}\n' % ('' if set_when_true else '!', bit, bit) + R0)
emit(0x47bc20, 133, 'true when the unit\'s animation state byte (+0x2a3) is 0x1c (custom animation); false for none.',
     X['object_data'],
     'uint8_t playing = 0;\n\nif (arguments[0] != -1) {\n    ' + UNIT_FLAGS.strip() + '\n\n    playing = (uint8_t)(unit[0x2a3] == 0x1c);\n}\n' + BOOL('playing'))
emit(0x47c060, 74, 'sets the maximum vitalities of every object in the list (0x561ab0; body and shield reals by value); returns 0.',
     'extern void ai_object_list_initialize_shield_stun_thresholds(datum_index object_list_header_handle, float override_max_body_vitality,\n'
     '    float override_max_shield_vitality); // 0x561ab0, blam-cc: ECX list, stack body, shield\n',
     'ai_object_list_initialize_shield_stun_thresholds((datum_index)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);\n' + R0)
emit(0x47c210, 71, 'stores the base animation state named by the string (0x56eb90) in 0x0069fde0; returns 0.',
     'extern int16_t unit_base_animation_state_from_name(const char *name); // 0x56eb90, blam-cc: EDI name\n'
     'extern int16_t magic_seat_animation_state_0069fde0; // 0x0069fde0, UNSURE name\n',
     'magic_seat_animation_state_0069fde0 = unit_base_animation_state_from_name((const char *)arguments[0]);\n' + R0)
emit(0x47c2d0, 36, 'makes the first player\'s unit (player +0x34) ready its weapon (0x569a20, not forced, no direction); returns 0.',
     'extern data_array *player_data; // 0x0087a480\n'
     'extern uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t forced, const void *direction); // 0x569a20, blam-cc: EDI unit\n',
     'uint8_t *player = (uint8_t *)player_data->data;\n\nunit_try_ready_weapon(*(uint32_t *)(player + 0x34), 0, 0);\n' + R0, args=False)
emit(0x47c580, 101, 'true when both the unit and the weapon definition are given and the unit carries that weapon (0x56d610).',
     'extern uint8_t unit_has_weapon_of_type(uint32_t unit_index, int32_t weapon_group_tag); // 0x56d610, blam-cc: EAX unit, EBX tag\n',
     'uint8_t has = 0;\n\nif (arguments[0] != -1 && arguments[1] != -1) {\n    has = unit_has_weapon_of_type((uint32_t)arguments[0], arguments[1]);\n}\n' + BOOL('has'))
emit(0x47c7a0, 130, 'requests the flashlight on (unit flag 0x10000000, +0x204) or off (flag 0x20000000); a none unit is ignored. Returns 0.',
     X['object_data'],
     'if (arguments[0] != -1) {\n    ' + UNIT_FLAGS.strip() + '\n\n'
     '    *(uint32_t *)(unit + 0x204) |= (uint8_t)arguments[1] ? 0x10000000 : 0x20000000;\n}\n' + R0)
emit(0x47c830, 117, 'returns unit flag bit 19 (+0x204, flashlight on); false for none.', X['object_data'],
     'uint8_t on = 0;\n\nif (arguments[0] != -1) {\n    ' + UNIT_FLAGS.strip() + '\n\n    on = (uint8_t)((*(uint32_t *)(unit + 0x204) >> 0x13) & 1);\n}\n' + BOOL('on'))
emit(0x47d0a0, 68, 'spawns the ai into the object list (0x432a40); returns 0.',
     'extern void ai_object_list_spawn_members(datum_index object_list_header_handle, uint32_t packed_reference); // 0x432a40, blam-cc: EAX, EDI\n',
     'ai_object_list_spawn_members((datum_index)arguments[0], (uint32_t)arguments[1]);\n' + R0, record=0x6589e0)
emit(0x47d1b0, 63, 'clears the orders of every unit in the list (0x432ad0); returns 0.',
     'extern void ai_object_list_clear_orders_with_weapon(datum_index object_list_header_handle); // 0x432ad0, blam-cc: ECX\n',
     'ai_object_list_clear_orders_with_weapon((datum_index)arguments[0]);\n' + R0, record=0x658a40)
emit(0x47d3b0, 133, 'with a valid encounter reference while encounters are live (ai_globals +1): the encounter\'s respawn byte (+0x3c) takes the boolean, its +0x0e word becomes 0x96 and it is activated. Returns 0.',
     X['ai_global_data'] + X['encounter_data'] + 'extern uint8_t encounter_activate(datum_index encounter_index); // 0x437710, blam-cc: ECX\n',
     'if (arguments[0] != -1 && ai_global_data[1] != 0) {\n    uint32_t index = (uint32_t)arguments[0] & 0xffff;\n'
     '    uint8_t *encounter = (uint8_t *)encounter_data->data + index * 0x6c;\n\n    encounter[0x3c] = (uint8_t)arguments[1];\n'
     '    *(int16_t *)((uint8_t *)encounter_data->data + index * 0x6c + 0xe) = 0x96;\n    encounter_activate((datum_index)index);\n}\n' + R0)
emit(0x47d5c0, 68, 'ai_reference_respawn_member (0x432df0) with the ai and the unit; returns 0.',
     'extern void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index); // 0x432df0, blam-cc: EAX, EDI\n',
     'ai_reference_respawn_member((uint32_t)arguments[0], (datum_index)arguments[1]);\n' + R0)
emit(0x47d610, 70, 'ai_object_list_respawn_members (0x432e80) with the list (second argument) and the ai (first); returns 0.',
     'extern void ai_object_list_respawn_members(datum_index object_list_header_handle, uint32_t packed_reference); // 0x432e80, blam-cc: EAX, EBX\n',
     'ai_object_list_respawn_members((datum_index)arguments[1], (uint32_t)arguments[0]);\n' + R0, record=0x658c18)
simple_call(0x47d660, 63, 'extern void ai_reference_mark_squads_unknown_11(uint32_t packed_reference); // 0x432f10, blam-cc: EAX\n',
            'ai_reference_mark_squads_unknown_11((uint32_t)arguments[0])')
emit(0x47da20, 76, 'ai_object_process_nearby_actors (0x433cc0) with the ai, vehicle, seat name and 1; returns 0.',
     'extern void ai_object_process_nearby_actors(uint32_t ai_reference, datum_index vehicle_index, char *seat_name, char allow_boarding_actors); // 0x433cc0, blam-cc: EAX ai\n',
     'ai_object_process_nearby_actors((uint32_t)arguments[0], (datum_index)arguments[1], (char *)arguments[2], 1);\n' + R0)
emit(0x47de80, 123, 'for a live unit (object_try_and_get mask 3): steps the swarm components of its actor (+0x1f4) or, without one, of +0x1f8 (0x407240). Returns 0.',
     'extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack\n'
     'extern void actor_swarm_for_each_component_thunk(uint32_t actor_index); // 0x407240, blam-cc: EAX\n',
     'if (arguments[0] != -1) {\n    uint8_t *unit = (uint8_t *)object_try_and_get((datum_index)arguments[0], 3);\n\n    if (unit != 0) {\n'
     '        if (*(int32_t *)(unit + 0x1f4) != -1) {\n            actor_swarm_for_each_component_thunk(*(uint32_t *)(unit + 0x1f4));\n'
     '        } else if (*(int32_t *)(unit + 0x1f8) != -1) {\n            actor_swarm_for_each_component_thunk(*(uint32_t *)(unit + 0x1f8));\n        }\n    }\n}\n' + R0)
emit(0x47df80, 72, 'ai_unit_set_actor_unknown_0a (0x435540) with the unit and the boolean; returns 0.',
     'extern void ai_unit_set_actor_unknown_0a(datum_index unit_index, uint8_t value); // 0x435540, blam-cc: EAX, stack\n',
     'ai_unit_set_actor_unknown_0a((datum_index)arguments[0], (uint8_t)arguments[1]);\n' + R0)
emit(0x47dfd0, 69, 'squad_members_assign_team_and_request_order (0x435590) with the ai and the state; returns 0.',
     'extern void squad_members_assign_team_and_request_order(uint32_t packed_reference, int16_t value); // 0x435590, blam-cc: EAX, SI\n',
     'squad_members_assign_team_and_request_order((uint32_t)arguments[0], (int16_t)arguments[1]);\n' + R0)
emit(0x47e020, 69, 'squad_members_request_order (0x435630) with the ai and the state; returns 0.',
     'extern void squad_members_request_order(uint32_t packed_reference, int16_t order_code); // 0x435630, blam-cc: EAX, SI\n',
     'squad_members_request_order((uint32_t)arguments[0], (int16_t)arguments[1]);\n' + R0)
emit(0x47e180, 95, 'for a unit: sets bit (team) of the word +0x08 of its ai attention record (0x435900, created on demand); returns 0.',
     'extern uint8_t *ai_object_attention_find_or_create(datum_index object_index); // 0x435900, blam-cc: EDI\n',
     'if (arguments[0] != -1) {\n    uint8_t *record = ai_object_attention_find_or_create((datum_index)arguments[0]);\n\n'
     '    if (record != 0) {\n        *(uint16_t *)(record + 8) |= (uint16_t)(1u << ((int16_t)arguments[1] & 0x1f));\n    }\n}\n' + R0)
emit(0x47e240, 107, 'for a unit and an ai: appends the ai to the (at most 6) enterable actors of the unit\'s ai attention record (+0x0c count, +0x10 list); returns 0.',
     'extern uint8_t *ai_object_attention_find_or_create(datum_index object_index); // 0x435900, blam-cc: EDI\n',
     'if (arguments[0] != -1 && arguments[1] != -1) {\n    uint8_t *record = ai_object_attention_find_or_create((datum_index)arguments[0]);\n\n'
     '    if (record != 0 && *(int16_t *)(record + 0xc) < 6) {\n'
     '        ((int32_t *)(record + 0x10))[*(int16_t *)(record + 0xc)] = arguments[1];\n        *(int16_t *)(record + 0xc) += 1;\n    }\n}\n' + R0)
emit(0x47e2f0, 66, 'ai_unit_dispatch_actor_event_d (0x435a00) with the unit and the object; returns 0.',
     'extern void ai_unit_dispatch_actor_event_d(datum_index unit_index, int32_t unused); // 0x435a00, blam-cc: EAX, ECX\n',
     'ai_unit_dispatch_actor_event_d((datum_index)arguments[0], arguments[1]);\n' + R0)
simple_call(0x47e340, 63, 'extern void ai_unit_clear_actor_vocalization(datum_index unit_index); // 0x435a50, blam-cc: EAX\n',
            'ai_unit_clear_actor_vocalization((datum_index)arguments[0])')
emit(0x47e490, 113, 'for a valid encounter: without a unit its follow mode word (+0x62) becomes 0; with one, 2 and the unit goes to +0x64. Returns 0.',
     X['encounter_data'],
     'if (arguments[0] != -1) {\n    uint8_t *encounter = (uint8_t *)encounter_data->data + ((uint32_t)arguments[0] & 0xffff) * 0x6c;\n\n'
     '    if (arguments[1] == -1) {\n        *(int16_t *)(encounter + 0x62) = 0;\n    } else {\n        *(int16_t *)(encounter + 0x62) = 2;\n'
     '        *(int32_t *)(encounter + 0x64) = arguments[1];\n    }\n}\n' + R0)
emit(0x47e740, 67, 'encounter_set_team (0x435b30) with the ai and the team; returns 0.',
     'extern void encounter_set_team(datum_index encounter_index, int16_t team); // 0x435b30, blam-cc: EAX, CX\n',
     'encounter_set_team((datum_index)arguments[0], (int16_t)arguments[1]);\n' + R0)
emit(0x47e790, 72, 'ai_reference_set_unknown_1cb (0x434d40) with the ai and the boolean; returns 0.',
     'extern void ai_reference_set_unknown_1cb(uint32_t packed_reference, char flag); // 0x434d40, blam-cc: EAX, stack\n',
     'ai_reference_set_unknown_1cb((uint32_t)arguments[0], (char)(uint8_t)arguments[1]);\n' + R0)
emit(0x47e830, 82, 'returns ai_platoon_range_has_available (0x433180) for the ai as a boolean.',
     'extern uint8_t ai_platoon_range_has_available(uint32_t packed_reference); // 0x433180, blam-cc: EAX\n',
     BOOL('ai_platoon_range_has_available((uint32_t)arguments[0])'))
emit(0x47eaa0, 98, 'returns ai_reference_get_stat_pair (0x432f90, stat kind 1, no outputs) for the ai as a short.',
     'extern uint32_t ai_reference_get_stat_pair(uint32_t packed_reference, int16_t stat_kind, int32_t *out_member_count, uint32_t *out_extra); // 0x432f90, blam-cc: EAX, EDI, stack\n',
     SHORT('ai_reference_get_stat_pair((uint32_t)arguments[0], 1, 0, 0)'))
emit(0x47eee0, 72, 'camera_script_set_animation (0x444b30) with the animation graph and the name; returns 0.',
     'extern void camera_script_set_animation(datum_index animation_tag, char *name); // 0x444b30, blam-cc: EBX tag, stack name\n',
     'camera_script_set_animation((datum_index)arguments[0], (char *)arguments[1]);\n' + R0)
noarg_call(0x47f020, 16, 'extern void camera_debug_save_to_file(void); // 0x445880\n', 'camera_debug_save_to_file()')
noarg_call(0x47f030, 16, 'extern void camera_debug_load_from_file(void); // 0x445940\n', 'camera_debug_load_from_file()')
simple_call(0x47f040, 63, 'extern void game_engine_set_variant_by_name(const char *name); // 0x45b920\n',
            'game_engine_set_variant_by_name((const char *)arguments[0])')
emit(0x47f100, 22, 'player control globals (0x006b145c) word +0x34 = -1 (zoom level); returns 0.',
     'extern uint8_t *player_control_globals_ptr; // 0x006b145c\n',
     '*(int16_t *)(player_control_globals_ptr + 0x34) = -1;\n' + R0, args=False)
emit(0x47f500, 39, 'clears 0x0071973c and 0x0071974f, resets the quit prompt string to -1 and sets 0x00719738 (the map reset request); returns 0.',
     'extern uint8_t main_globals_byte_0071973c; // 0x0071973c\nextern uint8_t main_globals_byte_0071974f; // 0x0071974f\n'
     'extern uint16_t split_screen_quit_prompt_string; // 0x00719754\nextern uint8_t unknown_00719738; // 0x00719738, UNSURE\n',
     'main_globals_byte_0071973c = 0;\nmain_globals_byte_0071974f = 0;\nsplit_screen_quit_prompt_string = 0xffff;\nunknown_00719738 = 1;\n' + R0, args=False)
simple_call(0x47f530, 65, 'extern uint8_t main_queue_map_change_by_name_or_clear(char *name); // 0x4c87a0, blam-cc: EDI\n',
            'main_queue_map_change_by_name_or_clear((char *)arguments[0])')
emit(0x47f580, 76, 'a difficulty 0..3 becomes the pending difficulty (0x00696564); returns 0.',
     'extern int16_t pending_difficulty; // 0x00696564\n',
     'int16_t difficulty = (int16_t)arguments[0];\n\nif (difficulty >= 0 && difficulty < 4) {\n    pending_difficulty = difficulty;\n}\n' + R0)
emit(0x47f5d0, 66, 'deliberately crashes: stores the address of "chucky was here! NULL belongs to me!!!!!" at address 0 (the return after it is never reached).',
     '', '*(volatile const char **)0 = "chucky was here! NULL belongs to me!!!!!"; // 0x0066b288\n' + R0)
emit(0x47f6a0, 26, 'prints the build string to the console (console_print_error_va, not cleared first); returns 0.',
     'extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, blam-cc: AL\n',
     'console_print_error_va(0, "halo pc 01.00.10.0621 Apr 16 2014 15:54:48"); // 0x0066b25c\n' + R0, args=False)
emit(0x47f6c0, 18, 'sets 0x00719768 (playback requested); returns 0.', 'extern uint8_t playback_requested_00719768; // 0x00719768, UNSURE name\n',
     'playback_requested_00719768 = 1;\n' + R0, args=False)
noarg_call(0x47f6e0, 16, 'extern void sound_cache_dump_to_file(void); // 0x444240\n', 'sound_cache_dump_to_file()')
emit(0x47fa20, 25, 'clears 0x0071973c and sets 0x0071974f (game lost); returns 0.',
     'extern uint8_t main_globals_byte_0071973c; // 0x0071973c\nextern uint8_t main_globals_byte_0071974f; // 0x0071974f\n',
     'main_globals_byte_0071973c = 0;\nmain_globals_byte_0071974f = 1;\n' + R0, args=False)
simple_call(0x47fe20, 63, 'extern void sound_looping_predict(datum_index looping_definition); // 0x544000, blam-cc: EAX\n',
            'sound_looping_predict((datum_index)arguments[0])')
for addr, fn, fa, glob, gaddr in ((0x480070, 'sound_set_master_gain', 0x548590, 'sound_master_gain', 0x7252ac),
                                  (0x4800e0, 'sound_set_music_gain', 0x548680, 'sound_music_gain', 0x7252a8),
                                  (0x480150, 'sound_set_effects_gain', 0x5487b0, 'sound_effects_gain', 0x7252b0)):
    emit(addr, 67, '%s (0x%x) with the real; returns 0.' % (fn, fa), 'extern void %s(float gain); // 0x%x\n' % (fn, fa),
         '%s(*(float *)&arguments[0]);\n' % fn + R0)
    emit(addr + 0x50, 21, 'returns %s (0x%08x) as a real.' % (glob, gaddr), 'extern float %s; // 0x%08x\n' % (glob, gaddr),
         'hs_thread_return(*(int32_t *)&%s, thread_index);\n' % glob, args=False)
emit(0x4806b0, 23, 'resets the network bandwidth graph history (0x4d8080 on 0x00719ce0); returns 0.',
     'extern uint8_t network_bandwidth_graph_globals[]; // 0x00719ce0\n'
     'extern void network_bandwidth_graph_instance_history_reset(void *graph); // 0x4d8080, blam-cc: ESI\n',
     'network_bandwidth_graph_instance_history_reset(network_bandwidth_graph_globals);\n' + R0, args=False)
emit(0x4806d0, 89, 'returns network_bandwidth_graph_set_units_command (0x4d7d90) for the two strings as a boolean (its low byte).',
     'extern uint32_t network_bandwidth_graph_set_units_command(const char *units_name, const char *direction_name); // 0x4d7d90, blam-cc: ECX, stack\n',
     BOOL('network_bandwidth_graph_set_units_command((const char *)arguments[0], (const char *)arguments[1])'))
emit(0x480730, 74, 'player_update_history_play_local_player (0x4e7730) with the long (EBX); the boolean is pushed but not read. Returns 0.',
     'extern void player_update_history_play_local_player(int32_t target_update_id); // 0x4e7730, blam-cc: EBX\n',
     'player_update_history_play_local_player(arguments[0]);\n' + R0)
simple_call(0x480780, 65, 'extern void message_delta_metrics_dump(char *suffix); // 0x4ec3d0, blam-cc: ESI\n',
            'message_delta_metrics_dump((char *)arguments[0])')
emit(0x4807d0, 64, 'stores the boolean in console_debug_flag_4 (0x0087ac04); returns 0.', 'extern uint8_t console_debug_flag_4; // 0x0087ac04\n',
     'console_debug_flag_4 = (uint8_t)arguments[0];\n' + R0)
emit(0x4812f0, 74, 'cinematic_screen_effect_set_video (0x5122a0) with the short (zero-extended) and the real; returns 0.',
     'extern void cinematic_screen_effect_set_video(int16_t overbright_mode, float noise_intensity); // 0x5122a0\n',
     'cinematic_screen_effect_set_video((int16_t)arguments[0], *(float *)&arguments[1]);\n' + R0)
emit(0x481650, 72, 'sound_driver_set_eax_enabled (0x548200) with the first boolean and the second as force; returns 0.',
     'extern void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force); // 0x548200, blam-cc: stack, CL\n',
     'sound_driver_set_eax_enabled((uint8_t)arguments[0], (uint8_t)arguments[1]);\n' + R0)
emit(0x4816a0, 124, 'the short (out of 0..2 means 2) becomes the supplementary buffer count (0x00746122); when it changed and the boolean is set, re-applies EAX (0x548200 with 0x00746121, forced). Returns 0.',
     'extern int16_t sound_supplementary_buffers_00746122; // 0x00746122, UNSURE name\nextern uint8_t directsound_eax_enabled; // 0x00746121\n'
     'extern void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force); // 0x548200, blam-cc: stack, CL\n',
     'int16_t count = (int16_t)arguments[0];\nuint8_t changed = 0;\n\nif (count < 0 || count > 2) {\n    count = 2;\n}\n'
     'if (sound_supplementary_buffers_00746122 != count) {\n    sound_supplementary_buffers_00746122 = count;\n    changed = 1;\n}\n'
     'if ((uint8_t)arguments[1] && changed) {\n    sound_driver_set_eax_enabled(directsound_eax_enabled, 1);\n}\n' + R0)
emit(0x481720, 33, 'returns the supplementary buffer count (0x00746122) as a short.',
     'extern int16_t sound_supplementary_buffers_00746122; // 0x00746122, UNSURE name\n',
     SHORT('sound_supplementary_buffers_00746122'), args=False)
emit(0x481820, 109, 'true when the device index is below the device count (0x006b1844) and its slot (0x006b1a98 + device * 0x240) is assigned.',
     'extern int32_t input_device_count; // 0x006b1844\nextern uint8_t input_device_to_slot[]; // 0x006b1a98, stride 0x240\n',
     'int32_t device = (int16_t)arguments[0];\nuint8_t active = 0;\n\nif (device < input_device_count) {\n'
     '    active = (uint8_t)(*(int32_t *)(input_device_to_slot + device * 0x240) != -1);\n}\n' + BOOL('active'))
emit(0x481920, 107, 'for a device index below the count with an assigned slot: unassigns it (device slot and joystick_slot_devices[slot] = -1); returns 0.',
     'extern int32_t input_device_count; // 0x006b1844\nextern uint8_t input_device_to_slot[]; // 0x006b1a98, stride 0x240\n'
     'extern int32_t joystick_slot_devices[4]; // 0x006b2ce8\n',
     'int32_t device = (int16_t)arguments[0];\n\nif (device < input_device_count) {\n    int32_t *slot = (int32_t *)(input_device_to_slot + device * 0x240);\n\n'
     '    if (*slot != -1) {\n        int32_t old_slot = *slot;\n\n        *slot = -1;\n        joystick_slot_devices[old_slot] = -1;\n    }\n}\n' + R0)
emit(0x481990, 78, 'evaluates the string and returns -1 as a short.', '', SHORT('-1'))
noarg_call(0x4819e0, 16, 'extern void input_device_list_print(void); // 0x491750\n', 'input_device_list_print()')
simple_call(0x4819f0, 63, 'extern void test_input_device_defaults_find(char *device_id_ansi); // 0x490090, blam-cc: ESI (ECX loaded here)\n',
            'test_input_device_defaults_find((char *)arguments[0])')

# per-controller look rates and player_control_settings fields (0x85c per controller)
def getter(addr, size, base, offset, what, scaled=False, clamp=False):
    ext = X[base] + (K_TURN if scaled else '')
    field = '*(float *)(%s + (int16_t)arguments[0] * 0x85c + 0x%x)' % (('player_control_look_rates_0070facc' if base == 'look_rates' else 'player_control_settings_cache'), offset)
    if scaled:
        body = 'float value = %s * *(const float *)&k_turn_rate_display_bits;\n\nhs_thread_return(*(int32_t *)&value, thread_index);\n' % field
    elif clamp:
        body = ('float value = %s;\n\nif (value < 0.0f) {\n    value = 0.0f;\n} else if (value > 1.0f) {\n    value = 1.0f;\n}\n'
                'hs_thread_return(*(int32_t *)&value, thread_index);\n' % field)
    else:
        body = 'hs_thread_return(*(int32_t *)&%s, thread_index);\n' % field
    note = 'returns %s of the controller (%s +0x%x)%s.' % (what, '0x0070facc' if base == 'look_rates' else 'player_control_settings', offset,
            ' times 1000/2pi (0x00672c10)' if scaled else ' clamped to 0..1 (NaN passes)' if clamp else '')
    emit(addr, size, note, ext, body)

def setter(addr, size, base, offset, what, convert=None):
    ext = X[base] + (convert[1] if convert else '')
    target = '*(float *)(%s + slot * 0x85c + 0x%x)' % (('player_control_look_rates_0070facc' if base == 'look_rates' else 'player_control_settings_cache'), offset)
    value = '%s(*(float *)&arguments[1])' % convert[0] if convert else '*(float *)&arguments[1]'
    emit(addr, size, 'for a controller 0..3 sets %s (+0x%x)%s; returns 0.' % (what, offset, (' through ' + convert[0]) if convert else ''), ext,
         'int16_t slot = (int16_t)arguments[0];\n\nif (slot >= 0 && slot < 4) {\n    %s = %s;\n}\n' % (target, value) + R0)

CLAMP = ('input_clamp_unit_float', 'extern float input_clamp_unit_float(float value); // 0x48c8a0\n')
TURN = ('input_sensitivity_to_turn_rate', 'extern float input_sensitivity_to_turn_rate(float sensitivity); // 0x48c8e0\n')
getter(0x481a30, 75, 'look_rates', 0, 'the yaw rate')
getter(0x481a80, 75, 'look_rates', 4, 'the pitch rate')
setter(0x481ad0, 88, 'look_rates', 0, 'the yaw rate')
setter(0x481b30, 88, 'look_rates', 4, 'the pitch rate')
getter(0x481b90, 147, 'settings', 0x810, 'the digital forward throttle', clamp=True)
setter(0x481c30, 97, 'settings', 0x810, 'the digital forward throttle', CLAMP)
getter(0x481ca0, 147, 'settings', 0x814, 'the digital strafe throttle', clamp=True)
setter(0x481d40, 97, 'settings', 0x814, 'the digital strafe throttle', CLAMP)
getter(0x481db0, 83, 'settings', 0x818, 'the digital yaw increment', scaled=True)
setter(0x481e10, 97, 'settings', 0x818, 'the digital yaw increment', TURN)
getter(0x481e80, 83, 'settings', 0x81c, 'the digital pitch increment', scaled=True)
setter(0x481ee0, 97, 'settings', 0x81c, 'the digital pitch increment', TURN)
getter(0x481f50, 75, 'settings', 0x820, 'the mouse forward threshold')
setter(0x481fa0, 88, 'settings', 0x820, 'the mouse forward threshold')
getter(0x482000, 75, 'settings', 0x824, 'the mouse strafe threshold')
setter(0x482050, 88, 'settings', 0x824, 'the mouse strafe threshold')
getter(0x4820b0, 83, 'settings', 0x828, 'the mouse yaw scale', scaled=True)
setter(0x482110, 97, 'settings', 0x828, 'the mouse yaw scale', TURN)
getter(0x482180, 83, 'settings', 0x82c, 'the mouse pitch scale', scaled=True)
setter(0x4821e0, 97, 'settings', 0x82c, 'the mouse pitch scale', TURN)
getter(0x482250, 75, 'settings', 0x830, 'the gamepad forward threshold')
emit(0x4822a0, 71, 'input_joystick_set_axis_scale_x (0x48c930) with the controller and the real (no range check); returns 0.',
     'extern void input_joystick_set_axis_scale_x(int16_t slot, float value); // 0x48c930, blam-cc: CX, stack\n',
     'input_joystick_set_axis_scale_x((int16_t)arguments[0], *(float *)&arguments[1]);\n' + R0)
getter(0x4822f0, 75, 'settings', 0x834, 'the gamepad strafe threshold')
emit(0x482340, 71, 'input_joystick_set_axis_scale_y (0x48c9a0) with the controller and the real (no range check); returns 0.',
     'extern void input_joystick_set_axis_scale_y(int16_t slot, float value); // 0x48c9a0, blam-cc: CX, stack\n',
     'input_joystick_set_axis_scale_y((int16_t)arguments[0], *(float *)&arguments[1]);\n' + R0)
emit(0x482390, 66, 'evaluates the controller and returns 0.0.', '', R0)
print('part 1 done')
