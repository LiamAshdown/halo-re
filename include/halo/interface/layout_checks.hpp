/**
 * @file include/halo/interface/layout_checks.hpp
 * Compile-time checks that the engine record and tag structs the interface code reads through named members keep the byte offsets
 * the original code used. Include after the engine type headers (tags.h, objects.h, units.h, items.h, game.h, networking.h,
 * saved_games.h, interface.h).
 */
#pragma once

#include <stddef.h>

#include "items.h"
#include "saved_games.h"

static_assert(offsetof(HUDGlobals, hud_damage_top_offset) == 0x310 && offsetof(HUDGlobals, hud_damage_right_offset) == 0x316 &&
              offsetof(HUDGlobals, hud_damage_indicator_bitmap.tag_id) == 0x344 && offsetof(HUDGlobals, hud_damage_sequence_index) == 0x348 &&
              offsetof(HUDGlobals, hud_damage_multiplayer_sequence_index) == 0x34a && offsetof(HUDGlobals, hud_damage_color) == 0x34c,
              "HUDGlobals damage indicator block");
static_assert(sizeof(GlobalsGrenade) == 0x44 && offsetof(GlobalsGrenade, hud_interface.tag_id) == 0x20 &&
              offsetof(Globals, grenades.pointer) == 0x12c && offsetof(Globals, first_person_interface.pointer) == 0x180,
              "Globals grenade and first person blocks");
static_assert(offsetof(Unit, seats.count) == 0x2e4 && offsetof(UnitSeat, label) == 4, "Unit seat block");
static_assert(offsetof(Equipment, powerup_type) == 0x308 && offsetof(Equipment, pickup_sound.tag_id) == 0x31c, "Equipment fields");
static_assert(offsetof(Font, character_tables.pointer) == 0x34 && offsetof(Font, characters.pointer) == 0x80 &&
              sizeof(FontCharacter) == 0x14 && sizeof(FontCharacterTables) == 0xc, "Font tables");
static_assert(offsetof(Scenario, scripts.pointer) == 0x4a0 && offsetof(Scenario, hud_messages.tag_id) == 0x5a0 &&
              sizeof(ScenarioScript) == 0x5c && offsetof(ScenarioScript, root_expression_index) == 0x24, "Scenario script and hud message fields");
static_assert(offsetof(HUDMessageText, text_data.pointer) == 0xc && offsetof(HUDMessageTextMessage, start_index_into_text_blob) == 0x20,
              "HUD message text");
static_assert(offsetof(Weapon, magazines.pointer) == 0x4f4 && offsetof(WeaponMagazine, rounds_loaded_maximum) == 0xa, "Weapon magazines");
static_assert(offsetof(WeaponHUDInterface, child_hud.tag_id) == 0xc && offsetof(WeaponHUDInterface, total_ammo_cutoff) == 0x14 &&
              offsetof(WeaponHUDInterface, loaded_ammo_cutoff) == 0x16 && offsetof(WeaponHUDInterface, heat_cutoff) == 0x18 &&
              offsetof(WeaponHUDInterface, age_cutoff) == 0x1a && offsetof(WeaponHUDInterface, crosshair_types) == 0x9c,
              "WeaponHUDInterface meter evaluation fields");
static_assert(offsetof(UIWidgetDefinition, flags_2) == 0x150, "UIWidgetDefinition flags_2");

static_assert(offsetof(object, vitality_flags) == 0x106 && offsetof(object, owner_team) == 0xb8 && offsetof(object, bounding_center) == 0xa0 &&
              offsetof(object, function_out_values) == 0x134 && offsetof(object, change_colors) == 0x1b8, "object fields");
