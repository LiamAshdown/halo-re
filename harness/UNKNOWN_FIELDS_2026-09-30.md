# unknown_* field naming log (2026-09-30)
Method: read-only agents propose maps from in-repo evidence; maps are dry-run, filtered, applied with tools/rename_field.py,
then tools/check32.sh must pass. Full skip/conflict reasons from the agents are summarised here.

- objects.map: 11 applied (light, light_transient, antenna, antenna_vertex, damage_data, vehicle_data). 98 fields have no uses in src; 21 more skipped for weak evidence.
- ai2.map: 23 of 26 applied. Dropped: ai_search_context.unknown_04 (caller says ignores_glass, callee header says ignore_permission), actor_firing_position_query.unknown_42 (goal_kind_is_5 too weak), ScenarioNetgameEquipment.unknown_ffffffff (types/tags.h is generated from invader).
  Conflicts kept unknown: path_find_node.unknown_00 (scratch, not stable). Stale header note: actor_firing_position_query 0x38/0x3c follow the code (avoid_weight / avoid_radius).
- Open conflicts elsewhere: actor 0x350 block (actor_update_crouch_state vs actor_update_facing_change_timer disagree on 0x354/0x358/0x35a).
- rest.map: 28 of 30 applied (effects, render, rasterizer, camera, interface, cache, hs, structures). Dropped: decal_type_parameters.unknown_04/0c (1 use each, weak).
  Conflicts left unknown: bit_stream.unknown_00 (0/1/-1 written by network code), particle_system.unknown_54 (byte flag vs "no reader"), player_camera_shake +0x24/+0x28 vs DamageEffect mirror, sound_location.unknown_36 / sound_placement.unknown_2a, hud_text_message.unknown_04.
- game.map: applied all but team_pair_override.unknown_08/09 (direction of a_to_b/b_to_a is a guess). Conflicts left unknown: player.unknown_f0/f4/f8 (control record in remote-update code, x/y/z position in history-play code); scoreboard_entry.unknown_04 was renamed (single_sort_key) because the "never compared" header was stale. Needs a real split, not a rename: network_server_globals.unknown_9bc[0x3c], message_delta_field_type_vtable.unknown_00[8].
