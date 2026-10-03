#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "hs.h"

#include "halo/hs/hs1_command.hpp"

namespace halo::hs {

/**
 * Evaluate handlers of the hs (team) nav point activation script commands.
 */
class NavPointCommands {
public:
    static void activate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first);
    static void activate_nav_point_object(int16_t function_index, uint32_t thread_index, char first);
    static void activate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first);
    static void activate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first);
    static void deactivate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first);
    static void deactivate_nav_point_object(int16_t function_index, uint32_t thread_index, char first);
    static void deactivate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first);
    static void deactivate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs breakable surface and custom animation script commands.
 */
class WorldStateCommands {
public:
    static void breakable_surfaces_enable(int16_t function_index, uint32_t thread_index, char first);
    static void run_breakable_surfaces_reset(int16_t function_index, uint32_t thread_index, char first);
    static void custom_animation(int16_t function_index, uint32_t thread_index, char first);
    static void custom_animation_list(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs cheat_* script commands.
 */
class CheatCommands {
public:
    static void cheat_active_camouflage(int16_t function_index, uint32_t thread_index, char first);
    static void cheat_active_camouflage_local_player(int16_t function_index, uint32_t thread_index, char first);
    static void cheat_all_powerups(int16_t function_index, uint32_t thread_index, char first);
    static void cheat_all_vehicles(int16_t function_index, uint32_t thread_index, char first);
    static void run_cheat_all_weapons(int16_t function_index, uint32_t thread_index, char first);
    static void run_cheat_spawn_warthog(int16_t function_index, uint32_t thread_index, char first);
    static void run_cheat_teleport_to_camera(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs damage_* script commands.
 */
class DamageCommands {
public:
    static void damage_new(int16_t function_index, uint32_t thread_index, char first);
    static void damage_object(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs device_* script commands.
 */
class DeviceCommands {
public:
    static void device_get_position(int16_t function_index, uint32_t thread_index, char first);
    static void device_get_power(int16_t function_index, uint32_t thread_index, char first);
    static void device_group_change_only_once_more_set(int16_t function_index, uint32_t thread_index, char first);
    static void device_group_get(int16_t function_index, uint32_t thread_index, char first);
    static void device_group_set(int16_t function_index, uint32_t thread_index, char first);
    static void device_group_set_immediate(int16_t function_index, uint32_t thread_index, char first);
    static void device_one_sided_set(int16_t function_index, uint32_t thread_index, char first);
    static void device_operates_automatically_set(int16_t function_index, uint32_t thread_index, char first);
    static void device_set_never_appears_locked(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

}
