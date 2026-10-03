#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "crt.h"
#include "hs.h"

#include "halo/hs/hs1_command.hpp"

namespace halo::hs {

/**
 * Evaluate handlers of the hs ai_* script commands (query group).
 */
class AiQueryCommands {
public:
    static void ai(int16_t function_index, uint32_t thread_index, char first);
    static void actors(int16_t function_index, uint32_t thread_index, char first);
    static void is_attacking(int16_t function_index, uint32_t thread_index, char first);
    static void living_count(int16_t function_index, uint32_t thread_index, char first);
    static void living_fraction(int16_t function_index, uint32_t thread_index, char first);
    static void nonswarm_count(int16_t function_index, uint32_t thread_index, char first);
    static void status(int16_t function_index, uint32_t thread_index, char first);
    static void run_strength(int16_t function_index, uint32_t thread_index, char first);
    static void swarm_count(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs ai_* script commands (behaviour group).
 */
class AiBehaviourCommands {
public:
    static void allegiance(int16_t function_index, uint32_t thread_index, char first);
    static void allegiance_broken(int16_t function_index, uint32_t thread_index, char first);
    static void allegiance_remove(int16_t function_index, uint32_t thread_index, char first);
    static void automatic_migration_target(int16_t function_index, uint32_t thread_index, char first);
    static void braindead(int16_t function_index, uint32_t thread_index, char first);
    static void braindead_by_unit(int16_t function_index, uint32_t thread_index, char first);
    static void dialogue_triggers(int16_t function_index, uint32_t thread_index, char first);
    static void force_active(int16_t function_index, uint32_t thread_index, char first);
    static void force_active_by_unit(int16_t function_index, uint32_t thread_index, char first);
    static void link_activation(int16_t function_index, uint32_t thread_index, char first);
    static void set_current_state(int16_t function_index, uint32_t thread_index, char first);
    static void set_respawn(int16_t function_index, uint32_t thread_index, char first);
    static void set_return_state(int16_t function_index, uint32_t thread_index, char first);
    static void set_team(int16_t function_index, uint32_t thread_index, char first);
    static void timer_expire(int16_t function_index, uint32_t thread_index, char first);
    static void timer_start(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs ai_* script commands (targeting group).
 */
class AiTargetingCommands {
public:
    static void allow_charge(int16_t function_index, uint32_t thread_index, char first);
    static void allow_dormant(int16_t function_index, uint32_t thread_index, char first);
    static void attack(int16_t function_index, uint32_t thread_index, char first);
    static void berserk(int16_t function_index, uint32_t thread_index, char first);
    static void defend(int16_t function_index, uint32_t thread_index, char first);
    static void disregard(int16_t function_index, uint32_t thread_index, char first);
    static void follow_distance(int16_t function_index, uint32_t thread_index, char first);
    static void follow_target_ai(int16_t function_index, uint32_t thread_index, char first);
    static void follow_target_disable(int16_t function_index, uint32_t thread_index, char first);
    static void follow_target_players(int16_t function_index, uint32_t thread_index, char first);
    static void follow_target_unit(int16_t function_index, uint32_t thread_index, char first);
    static void grenades(int16_t function_index, uint32_t thread_index, char first);
    static void look_at_object(int16_t function_index, uint32_t thread_index, char first);
    static void magically_see_encounter(int16_t function_index, uint32_t thread_index, char first);
    static void magically_see_players(int16_t function_index, uint32_t thread_index, char first);
    static void magically_see_unit(int16_t function_index, uint32_t thread_index, char first);
    static void magically_see_units(int16_t function_index, uint32_t thread_index, char first);
    static void maneuver_enable(int16_t function_index, uint32_t thread_index, char first);
    static void playfight(int16_t function_index, uint32_t thread_index, char first);
    static void prefer_target(int16_t function_index, uint32_t thread_index, char first);
    static void run_retreat(int16_t function_index, uint32_t thread_index, char first);
    static void set_blind(int16_t function_index, uint32_t thread_index, char first);
    static void set_deaf(int16_t function_index, uint32_t thread_index, char first);
    static void stop_looking(int16_t function_index, uint32_t thread_index, char first);
    static void try_to_fight(int16_t function_index, uint32_t thread_index, char first);
    static void try_to_fight_nothing(int16_t function_index, uint32_t thread_index, char first);
    static void try_to_fight_player(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs ai_* script commands (placement group).
 */
class AiPlacementCommands {
public:
    static void attach(int16_t function_index, uint32_t thread_index, char first);
    static void attach_free(int16_t function_index, uint32_t thread_index, char first);
    static void attach_units(int16_t function_index, uint32_t thread_index, char first);
    static void detach(int16_t function_index, uint32_t thread_index, char first);
    static void detach_units(int16_t function_index, uint32_t thread_index, char first);
    static void erase(int16_t function_index, uint32_t thread_index, char first);
    static void erase_all(int16_t function_index, uint32_t thread_index, char first);
    static void run_free(int16_t function_index, uint32_t thread_index, char first);
    static void free_units(int16_t function_index, uint32_t thread_index, char first);
    static void kill(int16_t function_index, uint32_t thread_index, char first);
    static void kill_silent(int16_t function_index, uint32_t thread_index, char first);
    static void migrate(int16_t function_index, uint32_t thread_index, char first);
    static void migrate_and_speak(int16_t function_index, uint32_t thread_index, char first);
    static void migrate_by_unit(int16_t function_index, uint32_t thread_index, char first);
    static void place(int16_t function_index, uint32_t thread_index, char first);
    static void renew(int16_t function_index, uint32_t thread_index, char first);
    static void spawn_actor(int16_t function_index, uint32_t thread_index, char first);
    static void teleport_to_starting_location(int16_t function_index, uint32_t thread_index, char first);
    static void teleport_to_starting_location_if_unsupported(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs ai_* script commands (commandlist group).
 */
class AiCommandListCommands {
public:
    static void command_list(int16_t function_index, uint32_t thread_index, char first);
    static void command_list_advance(int16_t function_index, uint32_t thread_index, char first);
    static void command_list_advance_by_unit(int16_t function_index, uint32_t thread_index, char first);
    static void command_list_by_unit(int16_t function_index, uint32_t thread_index, char first);
    static void command_list_status(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs ai_* script commands (conversation group).
 */
class AiConversationCommands {
public:
    static void conversation(int16_t function_index, uint32_t thread_index, char first);
    static void conversation_advance(int16_t function_index, uint32_t thread_index, char first);
    static void conversation_line(int16_t function_index, uint32_t thread_index, char first);
    static void conversation_status(int16_t function_index, uint32_t thread_index, char first);
    static void conversation_stop(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs ai_* script commands (vehicle group).
 */
class AiVehicleCommands {
public:
    static void exit_vehicle(int16_t function_index, uint32_t thread_index, char first);
    static void go_to_vehicle(int16_t function_index, uint32_t thread_index, char first);
    static void go_to_vehicle_override(int16_t function_index, uint32_t thread_index, char first);
    static void going_to_vehicle(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_encounter(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_enterable_actor_type(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_enterable_actors(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_enterable_disable(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_enterable_distance(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_enterable_team(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

}
