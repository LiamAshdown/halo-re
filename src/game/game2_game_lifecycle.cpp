#include "halo/game/game2_game_lifecycle.hpp"
#include "halo/core/cstring.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/main/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/saved_games/vars.hpp"

static auto &game_state_cursor = halo::link::ref<int32_t>(halo::saved_games::vars().game_state_cursor);
static auto &game_state_base = halo::link::ref<uint8_t *>(halo::saved_games::vars().game_state_base);
static auto &game_state_crc = halo::link::ref<uint32_t>(halo::saved_games::vars().game_state_crc);
static auto &main_game_globals = halo::link::ref<void *>(halo::game::vars().main_game_globals);
static auto &game_engine_active_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_active_variant);
static auto &object_render_state_cache = halo::link::ref<data_array *>(halo::game::vars().object_render_state_cache);
static auto &breakable_surface_state = halo::link::ref<breakable_surface_globals *>(halo::physics::vars().breakable_surface_state);
static auto &particle_data = halo::link::ref<data_array *>(halo::effects::vars().particle_data);
static auto &effect_data = halo::link::ref<data_array *>(halo::effects::vars().effect_data);
static auto &effect_location_data = halo::link::ref<data_array *>(halo::effects::vars().effect_location_data);
static auto &weather_particle_data = halo::link::ref<data_array *>(halo::game::vars().weather_particle_data);
static auto &particle_system_data = halo::link::ref<void *>(halo::effects::vars().particle_system_data);
static auto &sound_class_gains = halo::link::ref<void *>(halo::game::vars().sound_class_gains);
static auto &recorded_animations = halo::link::ref<void *>(halo::game::vars().recorded_animations);
static auto &cinematic_globals_ptr = halo::link::ref<uint32_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &debug_print_safety_checks = halo::link::ref<uint8_t>(halo::game::vars().debug_print_safety_checks);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);

