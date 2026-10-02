"""Checker for globals->C slice 5 (stdlib only, never reads retail data).

For every global converted into standalone/data/slice05.c it checks
  * the EQU/PUBLIC lines are gone from standalone/globals.asm and the C file defines the name exactly once, with the
    original address in its trailing comment;
  * the initial bytes: the original address range lies past the initialised part of the .data piece
    (standalone/image/pieces.json), i.e. it is BSS and zero in the data image, which is what a C definition without an
    initialiser is; a range that did fall inside a piece would be decoded from standalone/image/data.asm (not needed
    for this slice, reported as an error);
  * no converted object, with its sizeof (table below, from the compiled definitions), overlaps another converted
    object or a symbol still defined as EQU in globals.asm;
  * with a linked map (build/s05/**/halo_rebuilt.map, or argv[1]) every name is defined once and the converted objects
    do not overlap each other at their new addresses.
Usage: python tools/globals_check_slice05.py [map file]"""
import glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SA = os.path.join(ROOT, "standalone")

# (original address, name, sizeof in bytes)
GLOBALS = [
    (0x006b1358, "game_engine_bucket_scores_extra",               64),
    (0x006b139c, "race_used_locations",                           32),
    (0x006b13bc, "race_used_location_count",                       4),
    (0x006b13c0, "race_vehicle_counts",                           16),
    (0x006b1458, "game_engine_attribute_enabled",                  4),
    (0x006b145c, "player_control_globals_ptr",                     4),
    (0x006b1460, "machine_to_player",                             64),
    (0x006b14a0, "hs_autocomplete_maximum_count",                  2),
    (0x006b14a4, "hs_autocomplete_prefix",                         4),
    (0x006b14a8, "hs_reload_pending",                              1),
    (0x006b14ac, "hs_autocomplete_gametype_mask",                  2),
    (0x006b14b0, "hs_autocomplete_count",                          2),
    (0x006b14b4, "hs_autocomplete_results",                        4),
    (0x006b14b8, "hs_compiling",                                   1),
    (0x006b14bc, "hs_compiled_source_length",                      4),
    (0x006b14c0, "hs_compiled_source",                             4),
    (0x006b14d0, "hs_syntax_data_dirty",                           1),
    (0x006b14d4, "hs_compile_error",                               4),
    (0x006b14d8, "hs_compile_error_offset",                        4),
    (0x006b14dc, "hs_compile_error_buffer",                      256),
    (0x006b15dc, "hs_compiled_source_owned",                       1),
    (0x006b15dd, "hs_compile_release_source",                      1),
    (0x006b15de, "hs_blocking_forbidden",                          1),
    (0x006b15df, "hs_set_forbidden",                               1),
    (0x006b15e0, "hs_postprocessing",                              1),
    (0x006b15e4, "hs_comparison_types",                            4),
    (0x006b15e8, "hs_runtime_active",                              1),
    (0x006b15ea, "hs_current_thread_index",                        2),
    (0x006b15f8, "input_acquired",                                 1),
    (0x006b15f9, "input_suppressed",                               1),
    (0x006b15fc, "direct_input",                                   4),
    (0x006b1600, "key_block_timers",                              32),
    (0x006b1620, "key_frames",                                   109),
    (0x006b168d, "key_release_pending",                          109),
    (0x006b16fa, "key_event_read_index",                           2),
    (0x006b16fc, "key_event_count",                                2),
    (0x006b16fe, "key_events",                                   256),
    (0x006b1800, "keyboard_device",                                4),
    (0x006b1804, "mouse_device",                                   4),
    (0x006b1808, "mouse_wheel_granularity",                        4),
    (0x006b180c, "live_mouse_state",                              28),
    (0x006b1828, "mouse_neutral_state",                           28),
    (0x006b1844, "input_device_count",                             4),
    (0x006b1848, "joystick_devices",                              32),
    (0x006b2a68, "joystick_states",                              640),
    (0x006b2ce8, "joystick_slot_devices",                         16),
    (0x006b2cf8, "joystick_neutral_state",                       160),
    (0x006b2d98, "first_person_weapon_interfaces",                 4),
    (0x006b2dcc, "console_input_handle",                           4),
    (0x006b2dd0, "console_output_handle",                          4),
    (0x006b2dd8, "console_window_title",                          32),
    (0x006b2df8, "console_last_line",                            256),
    (0x006b2ef8, "console_last_cursor_column",                     4),
    (0x006b2efc, "terminal_initialized",                           1),
    (0x006b2f00, "terminal_messages",                              4),
    (0x006b2f04, "console_message_head",                           4),
    (0x006b2f08, "console_message_tail",                           4),
    (0x006b2f0c, "console_active",                                 4),
    (0x006b2f10, "console_caret_visible",                          1),
    (0x006b2f14, "console_caret_blink_time",                       4),
    (0x006b2f18, "console_win32_attached",                         1),
    (0x006b2f1c, "console_rcon_handle",                            4),
    (0x006b2f20, "previous_mouse_y",                               4),
    (0x006b2f24, "previous_mouse_x",                               4),
    (0x006b2f28, "progress_screen_text",                          64),
    (0x006b2f68, "progress_screen_subtext",                      128),
    (0x006b2fe8, "formatted_prompt_scratch",                    2048),
    (0x006b37e8, "hud_text_message_queue",                        12),
    (0x006b380c, "server_list_entries_006b380c",                  36),
    (0x006b3830, "ui_lists",                                      36),
    (0x006b3854, "ui_player_number_text",                          4),
    (0x006b3858, "chat_dialog_open",                               4),
    (0x006b385c, "chat_scope_active",                              4),
    (0x006b38e4, "chat_listbox_x",                                 4),
    (0x006b38e8, "chat_listbox_y",                                 4),
    (0x006b38ec, "chat_listbox_width",                             4),
    (0x006b38f0, "chat_listbox_height",                            4),
    (0x006b38f4, "unknown_006b38f4",                               4),
    (0x006b3914, "unknown_006b3914",                               4),
    (0x006b3a20, "hud_chat_message_expiry",                       32),
    (0x006b3a40, "hud_messaging",                                  4),
    (0x006b3a44, "hud_waypoints",                                  4),
    (0x006b42d8, "controls_available_gamepads",                 4352),
    (0x006b53d8, "controls_assigned_gamepads",                  2176),
    (0x006b5c58, "unicode_string_list_scratch_buffer",           512),
    (0x006b5e90, "string_widen_scratch",                        2048),
    (0x006b6690, "video_resolutions",                           2432),
    (0x006b7010, "ui_video_requested_display_mode_006b7010",      16),
    (0x006b7020, "console_globals_data",                        2500),
    (0x006b79e8, "pregame_render_view",                          172),
    (0x006b7a94, "timedemo_pixel_shader_version",                 20),
    (0x006b7aa8, "periodic_function_tables",                      48),
    (0x006b7ad8, "transition_function_tables",                    24),
    (0x006b7af0, "periodic_functions_initialized",                 1),
    (0x006b7af4, "sphere_point_table",                             4),
    (0x006b7af8, "sphere_point_table_count",                       2),
    (0x006b7b00, "crc32_lookup_table",                          1024),
    (0x006b7f00, "data_packet_group_error",                        4),
    (0x006b7f08, "model_render_default_function_values",          16),
    (0x006b7f18, "model_render_default_effect",                   40),
    (0x006b7f40, "model_render_default_region_permutations",      32),
    (0x006b7f60, "model_render_default_change_colors",            48),
    (0x006b7f90, "update_server_last_log_ms",                      4),
    (0x006b7f98, "network_challenge_packet_block",                 2),
    (0x006b7f9a, "network_broadcast_body",                      1536),
    (0x006b859c, "ban_list",                                      12),
    (0x006b85b8, "network_log_path_buffer",                      260),
    (0x006b86c0, "message_delta_parameters",                     768),
    (0x006b89c0, "message_delta_field_changed_flags",             64),
    (0x006b8a00, "object_sound_event_last_tick",                   4),
    (0x006b8c60, "object_unknown_006b8c60",                        4),
    (0x006b8c68, "default_collision_material",                    72),
    (0x006b8cb0, "object_visibility_computed_mask",                2),
    (0x006b8cb4, "object_memory_pool",                             4),
    (0x006b8cb8, "object_name_list",                               4),
    (0x006b8cbc, "object_globals_pointer",                         4),
    (0x006b8cc0, "object_marker_scratch",                        176),
    (0x006b8d70, "light_volume_instances",                         4),
    (0x006b8d74, "lightning_instances",                            4),
    (0x006b8d78, "breakable_surface_state",                        4),
    (0x006b8d7c, "k_water_density",                                4),
    (0x006b8d80, "k_air_density",                                  4),
    (0x006b8d84, "render_saved_projection_z",                     60),
    (0x006b8dc0, "rendered_object_count",                          2),
    (0x006b8dc4, "rendered_objects",                            1024),
    (0x006b91c8, "render_uncached_object_lighting",              116),
    (0x006b923c, "sky_animation_times",                           36),
    (0x006b9260, "frame_graphs",                               12976),
    (0x006bc510, "lens_flare_object_visibility_table",          8960),
    (0x006ce818, "lens_flare_instances",                       40960),
    (0x006d9838, "transparent_geometry_group_draw_cursor",         2),
    (0x006d983c, "transparent_geometry_group_drawn_bits",         48),
    (0x006d986c, "rasterizer_bound_bitmap_size_a",                 4),
    (0x006d9870, "rasterizer_bound_bitmap_size_b",                 4),
    (0x006d9874, "rasterizer_bound_bitmap_size_c",                 4),
    (0x006d9878, "rasterizer_screen_quad_vertices",               96),
    (0x006d98d8, "rasterizer_decal_blend_mode",                    2),
    (0x006d98dc, "rasterizer_decal_layer",                         2),
    (0x006d98e0, "rasterizer_decal_bitmap_tag",                    4),
    (0x006d98e4, "rasterizer_decal_bitmap_frame",                  2),
    (0x006d99d8, "rasterizer_dynamic_vertex_slots",            16384),
    (0x006dd9d8, "rasterizer_dynamic_vertex_slot_count",           4),
    (0x006dd9e0, "rasterizer_dynamic_index_slots",             12288),
    (0x006e09e0, "rasterizer_dynamic_index_slot_count",            4),
    (0x006e09e4, "rasterizer_dynamic_index_count",                 4),
    (0x006e09e8, "rasterizer_dynamic_index_buffer",                4),
    (0x006e09ec, "water_fade_plane_distance",                      4),
    (0x006e09f0, "water_fade_factor_a",                            4),
    (0x006e09f4, "water_fade_factor_b",                            4),
    (0x006e09f8, "rasterizer_underwater_tint_jitter_r",            4),
    (0x006e09fc, "rasterizer_underwater_tint_jitter_g",            4),
    (0x006e0a00, "rasterizer_underwater_tint_jitter_b",            4),
    (0x006e0a04, "unknown_006e0a04",                               1),
    (0x006e0a08, "rasterizer_environment_lightmap",                4),
    (0x006e0a0c, "rasterizer_environment_lightmap_missing",        1),
    (0x006e0a10, "rasterizer_projected_light",                    80),
    (0x006e0a60, "rasterizer_projected_light_has_cube_map",        1),
    (0x006e0a64, "rasterizer_projected_light_cube_map",            4),
    (0x006e0a68, "rasterizer_lightmap_bitmap_missing",             1),
    (0x006e0a6c, "rasterizer_lightmap_bitmap",                     4),
    (0x006e0a70, "transparent_geometry_group_immediate",         168),
    (0x006e0b18, "rasterizer_desktop_gamma_ramp",               1536),
    (0x006e1118, "rasterizer_game_gamma_ramp",                  1536),
    (0x006e1718, "rasterizer_gamma_high_bit_17",                   1),
    (0x006e1720, "rasterizer_shadow_screen_quad",                 96),
    (0x006e1780, "environment_techniques_multipurpose",           96),
    (0x006e17e0, "rasterizer_model_scratch_lighting",              4),
    (0x006e17e4, "rasterizer_model_effect_vector",                16),
    (0x006e1828, "transparent_geometry_group_environment_immediate",   168),
    (0x006e18d0, "rasterizer_model_scratch_function_source",       4),
    (0x006e18d4, "rasterizer_model_scratch_node_matrices",         4),
    (0x006e18d8, "environment_techniques_self_illumination",      96),
    (0x006e1938, "environment_techniques_plain",                  48),
    (0x006e1968, "environment_techniques_reflection",             96),
    (0x006e19c8, "rasterizer_model_scratch_node_count",            2),
    (0x006e19d0, "environment_techniques_change_color",           96),
    (0x006e1a30, "rasterizer_screen_effect_quad",                 96),
]