static_assert(offsetof(unit_object, unit.flags) == 0x204 && offsetof(unit_object, unit.control_flags) == 0x208 &&
              offsetof(unit_object, unit.aiming_vector) == 0x23c && offsetof(unit_object, unit.throttle) == 0x278 &&
              offsetof(unit_object, unit.throwing_grenade_state) == 0x28d && offsetof(unit_object, unit.weapons) == 0x2f8 &&
              offsetof(unit_object, unit.current_grenade_index) == 0x31c && offsetof(unit_object, unit.grenade_counts) == 0x31e &&
              offsetof(unit_object, unit.driver_unit_index) == 0x324 && offsetof(unit_object, unit.gunner_unit_index) == 0x328,
              "unit_object fields");
static_assert(offsetof(weapon_object, weapon.flags) == 0x22c && offsetof(weapon_object, weapon.magazines) == 0x2b0, "weapon_object fields");
static_assert(offsetof(weapon_hud_ammo_state, magazines[0].rounds_loaded) == 0xe && offsetof(weapon_hud_ammo_state, magazines[0].rounds_unloaded) == 0x12 &&
              offsetof(weapon_hud_ammo_state, magazines[1].reloading) == 0x16 && offsetof(weapon_hud_ammo_state, magazines[1].rounds_loaded) == 0x18 &&
              offsetof(weapon_hud_ammo_state, magazines[1].rounds_unloaded) == 0x1c, "weapon_hud_ammo_state fields");
static_assert(offsetof(player, team) == 0x20 && offsetof(player, unit) == 0x34 && offsetof(player, machine_index) == 0x64, "player fields");
static_assert(sizeof(hud_player_messaging_state) == 0x460, "hud_player_messaging_state stride");

static_assert(offsetof(network_player_entry, machine_index) == 0x1c && offsetof(network_game_session, players) == 0x1a2 &&
              offsetof(network_game_session, variant.teams) == 0x138 - 0x0 && offsetof(network_game_session, variant) == 0x104,
              "network_game_session fields");
static_assert(offsetof(network_server_globals, session) == 0x8 && offsetof(network_server_globals, flags) == 0x6 &&
              offsetof(network_server_globals, machines) == 0x3b8 && offsetof(network_server_globals, handshake_blocked) == 0x9d5 &&
              offsetof(network_server_globals, handshake_timer) == 0x9c8, "network_server_globals fields");
static_assert(offsetof(network_client_globals, state) == 0xeda && offsetof(network_client_globals, session) == 0xb14 &&
              offsetof(network_client_globals, search_entries) == 0x4 && offsetof(network_client_globals, channel) == 0xadc &&
              offsetof(network_client_globals, game_start_countdown_seconds) == 0xed8, "network_client_globals fields");
static_assert(offsetof(network_channel, flags) == 0xa8c && offsetof(network_channel, send_budget) == 0xa80 &&
              offsetof(network_channel, outgoing) == 0x10 && offsetof(network_channel, listening) == 0xae0 &&
              offsetof(network_machine, channel) == 0 && offsetof(network_machine, machine_id) == 0xc, "network channel fields");
static_assert(offsetof(network_game_search_entry, joinable) == 0x12c && offsetof(network_game_search_entry, in_use) == 0x12d &&
              offsetof(network_game_search_entry, game_engine_index) == 0x120, "network_game_search_entry fields");

static_assert(offsetof(saved_player_profile, flags) == 0x11c && offsetof(saved_player_profile, campaign_progress) == 0x11e &&
              offsetof(saved_player_profile, gamepad_action_buttons) == 0x32a && offsetof(saved_player_profile, gamma) == 0xa76 &&
              offsetof(saved_player_profile, master_volume) == 0xb78 && offsetof(saved_player_profile, server_name) == 0xd8c &&
              offsetof(saved_player_profile, connection_type) == 0xfc0 && offsetof(saved_player_profile, join_server_address) == 0xfc2 &&
              offsetof(saved_player_profile, server_port) == 0x1002 && offsetof(saved_player_profile, gamepads) == 0x1108 &&
              offsetof(saved_player_profile, server_browser_ping_limit) == 0xc8a, "saved_player_profile fields");