namespace halo::game {

/**
 * Returns a pointer to player_starting_locations[index], or NULL if index is negative or past the reflexive's
 * count.
 *
 * @address 0x477640
 */
ScenarioPlayerStartingLocation * GameLifecycle::get_player_starting_location(int16_t index)
{
    if (index >= 0 && index < halo::scenario::globals().scenario->player_starting_locations.count) {
        return &((ScenarioPlayerStartingLocation *)halo::scenario::globals().scenario->player_starting_locations.pointer)[index];
    }
    return (ScenarioPlayerStartingLocation *)0;
}

/**
 * One-time post-map-load initialization that bump-allocates every particle/effect/render-state pool, sets the
 * FPU control word, allocates the simulation tick record, loads the active game engine, allocates the team-
 * pair table, and starts the object, player, sound, AI, script, input and save-file subsystems.
 *
 * @address 0x45a9c0
 */
void GameLifecycle::initialize(void)
{
    uint32_t *cursor;
    int32_t i;
    uint32_t size;

    cursor = (uint32_t *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x114;
    size = 0x114;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    main_game_globals = cursor;
    for (i = 0x45; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }

    cursor = (uint32_t *)&game_engine_active_variant;
    for (i = 0x26; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }

    _control87(0x9001f, 0xfffff);
    halo::game::game_engine_allocate_tick_record();
    halo::game::game_engine_load_from_variant(&game_engine_active_variant);
    halo::game::team_pair_table_allocate();
    halo::interface::interface_globals_allocate();

    size = 0x7c;
    halo::scenario::globals().game_globals = (scenario_game_globals *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x7c;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);

    halo::camera::globals().hs_camera_control_pointer = (uint8_t *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    size = 4;
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 4;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    *halo::camera::globals().hs_camera_control_pointer = 0;

    object_render_state_cache = (data_array *)halo::saved_games::game_state_new(halo::mutable_literal("cached object render states"), 0x100, 0x100);
    halo::objects::objects_initialize();
    halo::structures::detail_objects_globals_allocate();

    size = 4;
    halo::structures::globals().runtime_decals_suppressed = (uint8_t *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 4;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);

    size = 0x4204;
    halo::physics::globals().breakable_surface_state = (breakable_surface_globals *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x4204;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);

    halo::effects::decals_initialize();
    halo::game::players_initialize();
    halo::effects::contrails_initialize();

    halo::effects::globals().particle_data = (data_array *)halo::saved_games::game_state_new(halo::mutable_literal("particle"), 0x400, 0x70);
    halo::effects::globals().effect_data = (data_array *)halo::saved_games::game_state_new(halo::mutable_literal("effect"), 0x100, 0xfc);
    halo::effects::globals().effect_location_data = (data_array *)halo::saved_games::game_state_new(halo::mutable_literal("effect location"), 0x200, 0x3c);
    halo::effects::globals().weather_particle_data = halo::memory::data_new(0x54, halo::mutable_literal("weather particles"), 0x200);
    particle_system_data = halo::saved_games::game_state_new(halo::mutable_literal("particle systems"), 0x40, 0x158);
    halo::effects::globals().particle_system_particle_data = (data_array *)halo::saved_games::game_state_new(halo::mutable_literal("particle system particles"), 0x200, 0x80);

    size = 0x264;
    sound_class_gains = (void *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x264;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    halo::sound::game_sound_initialize();

    size = 0x128;
    halo::effects::globals().player_effect_state = (player_effect_globals *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x128;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    halo::ai::ai_initialize_for_new_map();

    halo::interface::widget_memory_pool_initialize();
    halo::hs::object_lists_initialize();
    halo::hs::hs_runtime_initialize();
    halo::hs::hs_scripts_reload();

    recorded_animations = halo::saved_games::game_state_new(halo::mutable_literal("recorded animations"), 0x40, 0x64);

    size = 0x1c;
    cinematic_globals_ptr = (uint32_t *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x1c;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    halo::saved_games::saved_game_files_initialize();

    halo::input::UiEvents::queue_initialize();
    halo::input::InputSystem::state_initialize();
    halo::interface::player_profile_subsystem_initialize();
}

/**
 * Returns true only when there are no nearby dangerous projectiles and no player is currently without a
 * controlled unit (dead/respawning).
 *
 * @address 0x45bbe0
 */
uint32_t GameLifecycle::no_player_is_dead(void)
{
    object_iterator iterator;

    iterator.type_mask = _object_mask_projectile;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    if (halo::objects::object_iterator_next(&iterator) == (object *)0) {
        if (halo::game::players_any_without_unit() == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Returns whether it is currently safe to pause: no nearby dangerous projectiles, items, effects or units. A
 * quieter subset of game_safe_to_save's checks (no AI-visibility check, no console logging).
 *
 * @address 0x45b9e0
 */
uint32_t GameLifecycle::safe_to_pause(void)
{
    object_iterator iterator;

    iterator.type_mask = _object_mask_projectile;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    if (halo::objects::object_iterator_next(&iterator) == (object *)0) {
        if (halo::items::item_any_detonating() == 0 && halo::effects::effect_check_object_collisions() == 0 && halo::units::unit_any_dying_or_seat_transition() == 0 && halo::ai::ai_scan_for_recent_combat_activity(0) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Returns whether it is currently safe to auto/quick-save, checking for nearby AI threats, dangerous
 * projectiles/items/effects, dangerous units, airborne/dead players, and moving vehicles, logging the specific
 * reason to the console when debug_print_safety_checks is set.
 *
 * @address 0x45ba50
 */
uint8_t GameLifecycle::safe_to_save(void)
{
    object_iterator iterator;

    if (halo::ai::ai_scan_for_recent_combat_activity(0) != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: ai_enemies_can_see_player");
        }
        return 0;
    }

    iterator.type_mask = _object_mask_projectile;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    if (halo::objects::object_iterator_next(&iterator) != (object *)0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: dangerous_projectiles_near_player");
        }
        return 0;
    }
    if (halo::items::item_any_detonating() != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: dangerous_items_near_player");
        }
        return 0;
    }
    if (halo::effects::effect_check_object_collisions() != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: dangerous_effects_near_player");
        }
        return 0;
    }
    if (halo::units::unit_any_dying_or_seat_transition() != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: any_unit_is_dangerous");
        }
        return 0;
    }
    if (halo::game::players_any_pending_seat_or_respawn() != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: any_player_is_in_the_air");
        }
        return 0;
    }
    if (halo::game::players_any_without_unit() != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: any_player_is_dead");
        }
        return 0;
    }
    if (halo::units::unit_is_area_clear_of_fast_objects() != 0) {
        if (debug_print_safety_checks != 0) {
            halo::main::console_print_va("not safe to save: vehicle_moving_near_any_player");
        }
        return 0;
    }
    return 1;
}

/**
 * Binds `player_handle` to local-player slot `local_player_index` (only slot 0 is ever valid in this build).
 * Clears the previous occupant's local_player_index back-reference first, and sets the new occupant's
 * local_player_index to this slot unless player_handle is the wildcard.
 *
 * @address 0x474d50
 */
void GameLifecycle::set_local_player(datum_index player_handle, int16_t local_player_index)
{
    datum_index previous;
    player *p;

    if (local_player_index >= 0 && local_player_index < 1) {
        previous = local_player_globals->local_players[local_player_index];
        if (previous != k_datum_index_none) {
            p = halo::game::player_at(previous);
            p->local_player_index = -1;
        }
        local_player_globals->local_players[local_player_index] = player_handle;
        if (player_handle != k_datum_index_none) {
            p = halo::game::player_at(player_handle);
            p->local_player_index = local_player_index;
        }
    }
}

/**
 * Formats a tick count as minutes:seconds wide text. The minutes part is a single space when zero and the
 * seconds part is zero-padded below ten; the format templates are ordinary wide string literals.
 *
 * @address 0x466530
 */
void GameLifecycle::time_format_minutes_seconds(uint32_t ticks, uint32_t count, wchar_t *dest)
{
    int32_t total_seconds = (int32_t)ticks / 30;
    int32_t minutes = total_seconds / 60;
    int32_t seconds = total_seconds - minutes * 60;
    uint16_t minutes_text[0x40];
    uint16_t seconds_text[0x40];

    if (minutes == 0) {
        halo::text::string_format_wide_va_bounded(0x40, minutes_text, (const uint16_t *)L" ");
    } else {
        halo::text::string_format_wide_va_bounded(0x40, minutes_text, (const uint16_t *)L"%d", minutes);
    }
    halo::text::string_format_wide_va_bounded(0x40, seconds_text, (const uint16_t *)(seconds <= 9 ? L"0%d" : L"%d"), seconds);
    halo::text::string_format_wide_va_bounded(count, (uint16_t *)dest, (const uint16_t *)L"%s:%s", minutes_text, seconds_text);
}

/**
 * ASCII counterpart of game_time_format_minutes_seconds: ticks / 30 as seconds, minutes and zero-padded
 * seconds formatted with _snprintf and combined as "%s:%s".
 *
 * @address 0x466600
 */
void GameLifecycle::time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest)
{
    int32_t total_seconds = (int32_t)ticks / 30;
    int32_t minutes = total_seconds / 60;
    int32_t seconds = total_seconds - minutes * 60;
    char minutes_text[0x40];
    char seconds_text[0x40];

    if (minutes == 0) {
        _snprintf(minutes_text, 0x40, " ");
    } else {
        _snprintf(minutes_text, 0x40, "%d", minutes);
    }
    _snprintf(seconds_text, 0x40, seconds <= 9 ? "0%d" : "%d", seconds);
    _snprintf(dest, count, "%s:%s", minutes_text, seconds_text);
}

}
