#include "halo/hs/hs1_world_commands.hpp"
#include "halo/devices/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern datum_index player_index_from_unit_index(datum_index unit_index);
extern void hud_waypoint_activate_for_player(datum_index player_index, datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset);
extern void hud_waypoint_activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind, float vertical_offset);
extern uint8_t *breakable_surface_state;
extern Globals *global_globals;
extern void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count);
extern void cheat_all_weapons(void);
extern void cheat_spawn_warthog(void);
extern void cheat_teleport_to_camera(void);
extern void hud_waypoint_deactivate_for_player(datum_index player_index, datum_index target, int16_t kind);
extern void hud_waypoint_deactivate_for_team(int16_t kind, int16_t team, datum_index target);
}

namespace halo::hs {

/**
 * Evaluate handler for hs function "activate_nav_point_flag" (navpoint, unit, cutscene_flag, real -> void).
 *
 * @address 0x4803b0
 */
void NavPointCommands::activate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        datum_index player = halo::game::player_index_from_unit_index((datum_index)arguments[1]);

        if (player != k_datum_index_none) {
            halo::interface::hud_waypoint_activate_for_player(player, (datum_index)(int32_t)*(int16_t *)&arguments[2], 0,
                *(int16_t *)&arguments[0], *(float *)&arguments[3]);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "activate_nav_point_object" (navpoint, unit, object, real -> void).
 *
 * @address 0x480420
 */
void NavPointCommands::activate_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index player = halo::game::player_index_from_unit_index((datum_index)arguments[1]);

    if (player != k_datum_index_none) {
        halo::interface::hud_waypoint_activate_for_player(player, (datum_index)arguments[2], 1, *(int16_t *)&arguments[0],
            *(float *)&arguments[3]);
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "activate_team_nav_point_flag" (navpoint, team, cutscene_flag, real ->
 * void).
 *
 * @address 0x480490
 */
void NavPointCommands::activate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_waypoint_activate_for_team((datum_index)(int32_t)*(int16_t *)&arguments[2], *(int16_t *)&arguments[0],
        *(int16_t *)&arguments[1], 0, *(float *)&arguments[3]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "activate_team_nav_point_object" (navpoint, team, object, real -> void).
 *
 * @address 0x4804f0
 */
void NavPointCommands::activate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_waypoint_activate_for_team((datum_index)arguments[2], *(int16_t *)&arguments[0], *(int16_t *)&arguments[1], 1,
        *(float *)&arguments[3]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "deactivate_nav_point_flag" (unit, cutscene_flag -> void).
 *
 * @address 0x480550
 */
void NavPointCommands::deactivate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        datum_index player = halo::game::player_index_from_unit_index((datum_index)arguments[0]);

        if (player != k_datum_index_none) {
            halo::interface::hud_waypoint_deactivate_for_player(player, (datum_index)(int32_t)*(int16_t *)&arguments[1], 0);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "deactivate_nav_point_object" (unit, object -> void).
 *
 * @address 0x4805b0
 */
void NavPointCommands::deactivate_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        datum_index player = halo::game::player_index_from_unit_index((datum_index)arguments[0]);

        if (player != k_datum_index_none) {
            halo::interface::hud_waypoint_deactivate_for_player(player, (datum_index)arguments[1], 1);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "deactivate_team_nav_point_flag" (team, cutscene_flag -> void).
 *
 * @address 0x480610
 */
void NavPointCommands::deactivate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_waypoint_deactivate_for_team(0, *(int16_t *)&arguments[0], (datum_index)(int32_t)*(int16_t *)&arguments[1]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "deactivate_team_nav_point_object" (team, object -> void).
 *
 * @address 0x480660
 */
void NavPointCommands::deactivate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_waypoint_deactivate_for_team(1, *(int16_t *)&arguments[0], (datum_index)arguments[1]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_nav_point_commands_entries[] = {
    {"activate_nav_point_flag", &NavPointCommands::activate_nav_point_flag},
    {"activate_nav_point_object", &NavPointCommands::activate_nav_point_object},
    {"activate_team_nav_point_flag", &NavPointCommands::activate_team_nav_point_flag},
    {"activate_team_nav_point_object", &NavPointCommands::activate_team_nav_point_object},
    {"deactivate_nav_point_flag", &NavPointCommands::deactivate_nav_point_flag},
    {"deactivate_nav_point_object", &NavPointCommands::deactivate_nav_point_object},
    {"deactivate_team_nav_point_flag", &NavPointCommands::deactivate_team_nav_point_flag},
    {"deactivate_team_nav_point_object", &NavPointCommands::deactivate_team_nav_point_object},
};
constexpr ScriptCommandGroup k_nav_point_commands_group(k_nav_point_commands_entries, sizeof(k_nav_point_commands_entries) / sizeof(k_nav_point_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by NavPointCommands, keyed by script function name.
 */
const ScriptCommandGroup &NavPointCommands::commands()
{
    return k_nav_point_commands_group;
}

/**
 * Evaluate handler for hs function "breakable_surfaces_enable" (boolean -> void).
 *
 * @address 0x47af00
 */
void WorldStateCommands::breakable_surfaces_enable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        breakable_surface_state[0] = *(uint8_t *)&arguments[0];
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "breakable_surfaces_reset" (-> void).
 *
 * @address 0x47ce60
 */
void WorldStateCommands::run_breakable_surfaces_reset(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    halo::objects::breakable_surfaces_reset();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "custom_animation" (unit, animation_graph, string, boolean -> boolean).
 *
 * @address 0x47bad0
 */
void WorldStateCommands::custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)halo::units::unit_start_user_animation((uint32_t)arguments[0], (datum_index)arguments[1],
        (const char *)arguments[2], *(uint8_t *)&arguments[3]), thread_index);
    }
}

/**
 * Evaluate handler for hs function "custom_animation_list" (object_list, animation_graph, string, boolean ->
 * boolean).
 *
 * @address 0x47bb40
 */
void WorldStateCommands::custom_animation_list(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)halo::ai::ai_object_list_start_user_animation_until_failure((datum_index)arguments[0],
        (datum_index)arguments[1], (const char *)arguments[2], *(uint8_t *)&arguments[3]), thread_index);
    }
}

namespace {
const ScriptCommandEntry k_world_state_commands_entries[] = {
    {"breakable_surfaces_enable", &WorldStateCommands::breakable_surfaces_enable},
    {"breakable_surfaces_reset", &WorldStateCommands::run_breakable_surfaces_reset},
    {"custom_animation", &WorldStateCommands::custom_animation},
    {"custom_animation_list", &WorldStateCommands::custom_animation_list},
};
constexpr ScriptCommandGroup k_world_state_commands_group(k_world_state_commands_entries, sizeof(k_world_state_commands_entries) / sizeof(k_world_state_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by WorldStateCommands, keyed by script function name.
 */
const ScriptCommandGroup &WorldStateCommands::commands()
{
    return k_world_state_commands_group;
}

/**
 * Hs routine hs_evaluate_cheat_active_camouflage.
 *
 * @address 0x47ced0
 */
void CheatCommands::cheat_active_camouflage(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::cheat_make_selected_object_invincible();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Hs routine hs_evaluate_cheat_active_camouflage_local_player.
 *
 * @address 0x47cee0
 */
void CheatCommands::cheat_active_camouflage_local_player(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::game::cheat_make_player_invincible(*(int16_t *)arguments);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Hs routine hs_evaluate_cheat_all_powerups.
 *
 * @address 0x47ce70
 */
void CheatCommands::cheat_all_powerups(int16_t function_index, uint32_t thread_index, char first)
{
    TagDependency *list = *(int32_t *)&global_globals->cheat_powerups.count != 0 ? (TagDependency *)global_globals->cheat_powerups.pointer : 0;

    halo::game::cheat_spawn_objects_near_camera(list, *(int16_t *)&global_globals->cheat_powerups.count);
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Hs routine hs_evaluate_cheat_all_vehicles.
 *
 * @address 0x4828b0
 */
void CheatCommands::cheat_all_vehicles(int16_t function_index, uint32_t thread_index, char first)
{
    if (*(int32_t *)&global_globals->multiplayer_information.count != 0) {
        uint8_t *element = (uint8_t *)global_globals->multiplayer_information.pointer;

        halo::game::cheat_spawn_objects_near_camera(*(TagDependency **)(element + 0x24), (int16_t)*(uint16_t *)(element + 0x20));
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Hs routine hs_evaluate_cheat_all_weapons.
 *
 * @address 0x4828a0
 */
void CheatCommands::run_cheat_all_weapons(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::cheat_all_weapons();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Hs routine hs_evaluate_cheat_spawn_warthog.
 *
 * @address 0x47ceb0
 */
void CheatCommands::run_cheat_spawn_warthog(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::cheat_spawn_warthog();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Hs routine hs_evaluate_cheat_teleport_to_camera.
 *
 * @address 0x47cec0
 */
void CheatCommands::run_cheat_teleport_to_camera(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::cheat_teleport_to_camera();
    halo::hs::hs_thread_return(0, thread_index);
}

namespace {
const ScriptCommandEntry k_cheat_commands_entries[] = {
    {"cheat_active_camouflage", &CheatCommands::cheat_active_camouflage},
    {"cheat_active_camouflage_local_player", &CheatCommands::cheat_active_camouflage_local_player},
    {"cheat_all_powerups", &CheatCommands::cheat_all_powerups},
    {"cheat_all_vehicles", &CheatCommands::cheat_all_vehicles},
    {"cheat_all_weapons", &CheatCommands::run_cheat_all_weapons},
    {"cheat_spawn_warthog", &CheatCommands::run_cheat_spawn_warthog},
    {"cheat_teleport_to_camera", &CheatCommands::run_cheat_teleport_to_camera},
};
constexpr ScriptCommandGroup k_cheat_commands_group(k_cheat_commands_entries, sizeof(k_cheat_commands_entries) / sizeof(k_cheat_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by CheatCommands, keyed by script function name.
 */
const ScriptCommandGroup &CheatCommands::commands()
{
    return k_cheat_commands_group;
}

/**
 * Evaluate handler for hs function "damage_new" (damage, cutscene_flag -> void).
 *
 * @address 0x47aa60
 */
void DamageCommands::damage_new(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_damage_apply_at_location(*(int16_t *)&arguments[1], (uint32_t)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "damage_object" (damage, object -> void).
 *
 * @address 0x47aab0
 */
void DamageCommands::damage_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_damage_apply_with_sound((datum_index)arguments[1], (uint32_t)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_damage_commands_entries[] = {
    {"damage_new", &DamageCommands::damage_new},
    {"damage_object", &DamageCommands::damage_object},
};
constexpr ScriptCommandGroup k_damage_commands_group(k_damage_commands_entries, sizeof(k_damage_commands_entries) / sizeof(k_damage_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by DamageCommands, keyed by script function name.
 */
const ScriptCommandGroup &DamageCommands::commands()
{
    return k_damage_commands_group;
}

/**
 * Evaluate handler for hs function "device_get_position" (device -> real).
 *
 * @address 0x47cae0
 */
void DeviceCommands::device_get_position(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];
    int32_t result = 0;

    if (device != k_datum_index_none) {
        result = *(int32_t *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(device)) + 0x208);
    }
    halo::hs::hs_thread_return(result, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_get_power" (device -> real).
 *
 * @address 0x47c9c0
 */
void DeviceCommands::device_get_power(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int32_t power = 0;

        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            power = *(int32_t *)((uint8_t *)halo::ai::object_at(arguments[0]) + 0x1fc);
        }
        halo::hs::hs_thread_return(power, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_group_change_only_once_more_set" (device_group, boolean -> void).
 *
 * @address 0x47cde0
 */
void DeviceCommands::device_group_change_only_once_more_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t group = *(int16_t *)&arguments[0];

        if (group != -1) {
            uint8_t *record = (uint8_t *)halo::devices::globals().device_groups->data + (uint16_t)group * 8;

            if (*(uint8_t *)&arguments[1] != 0) {
                record[2] |= 1;
            } else {
                record[2] &= 0xfe;
            }
            record[2] &= 0xfd;
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_group_get" (device_group -> real).
 *
 * @address 0x47cbe0
 */
void DeviceCommands::device_group_get(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = *(float *)((uint8_t *)halo::devices::globals().device_groups->data + (uint16_t)*(uint16_t *)&arguments[0] * 8 + 4);
        halo::hs::hs_thread_return(*(int32_t *)&value, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_group_set" (device_group, real -> boolean).
 *
 * @address 0x47cc30
 */
void DeviceCommands::device_group_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)halo::devices::device_group_set_value(*(uint16_t *)&arguments[0], *(float *)&arguments[1]), thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_group_set_immediate" (device_group, real -> void).
 *
 * @address 0x47cc90
 */
void DeviceCommands::device_group_set_immediate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::devices::device_group_set_value_immediate(*(uint16_t *)&arguments[0], *(float *)&arguments[1]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_one_sided_set" (device, boolean -> void).
 *
 * @address 0x47cce0
 */
void DeviceCommands::device_one_sided_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        uint8_t *device = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], 0x80);

        if (device != 0) {
            if (*(uint8_t *)&arguments[1] != 0) {
                *(uint32_t *)(device + 0x214) |= 2;
            } else {
                *(uint32_t *)(device + 0x214) &= 0xfffffffd;
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_operates_automatically_set" (device, boolean -> void).
 *
 * @address 0x47cd60
 */
void DeviceCommands::device_operates_automatically_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint8_t *device = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], 0x80);

    if (device != 0) {
        if (*(uint8_t *)&arguments[1]) {
            *(uint32_t *)(device + 0x214) &= 0xfffffffe;
        } else {
            *(uint32_t *)(device + 0x214) |= 1;
        }
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "device_set_never_appears_locked" (device, boolean -> void).
 *
 * @address 0x47c8b0
 */
void DeviceCommands::device_set_never_appears_locked(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *device = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], 0x80);

            if (device != 0) {
                if (*(uint8_t *)&arguments[1] != 0) {
                    *(uint32_t *)(device + 0x214) |= 4;
                } else {
                    *(uint32_t *)(device + 0x214) &= 0xfffffffb;
                }
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_device_commands_entries[] = {
    {"device_get_position", &DeviceCommands::device_get_position},
    {"device_get_power", &DeviceCommands::device_get_power},
    {"device_group_change_only_once_more_set", &DeviceCommands::device_group_change_only_once_more_set},
    {"device_group_get", &DeviceCommands::device_group_get},
    {"device_group_set", &DeviceCommands::device_group_set},
    {"device_group_set_immediate", &DeviceCommands::device_group_set_immediate},
    {"device_one_sided_set", &DeviceCommands::device_one_sided_set},
    {"device_operates_automatically_set", &DeviceCommands::device_operates_automatically_set},
    {"device_set_never_appears_locked", &DeviceCommands::device_set_never_appears_locked},
};
constexpr ScriptCommandGroup k_device_commands_group(k_device_commands_entries, sizeof(k_device_commands_entries) / sizeof(k_device_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by DeviceCommands, keyed by script function name.
 */
const ScriptCommandGroup &DeviceCommands::commands()
{
    return k_device_commands_group;
}

}

namespace halo::hs {

void hs_evaluate_activate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::activate_nav_point_flag(function_index, thread_index, first);
}

void hs_evaluate_activate_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::activate_nav_point_object(function_index, thread_index, first);
}

void hs_evaluate_activate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::activate_team_nav_point_flag(function_index, thread_index, first);
}

void hs_evaluate_activate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::activate_team_nav_point_object(function_index, thread_index, first);
}

void hs_evaluate_deactivate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::deactivate_nav_point_flag(function_index, thread_index, first);
}

void hs_evaluate_deactivate_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::deactivate_nav_point_object(function_index, thread_index, first);
}

void hs_evaluate_deactivate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::deactivate_team_nav_point_flag(function_index, thread_index, first);
}

void hs_evaluate_deactivate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::NavPointCommands::deactivate_team_nav_point_object(function_index, thread_index, first);
}

void hs_evaluate_breakable_surfaces_enable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::WorldStateCommands::breakable_surfaces_enable(function_index, thread_index, first);
}

void hs_evaluate_breakable_surfaces_reset(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::WorldStateCommands::run_breakable_surfaces_reset(function_index, thread_index, first);
}

void hs_evaluate_custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::WorldStateCommands::custom_animation(function_index, thread_index, first);
}

void hs_evaluate_custom_animation_list(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::WorldStateCommands::custom_animation_list(function_index, thread_index, first);
}

void hs_evaluate_cheat_active_camouflage(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::cheat_active_camouflage(function_index, thread_index, first);
}

void hs_evaluate_cheat_active_camouflage_local_player(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::cheat_active_camouflage_local_player(function_index, thread_index, first);
}

void hs_evaluate_cheat_all_powerups(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::cheat_all_powerups(function_index, thread_index, first);
}

void hs_evaluate_cheat_all_vehicles(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::cheat_all_vehicles(function_index, thread_index, first);
}

void hs_evaluate_cheat_all_weapons(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::run_cheat_all_weapons(function_index, thread_index, first);
}

void hs_evaluate_cheat_spawn_warthog(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::run_cheat_spawn_warthog(function_index, thread_index, first);
}

void hs_evaluate_cheat_teleport_to_camera(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CheatCommands::run_cheat_teleport_to_camera(function_index, thread_index, first);
}

void hs_evaluate_damage_new(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DamageCommands::damage_new(function_index, thread_index, first);
}

void hs_evaluate_damage_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DamageCommands::damage_object(function_index, thread_index, first);
}

void hs_evaluate_device_get_position(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_get_position(function_index, thread_index, first);
}

void hs_evaluate_device_get_power(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_get_power(function_index, thread_index, first);
}

void hs_evaluate_device_group_change_only_once_more_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_group_change_only_once_more_set(function_index, thread_index, first);
}

void hs_evaluate_device_group_get(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_group_get(function_index, thread_index, first);
}

void hs_evaluate_device_group_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_group_set(function_index, thread_index, first);
}

void hs_evaluate_device_group_set_immediate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_group_set_immediate(function_index, thread_index, first);
}

void hs_evaluate_device_one_sided_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_one_sided_set(function_index, thread_index, first);
}

void hs_evaluate_device_operates_automatically_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_operates_automatically_set(function_index, thread_index, first);
}

void hs_evaluate_device_set_never_appears_locked(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::device_set_never_appears_locked(function_index, thread_index, first);
}

}