def main():
    errors = []
    asm = open(os.path.join(SA, "globals.asm")).read()
    equ = {m.group(1): int(m.group(2), 16) for m in re.finditer(r"^_(\w+) EQU ([0-9A-Fa-f]+)h", asm, re.M)}
    csrc = open(os.path.join(SA, "data", "slice05.c")).read()
    pieces = json.load(open(os.path.join(SA, "image", "pieces.json")))
    data = [p for p in pieces if p["label"] == "data"][0]
    bss_start = data["va"] + data["size"]
    names = set()
    for addr, name, size in GLOBALS:
        names.add(name)
        if name in equ:
            errors.append("%s: still an EQU in globals.asm" % name)
        if re.search(r"^PUBLIC _%s$" % name, asm, re.M):
            errors.append("%s: PUBLIC line left in globals.asm" % name)
        defs = re.findall(r"^[^/\n]*?\b%s\b\s*(?:\[[^\n]*?\])*;\s*// 0x([0-9a-f]+)" % re.escape(name), csrc, re.M)
        if len(defs) != 1:
            errors.append("%s: %d definitions in slice05.c" % (name, len(defs)))
        elif int(defs[0], 16) != addr:
            errors.append("%s: address comment 0x%s != 0x%08x" % (name, defs[0], addr))
        if addr < bss_start:
            errors.append("%s: 0x%08x is inside the initialised .data piece; initial bytes must be decoded" % (name, addr))
    ordered = sorted(GLOBALS)
    others = sorted(equ.items(), key=lambda kv: kv[1])
    for i, (addr, name, size) in enumerate(ordered):
        if i + 1 < len(ordered) and addr + size > ordered[i + 1][0]:
            errors.append("%s overlaps %s" % (name, ordered[i + 1][1]))
        for n, a in others:
            if addr < a < addr + size:
                errors.append("%s (0x%08x+%d) contains the still-EQU symbol %s at 0x%08x" % (name, addr, size, n, a))
    maps = [sys.argv[1]] if len(sys.argv) > 1 else glob.glob(os.path.join(ROOT, "build", "s05", "**", "halo_rebuilt.map"), recursive=True) \
        + glob.glob(os.path.join(ROOT, "build", "standalone", "halo_rebuilt.map"))
    placed = None
    if maps:
        text = open(maps[0], errors="replace").read()
        found = {}
        for m in re.finditer(r"^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+_(\w+)\s+([0-9a-f]{8})\s", text, re.M):
            if m.group(1) in names:
                found.setdefault(m.group(1), []).append(int(m.group(2), 16))
        for addr, name, size in GLOBALS:
            if name not in found:
                errors.append("%s: not in map %s" % (name, maps[0]))
            elif len(found[name]) != 1:
                errors.append("%s: defined %d times in the map" % (name, len(found[name])))
        placed = sorted((found[n][0], n, s) for _, n, s in GLOBALS if n in found and len(found[n]) == 1)
        for (a, n, s), (a2, n2, _) in zip(placed, placed[1:]):
            if a + s > a2:
                errors.append("linked: %s (0x%08x+%d) overlaps %s (0x%08x)" % (n, a, s, n2, a2))
    for e in errors:
        print("FAIL", e)
    print("%d converted globals checked, %d problems%s" % (len(GLOBALS), len(errors),
          "" if maps else " (no map found, link-time part skipped)"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
