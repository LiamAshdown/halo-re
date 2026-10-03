#include "halo/hs/hs1_ai_commands.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"

extern "C" {
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b);
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern uint32_t team_pair_override_remove(int16_t index_a, int16_t index_b);
extern Scenario *global_scenario;
}

namespace halo::hs {

/**
 * Evaluate handler for hs function "ai" (boolean -> void).
 *
 * @address 0x482850
 */
void AiQueryCommands::ai(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::globals().state->ai_active = (uint8_t)arguments[0];
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_actors" (ai -> object_list).
 *
 * @address 0x47eb80
 */
void AiQueryCommands::actors(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)halo::ai::ai_reference_build_object_list((uint32_t)arguments[0]), thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_is_attacking" (ai -> boolean).
 *
 * @address 0x47e830
 */
void AiQueryCommands::is_attacking(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint8_t)(halo::ai::ai_platoon_range_has_available((uint32_t)arguments[0])), thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_living_count" (ai -> short).
 *
 * @address 0x47e950
 */
void AiQueryCommands::living_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint16_t)halo::ai::ai_reference_get_stat_pair((uint32_t)arguments[0], 0, 0, 0), thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_living_fraction" (ai -> real).
 *
 * @address 0x47e9b0
 */
void AiQueryCommands::living_fraction(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t total = 0;
        int32_t living = (int32_t)halo::ai::ai_reference_get_stat_pair((uint32_t)arguments[0], 0, &total, 0);
        float fraction = 0.0f;

        if (total > 0) {
            fraction = (float)living / (float)total;
        }
        hs_thread_return(*(int32_t *)&fraction, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_nonswarm_count" (ai -> short).
 *
 * @address 0x47eb10
 */
void AiQueryCommands::nonswarm_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint32_t count = halo::ai::ai_reference_get_stat_pair((uint32_t)arguments[0], 2, 0, 0);
        hs_thread_return((int32_t)(uint16_t)count, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_status" (ai -> short).
 *
 * @address 0x47ebd0
 */
void AiQueryCommands::status(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t result = halo::ai::ai_reference_max_activity_stage((uint32_t)arguments[0]);
        hs_thread_return((int32_t)(uint16_t)result, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_strength" (ai -> real).
 *
 * @address 0x47ea40
 */
void AiQueryCommands::run_strength(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t strength = 0;

    halo::ai::ai_reference_get_stat_pair((uint32_t)arguments[0], 0, 0, &strength);
    hs_thread_return((int32_t)strength, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_swarm_count" (ai -> short).
 *
 * @address 0x47eaa0
 */
void AiQueryCommands::swarm_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint16_t)(halo::ai::ai_reference_get_stat_pair((uint32_t)arguments[0], 1, 0, 0)), thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_query_commands_entries[] = {
    {"ai", &AiQueryCommands::ai},
    {"ai_actors", &AiQueryCommands::actors},
    {"ai_is_attacking", &AiQueryCommands::is_attacking},
    {"ai_living_count", &AiQueryCommands::living_count},
    {"ai_living_fraction", &AiQueryCommands::living_fraction},
    {"ai_nonswarm_count", &AiQueryCommands::nonswarm_count},
    {"ai_status", &AiQueryCommands::status},
    {"ai_strength", &AiQueryCommands::run_strength},
    {"ai_swarm_count", &AiQueryCommands::swarm_count},
};
constexpr ScriptCommandGroup k_ai_query_commands_group(k_ai_query_commands_entries, sizeof(k_ai_query_commands_entries) / sizeof(k_ai_query_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiQueryCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiQueryCommands::commands()
{
    return k_ai_query_commands_group;
}

/**
 * Evaluate handler for hs function "ai_allegiance" (team, team -> void).
 *
 * @address 0x47d920
 */
void AiBehaviourCommands::allegiance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_category_matches_wildcard(*(int16_t *)&arguments[0], *(int16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_allegiance_broken" (team, team -> boolean).
 *
 * @address 0x47ed50
 */
void AiBehaviourCommands::allegiance_broken(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t team_a = *(int16_t *)&arguments[0];
        int16_t team_b = *(int16_t *)&arguments[1];
        uint8_t broken = 0;

        if (team_a != -1 && team_b != -1 && team_pair_flag_test(team_a, team_b) &&
            teams_are_enemies(team_b, team_a)) {
            broken = 1;
        }
        hs_thread_return((int32_t)broken, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_allegiance_remove" (team, team -> void).
 *
 * @address 0x47d970
 */
void AiBehaviourCommands::allegiance_remove(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t first_team = *(int16_t *)&arguments[0];
    int16_t second_team = *(int16_t *)&arguments[1];

    if (first_team != -1 && second_team != -1) {
        team_pair_override_remove(second_team, first_team);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_automatic_migration_target" (ai, boolean -> void).
 *
 * @address 0x47e380
 */
void AiBehaviourCommands::automatic_migration_target(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_squad_set_automatic_migration((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_braindead" (ai, boolean -> void).
 *
 * @address 0x47dab0
 */
void AiBehaviourCommands::braindead(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_reset_or_wake_awareness((uint32_t)arguments[0], *(char *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_braindead_by_unit" (object_list, boolean -> void).
 *
 * @address 0x47db00
 */
void AiBehaviourCommands::braindead_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_reset_or_wake_awareness((datum_index)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_dialogue_triggers" (boolean -> void).
 *
 * @address 0x47cf30
 */
void AiBehaviourCommands::dialogue_triggers(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::globals().state->dialogue_triggers_enabled = *(uint8_t *)&arguments[0];
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_force_active" (ai, boolean -> void).
 *
 * @address 0x47df00
 */
void AiBehaviourCommands::force_active(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t reference = (uint32_t)arguments[0];

    if (halo::ai::globals().state->actors_valid && reference != halo::k_dword_none && (int32_t)(reference & halo::k_slot_mask) < *(int32_t *)&global_scenario->encounters.count) {
        ((uint8_t *)halo::ai::globals().encounter_data->data)[(reference & halo::k_slot_mask) * 0x6c + 0xc] = *(uint8_t *)&arguments[1];
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_force_active_by_unit" (unit, boolean -> void).
 *
 * @address 0x47df80
 */
void AiBehaviourCommands::force_active_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_unit_set_actor_force_active((datum_index)arguments[0], (uint8_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_link_activation" (ai, ai -> void).
 *
 * @address 0x47e690
 */
void AiBehaviourCommands::link_activation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t first_reference = (uint32_t)arguments[0];
    uint32_t second_reference = (uint32_t)arguments[1];

    if (first_reference != halo::k_dword_none && second_reference != halo::k_dword_none) {
        halo::ai::ai_encounter_record_recent_zone(first_reference & halo::k_slot_mask, (int16_t)second_reference);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_set_current_state" (ai, ai_default_state -> void).
 *
 * @address 0x47e020
 */
void AiBehaviourCommands::set_current_state(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::squad_members_request_order((uint32_t)arguments[0], (int16_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_set_respawn" (ai, boolean -> void).
 *
 * @address 0x47d3b0
 */
void AiBehaviourCommands::set_respawn(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1 && halo::ai::globals().state->actors_valid != 0) {
            uint32_t index = (uint32_t)arguments[0] & halo::k_slot_mask;
            uint8_t *encounter = (uint8_t *)halo::ai::globals().encounter_data->data + index * 0x6c;

            encounter[0x3c] = (uint8_t)arguments[1];
            *(int16_t *)((uint8_t *)halo::ai::globals().encounter_data->data + index * 0x6c + 0xe) = 0x96;
            halo::ai::encounter_activate((datum_index)index);
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_set_return_state" (ai, ai_default_state -> void).
 *
 * @address 0x47dfd0
 */
void AiBehaviourCommands::set_return_state(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::squad_members_assign_team_and_request_order((uint32_t)arguments[0], (int16_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_set_team" (ai, team -> void).
 *
 * @address 0x47e740
 */
void AiBehaviourCommands::set_team(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::encounter_set_team((datum_index)arguments[0], (int16_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_timer_expire" (ai -> void).
 *
 * @address 0x47d6a0
 */
void AiBehaviourCommands::timer_expire(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_for_each_squad((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_timer_start" (ai -> void).
 *
 * @address 0x47d660
 */
void AiBehaviourCommands::timer_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_start_squad_timers((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_behaviour_commands_entries[] = {
    {"ai_allegiance", &AiBehaviourCommands::allegiance},
    {"ai_allegiance_broken", &AiBehaviourCommands::allegiance_broken},
    {"ai_allegiance_remove", &AiBehaviourCommands::allegiance_remove},
    {"ai_automatic_migration_target", &AiBehaviourCommands::automatic_migration_target},
    {"ai_braindead", &AiBehaviourCommands::braindead},
    {"ai_braindead_by_unit", &AiBehaviourCommands::braindead_by_unit},
    {"ai_dialogue_triggers", &AiBehaviourCommands::dialogue_triggers},
    {"ai_force_active", &AiBehaviourCommands::force_active},
    {"ai_force_active_by_unit", &AiBehaviourCommands::force_active_by_unit},
    {"ai_link_activation", &AiBehaviourCommands::link_activation},
    {"ai_set_current_state", &AiBehaviourCommands::set_current_state},
    {"ai_set_respawn", &AiBehaviourCommands::set_respawn},
    {"ai_set_return_state", &AiBehaviourCommands::set_return_state},
    {"ai_set_team", &AiBehaviourCommands::set_team},
    {"ai_timer_expire", &AiBehaviourCommands::timer_expire},
    {"ai_timer_start", &AiBehaviourCommands::timer_start},
};
constexpr ScriptCommandGroup k_ai_behaviour_commands_group(k_ai_behaviour_commands_entries, sizeof(k_ai_behaviour_commands_entries) / sizeof(k_ai_behaviour_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiBehaviourCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiBehaviourCommands::commands()
{
    return k_ai_behaviour_commands_group;
}

/**
 * Evaluate handler for hs function "ai_allow_charge" (ai, boolean -> void).
 *
 * @address 0x47e790
 */
void AiTargetingCommands::allow_charge(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_set_charge_allowed((uint32_t)arguments[0], (char)(uint8_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_allow_dormant" (ai, boolean -> void).
 *
 * @address 0x47e7e0
 */
void AiTargetingCommands::allow_dormant(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_set_squads_dormancy_allowed((uint32_t)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_attack" (ai -> void).
 *
 * @address 0x47d6e0
 */
void AiTargetingCommands::attack(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_platoon_range_clear_defending((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_berserk" (ai, boolean -> void).
 *
 * @address 0x47e6f0
 */
void AiTargetingCommands::berserk(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_set_combat_alert_flag((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_defend" (ai -> void).
 *
 * @address 0x47d720
 */
void AiTargetingCommands::defend(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_platoon_range_set_defending((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_disregard" (object_list, boolean -> void).
 *
 * @address 0x47db50
 */
void AiTargetingCommands::disregard(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_set_unit_flag_400((datum_index)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_follow_distance" (ai, real -> void).
 *
 * @address 0x47e590
 */
void AiTargetingCommands::follow_distance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            *(uint32_t *)((uint8_t *)halo::ai::globals().encounter_data->data + (arguments[0] & halo::k_slot_mask) * 0x6c + 0x68) = (uint32_t)arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_follow_target_ai" (ai, ai -> void).
 *
 * @address 0x47e510
 */
void AiTargetingCommands::follow_target_ai(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *encounter = (uint8_t *)halo::ai::globals().encounter_data->data + (arguments[0] & halo::k_slot_mask) * 0x6c;

            if ((uint32_t)arguments[1] == halo::k_dword_none) {
                ((struct encounter *)encounter)->follow_target_type = 0;
            } else {
                ((struct encounter *)encounter)->follow_target_type = 3;
                *(uint32_t *)&((struct encounter *)encounter)->follow_target = (uint32_t)arguments[1];
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_follow_target_disable" (ai -> void).
 *
 * @address 0x47e3d0
 */
void AiTargetingCommands::follow_target_disable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t reference = (uint32_t)arguments[0];

    if (reference != halo::k_dword_none) {
        *(int16_t *)((uint8_t *)halo::ai::globals().encounter_data->data + (reference & halo::k_slot_mask) * 0x6c + 0x62) = 0;
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_follow_target_players" (ai -> void).
 *
 * @address 0x47e430
 */
void AiTargetingCommands::follow_target_players(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t reference = (uint32_t)arguments[0];

    if (reference != halo::k_dword_none) {
        *(int16_t *)((uint8_t *)halo::ai::globals().encounter_data->data + (reference & halo::k_slot_mask) * 0x6c + 0x62) = 1;
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_follow_target_unit" (ai, unit -> void).
 *
 * @address 0x47e490
 */
void AiTargetingCommands::follow_target_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *encounter = (uint8_t *)halo::ai::globals().encounter_data->data + ((uint32_t)arguments[0] & halo::k_slot_mask) * 0x6c;

            if (arguments[1] == -1) {
                ((struct encounter *)encounter)->follow_target_type = 0;
            } else {
                ((struct encounter *)encounter)->follow_target_type = 2;
                ((struct encounter *)encounter)->follow_target = arguments[1];
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_grenades" (boolean -> void).
 *
 * @address 0x47cf80
 */
void AiTargetingCommands::grenades(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::globals().state->grenades_enabled = *(uint8_t *)&arguments[0];
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_look_at_object" (unit, object -> void).
 *
 * @address 0x47e2f0
 */
void AiTargetingCommands::look_at_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_unit_dispatch_actor_event_d((datum_index)arguments[0], arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_magically_see_encounter" (ai, ai -> void).
 *
 * @address 0x47d520
 */
void AiTargetingCommands::magically_see_encounter(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_respawn_placed_members((uint32_t)arguments[1], (uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_magically_see_players" (ai -> void).
 *
 * @address 0x47d570
 */
void AiTargetingCommands::magically_see_players(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_respawn_all_players((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_magically_see_unit" (ai, unit -> void).
 *
 * @address 0x47d5c0
 */
void AiTargetingCommands::magically_see_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_respawn_member((uint32_t)arguments[0], (datum_index)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * The evaluate handler of the orphan hs function record 0x658c18 "ai_magically_see_units" (ai, object_list
 * -> void), not in hs_function_definitions.
 *
 * @address 0x47d610
 */
void AiTargetingCommands::magically_see_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_respawn_members((datum_index)arguments[1], (uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_maneuver_enable" (ai, boolean -> void).
 *
 * @address 0x47d7a0
 */
void AiTargetingCommands::maneuver_enable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_platoon_range_set_maneuver_enabled((uint32_t)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_playfight" (ai, boolean -> void).
 *
 * @address 0x47e070
 */
void AiTargetingCommands::playfight(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t reference = (uint32_t)arguments[0];

    if (reference != halo::k_dword_none) {
        ((uint8_t *)halo::ai::globals().encounter_data->data)[(reference & halo::k_slot_mask) * 0x6c + 0x60] = *(uint8_t *)&arguments[1];
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_prefer_target" (object_list, boolean -> void).
 *
 * @address 0x47dba0
 */
void AiTargetingCommands::prefer_target(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_set_unit_flag_800((datum_index)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * The evaluate handler of hs functions 180 "ai_retreat" (ai -> void); 181 "ai_maneuver" (ai -> void).
 *
 * @address 0x47d760
 */
void AiTargetingCommands::run_retreat(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_platoon_range_set_maneuvering((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_set_blind" (ai, boolean -> void).
 *
 * @address 0x47d4b0
 */
void AiTargetingCommands::set_blind(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none && halo::ai::globals().state->actors_valid != 0) {
            ((uint8_t *)halo::ai::globals().encounter_data->data)[(arguments[0] & halo::k_slot_mask) * 0x6c + 0x40] = *(uint8_t *)&arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_set_deaf" (ai, boolean -> void).
 *
 * @address 0x47d440
 */
void AiTargetingCommands::set_deaf(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none && halo::ai::globals().state->actors_valid != 0) {
            ((uint8_t *)halo::ai::globals().encounter_data->data)[(arguments[0] & halo::k_slot_mask) * 0x6c + 0x41] = *(uint8_t *)&arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_stop_looking" (unit -> void).
 *
 * @address 0x47e340
 */
void AiTargetingCommands::stop_looking(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_unit_clear_actor_vocalization((datum_index)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_try_to_fight" (ai, ai -> void).
 *
 * @address 0x47dd10
 */
void AiTargetingCommands::try_to_fight(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_set_search_target_point((uint32_t)arguments[0], (uint32_t)arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_try_to_fight_nothing" (ai -> void).
 *
 * @address 0x47dcd0
 */
void AiTargetingCommands::try_to_fight_nothing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_clear_search_target((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_try_to_fight_player" (ai -> void).
 *
 * @address 0x47dd60
 */
void AiTargetingCommands::try_to_fight_player(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_set_search_target_area((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_targeting_commands_entries[] = {
    {"ai_allow_charge", &AiTargetingCommands::allow_charge},
    {"ai_allow_dormant", &AiTargetingCommands::allow_dormant},
    {"ai_attack", &AiTargetingCommands::attack},
    {"ai_berserk", &AiTargetingCommands::berserk},
    {"ai_defend", &AiTargetingCommands::defend},
    {"ai_disregard", &AiTargetingCommands::disregard},
    {"ai_follow_distance", &AiTargetingCommands::follow_distance},
    {"ai_follow_target_ai", &AiTargetingCommands::follow_target_ai},
    {"ai_follow_target_disable", &AiTargetingCommands::follow_target_disable},
    {"ai_follow_target_players", &AiTargetingCommands::follow_target_players},
    {"ai_follow_target_unit", &AiTargetingCommands::follow_target_unit},
    {"ai_grenades", &AiTargetingCommands::grenades},
    {"ai_look_at_object", &AiTargetingCommands::look_at_object},
    {"ai_magically_see_encounter", &AiTargetingCommands::magically_see_encounter},
    {"ai_magically_see_players", &AiTargetingCommands::magically_see_players},
    {"ai_magically_see_unit", &AiTargetingCommands::magically_see_unit},
    {"ai_magically_see_units", &AiTargetingCommands::magically_see_units},
    {"ai_maneuver_enable", &AiTargetingCommands::maneuver_enable},
    {"ai_playfight", &AiTargetingCommands::playfight},
    {"ai_prefer_target", &AiTargetingCommands::prefer_target},
    {"ai_retreat", &AiTargetingCommands::run_retreat},
    {"ai_set_blind", &AiTargetingCommands::set_blind},
    {"ai_set_deaf", &AiTargetingCommands::set_deaf},
    {"ai_stop_looking", &AiTargetingCommands::stop_looking},
    {"ai_try_to_fight", &AiTargetingCommands::try_to_fight},
    {"ai_try_to_fight_nothing", &AiTargetingCommands::try_to_fight_nothing},
    {"ai_try_to_fight_player", &AiTargetingCommands::try_to_fight_player},
};
constexpr ScriptCommandGroup k_ai_targeting_commands_group(k_ai_targeting_commands_entries, sizeof(k_ai_targeting_commands_entries) / sizeof(k_ai_targeting_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiTargetingCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiTargetingCommands::commands()
{
    return k_ai_targeting_commands_group;
}

/**
 * Evaluate handler for hs function "ai_attach" (unit, ai -> void).
 *
 * @address 0x47d050
 */
void AiPlacementCommands::attach(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_spawn_starting_location_object((datum_index)arguments[0], (uint32_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_attach_free" (unit, actor_variant -> void).
 *
 * @address 0x47d0f0
 */
void AiPlacementCommands::attach_free(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_unit_create_actor((datum_index)arguments[1], (datum_index)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * The evaluate handler of the orphan hs function record 0x6589e0 "ai_attach_units" (object_list, ai ->
 * void), not in hs_function_definitions.
 *
 * @address 0x47d0a0
 */
void AiPlacementCommands::attach_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_spawn_members((datum_index)arguments[0], (uint32_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_detach" (unit -> void).
 *
 * @address 0x47d140
 */
void AiPlacementCommands::detach(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none) {
        datum_index actor = *(datum_index *)(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit & halo::k_slot_mask) * 0xc + 8) + 0x1f4);

        if (actor != k_datum_index_none) {
            halo::ai::actor_delete(actor, 0);
        }
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * The evaluate handler of the orphan hs function record 0x658a40 "ai_detach_units" (object_list -> void),
 * not in hs_function_definitions.
 *
 * @address 0x47d1b0
 */
void AiPlacementCommands::detach_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_clear_orders_with_weapon((datum_index)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_erase" (ai -> void).
 *
 * @address 0x47d2e0
 */
void AiPlacementCommands::erase(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_notify_squad_index((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_erase_all" (-> void).
 *
 * @address 0x47d330
 */
void AiPlacementCommands::erase_all(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    halo::ai::ai_release_actors_filtered(halo::k_dword_none, -1, -1, 0);
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "ai_free" (ai -> void).
 *
 * @address 0x47cfd0
 */
void AiPlacementCommands::run_free(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_detach_actors_from_encounters((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_free_units" (object_list -> void).
 *
 * @address 0x47d010
 */
void AiPlacementCommands::free_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_detach_actors_from_encounters((datum_index)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_kill" (ai -> void).
 *
 * @address 0x47d240
 */
void AiPlacementCommands::kill(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_notify_actors((uint32_t)arguments[0], 0);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_kill_silent" (ai -> void).
 *
 * @address 0x47d290
 */
void AiPlacementCommands::kill_silent(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_notify_actors((uint32_t)arguments[0], 1);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_migrate" (ai, ai -> void).
 *
 * @address 0x47d7f0
 */
void AiPlacementCommands::migrate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_squads_merge((uint32_t)arguments[0], (uint32_t)arguments[1], 0, 0);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_migrate_and_speak" (ai, ai, string -> void).
 *
 * @address 0x47d840
 */
void AiPlacementCommands::migrate_and_speak(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        char *verb = (char *)arguments[2];
        char advance = 0;

        if (_stricmp(verb, "advance") == 0) {
            advance = 1;
        } else {
            _stricmp(verb, "retreat");
        }
        halo::ai::ai_squads_merge((uint32_t)arguments[0], (uint32_t)arguments[1], 1, advance);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_migrate_by_unit" (object_list, ai -> void).
 *
 * @address 0x47d8d0
 */
void AiPlacementCommands::migrate_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_remap_units_and_children((datum_index)arguments[0], (uint32_t)arguments[1], 0);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_place" (ai -> void).
 *
 * @address 0x47d1f0
 */
void AiPlacementCommands::place(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_activate_squads((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_renew" (ai -> void).
 *
 * @address 0x47dc90
 */
void AiPlacementCommands::renew(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_refill_grenades((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_spawn_actor" (ai -> void).
 *
 * @address 0x47d360
 */
void AiPlacementCommands::spawn_actor(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_resolve_squad_datum((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_teleport_to_starting_location" (ai -> void).
 *
 * @address 0x47dbf0
 */
void AiPlacementCommands::teleport_to_starting_location(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_face_starting_location((uint32_t)arguments[0], 0);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_teleport_to_starting_location_if_unsupported" (ai -> void).
 *
 * @address 0x47dc40
 */
void AiPlacementCommands::teleport_to_starting_location_if_unsupported(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_face_starting_location((uint32_t)arguments[0], 1);
        hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_placement_commands_entries[] = {
    {"ai_attach", &AiPlacementCommands::attach},
    {"ai_attach_free", &AiPlacementCommands::attach_free},
    {"ai_attach_units", &AiPlacementCommands::attach_units},
    {"ai_detach", &AiPlacementCommands::detach},
    {"ai_detach_units", &AiPlacementCommands::detach_units},
    {"ai_erase", &AiPlacementCommands::erase},
    {"ai_erase_all", &AiPlacementCommands::erase_all},
    {"ai_free", &AiPlacementCommands::run_free},
    {"ai_free_units", &AiPlacementCommands::free_units},
    {"ai_kill", &AiPlacementCommands::kill},
    {"ai_kill_silent", &AiPlacementCommands::kill_silent},
    {"ai_migrate", &AiPlacementCommands::migrate},
    {"ai_migrate_and_speak", &AiPlacementCommands::migrate_and_speak},
    {"ai_migrate_by_unit", &AiPlacementCommands::migrate_by_unit},
    {"ai_place", &AiPlacementCommands::place},
    {"ai_renew", &AiPlacementCommands::renew},
    {"ai_spawn_actor", &AiPlacementCommands::spawn_actor},
    {"ai_teleport_to_starting_location", &AiPlacementCommands::teleport_to_starting_location},
    {"ai_teleport_to_starting_location_if_unsupported", &AiPlacementCommands::teleport_to_starting_location_if_unsupported},
};
constexpr ScriptCommandGroup k_ai_placement_commands_group(k_ai_placement_commands_entries, sizeof(k_ai_placement_commands_entries) / sizeof(k_ai_placement_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiPlacementCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiPlacementCommands::commands()
{
    return k_ai_placement_commands_group;
}

/**
 * Evaluate handler for hs function "ai_command_list" (ai, ai_command_list -> void).
 *
 * @address 0x47dda0
 */
void AiCommandListCommands::command_list(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_flee_if_ready((uint32_t)arguments[0], *(uint16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_command_list_advance" (ai -> void).
 *
 * @address 0x47de40
 */
void AiCommandListCommands::command_list_advance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_reference_invoke_squad_callback_406f80((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_command_list_advance_by_unit" (unit -> void).
 *
 * @address 0x47de80
 */
void AiCommandListCommands::command_list_advance_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], 3);

            if (unit != 0) {
                if (*(int32_t *)&((unit_object *)unit)->unit.actor_index != -1) {
                    halo::ai::actor_swarm_for_each_component_thunk(*(uint32_t *)&((unit_object *)unit)->unit.actor_index);
                } else if (*(int32_t *)&((unit_object *)unit)->unit.swarm_actor_index != -1) {
                    halo::ai::actor_swarm_for_each_component_thunk(*(uint32_t *)&((unit_object *)unit)->unit.swarm_actor_index);
                }
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_command_list_by_unit" (unit, ai_command_list -> void).
 *
 * @address 0x47ddf0
 */
void AiCommandListCommands::command_list_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_unit_flee_if_ready((datum_index)arguments[0], *(uint16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_command_list_status" (object_list -> short).
 *
 * @address 0x47e890
 */
void AiCommandListCommands::command_list_status(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint16_t)halo::ai::ai_object_list_max_flee_grade((datum_index)arguments[0]), thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_command_list_commands_entries[] = {
    {"ai_command_list", &AiCommandListCommands::command_list},
    {"ai_command_list_advance", &AiCommandListCommands::command_list_advance},
    {"ai_command_list_advance_by_unit", &AiCommandListCommands::command_list_advance_by_unit},
    {"ai_command_list_by_unit", &AiCommandListCommands::command_list_by_unit},
    {"ai_command_list_status", &AiCommandListCommands::command_list_status},
};
constexpr ScriptCommandGroup k_ai_command_list_commands_group(k_ai_command_list_commands_entries, sizeof(k_ai_command_list_commands_entries) / sizeof(k_ai_command_list_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiCommandListCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiCommandListCommands::commands()
{
    return k_ai_command_list_commands_group;
}

/**
 * Evaluate handler for hs function "ai_conversation" (conversation -> boolean).
 *
 * @address 0x47ec30
 */
void AiConversationCommands::conversation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)halo::ai::ai_conversation_activate(*(int16_t *)&arguments[0], 1), thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_conversation_advance" (conversation -> void).
 *
 * @address 0x47e640
 */
void AiConversationCommands::conversation_advance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_conversation_mark_all(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_conversation_line" (conversation -> short).
 *
 * @address 0x47ec90
 */
void AiConversationCommands::conversation_line(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t line = halo::ai::ai_conversation_get_line_index(*(int16_t *)&arguments[0]);
        hs_thread_return((int32_t)(uint16_t)line, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_conversation_status" (conversation -> short).
 *
 * @address 0x47ecf0
 */
void AiConversationCommands::conversation_status(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint16_t)halo::ai::ai_conversation_get_status(*(int16_t *)&arguments[0]), thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_conversation_stop" (conversation -> void).
 *
 * @address 0x47e5f0
 */
void AiConversationCommands::conversation_stop(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_conversation_stop_all(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_conversation_commands_entries[] = {
    {"ai_conversation", &AiConversationCommands::conversation},
    {"ai_conversation_advance", &AiConversationCommands::conversation_advance},
    {"ai_conversation_line", &AiConversationCommands::conversation_line},
    {"ai_conversation_status", &AiConversationCommands::conversation_status},
    {"ai_conversation_stop", &AiConversationCommands::conversation_stop},
};
constexpr ScriptCommandGroup k_ai_conversation_commands_group(k_ai_conversation_commands_entries, sizeof(k_ai_conversation_commands_entries) / sizeof(k_ai_conversation_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiConversationCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiConversationCommands::commands()
{
    return k_ai_conversation_commands_group;
}

/**
 * Evaluate handler for hs function "ai_exit_vehicle" (ai -> void).
 *
 * @address 0x47da70
 */
void AiVehicleCommands::exit_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_reference_units_exit_vehicles((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_go_to_vehicle" (ai, unit, string -> void).
 *
 * @address 0x47d9d0
 */
void AiVehicleCommands::go_to_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_process_nearby_actors((uint32_t)arguments[0], (datum_index)arguments[1], (char *)arguments[2], 0);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_go_to_vehicle_override" (ai, unit, string -> void).
 *
 * @address 0x47da20
 */
void AiVehicleCommands::go_to_vehicle_override(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_process_nearby_actors((uint32_t)arguments[0], (datum_index)arguments[1], (char *)arguments[2], 1);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_going_to_vehicle" (unit -> short).
 *
 * @address 0x47e8f0
 */
void AiVehicleCommands::going_to_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t count = halo::ai::ai_count_actors_in_mode9_group((int32_t)arguments[0]);
        hs_thread_return((int32_t)(uint16_t)count, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_vehicle_encounter" (unit, ai -> void).
 *
 * @address 0x47e0d0
 */
void AiVehicleCommands::vehicle_encounter(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_unit_set_squad_reference((datum_index)arguments[0], (uint32_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_vehicle_enterable_actor_type" (unit, actor_type -> void).
 *
 * @address 0x47e1e0
 */
void AiVehicleCommands::vehicle_enterable_actor_type(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *record = (uint8_t *)halo::ai::ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0) {
                *(uint16_t *)(record + 0xa) |= (uint16_t)(1u << (*(uint8_t *)&arguments[1] & 0x1f));
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_vehicle_enterable_actors" (unit, ai -> void).
 *
 * @address 0x47e240
 */
void AiVehicleCommands::vehicle_enterable_actors(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1 && arguments[1] != -1) {
            uint8_t *record = (uint8_t *)halo::ai::ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0 && *(int16_t *)(record + 0xc) < 6) {
                ((int32_t *)(record + 0x10))[*(int16_t *)(record + 0xc)] = arguments[1];
                *(int16_t *)(record + 0xc) += 1;
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_vehicle_enterable_disable" (unit -> void).
 *
 * @address 0x47e2b0
 */
void AiVehicleCommands::vehicle_enterable_disable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_attention_remove((datum_index)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_vehicle_enterable_distance" (unit, real -> void).
 *
 * @address 0x47e120
 */
void AiVehicleCommands::vehicle_enterable_distance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *record = (uint8_t *)halo::ai::ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0) {
                *(float *)(record + 0x4) = *(float *)&arguments[1];
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "ai_vehicle_enterable_team" (unit, team -> void).
 *
 * @address 0x47e180
 */
void AiVehicleCommands::vehicle_enterable_team(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *record = (uint8_t *)halo::ai::ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0) {
                *(uint16_t *)(record + 8) |= (uint16_t)(1u << ((int16_t)arguments[1] & 0x1f));
            }
        }
        hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_ai_vehicle_commands_entries[] = {
    {"ai_exit_vehicle", &AiVehicleCommands::exit_vehicle},
    {"ai_go_to_vehicle", &AiVehicleCommands::go_to_vehicle},
    {"ai_go_to_vehicle_override", &AiVehicleCommands::go_to_vehicle_override},
    {"ai_going_to_vehicle", &AiVehicleCommands::going_to_vehicle},
    {"ai_vehicle_encounter", &AiVehicleCommands::vehicle_encounter},
    {"ai_vehicle_enterable_actor_type", &AiVehicleCommands::vehicle_enterable_actor_type},
    {"ai_vehicle_enterable_actors", &AiVehicleCommands::vehicle_enterable_actors},
    {"ai_vehicle_enterable_disable", &AiVehicleCommands::vehicle_enterable_disable},
    {"ai_vehicle_enterable_distance", &AiVehicleCommands::vehicle_enterable_distance},
    {"ai_vehicle_enterable_team", &AiVehicleCommands::vehicle_enterable_team},
};
constexpr ScriptCommandGroup k_ai_vehicle_commands_group(k_ai_vehicle_commands_entries, sizeof(k_ai_vehicle_commands_entries) / sizeof(k_ai_vehicle_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by AiVehicleCommands, keyed by script function name.
 */
const ScriptCommandGroup &AiVehicleCommands::commands()
{
    return k_ai_vehicle_commands_group;
}

}

extern "C" {

void hs_evaluate_ai(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::ai(function_index, thread_index, first);
}

void hs_evaluate_ai_actors(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::actors(function_index, thread_index, first);
}

void hs_evaluate_ai_is_attacking(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::is_attacking(function_index, thread_index, first);
}

void hs_evaluate_ai_living_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::living_count(function_index, thread_index, first);
}

void hs_evaluate_ai_living_fraction(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::living_fraction(function_index, thread_index, first);
}

void hs_evaluate_ai_nonswarm_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::nonswarm_count(function_index, thread_index, first);
}

void hs_evaluate_ai_status(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::status(function_index, thread_index, first);
}

void hs_evaluate_ai_strength(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::run_strength(function_index, thread_index, first);
}

void hs_evaluate_ai_swarm_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiQueryCommands::swarm_count(function_index, thread_index, first);
}

void hs_evaluate_ai_allegiance(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::allegiance(function_index, thread_index, first);
}

void hs_evaluate_ai_allegiance_broken(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::allegiance_broken(function_index, thread_index, first);
}

void hs_evaluate_ai_allegiance_remove(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::allegiance_remove(function_index, thread_index, first);
}

void hs_evaluate_ai_automatic_migration_target(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::automatic_migration_target(function_index, thread_index, first);
}

void hs_evaluate_ai_braindead(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::braindead(function_index, thread_index, first);
}

void hs_evaluate_ai_braindead_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::braindead_by_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_dialogue_triggers(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::dialogue_triggers(function_index, thread_index, first);
}

void hs_evaluate_ai_force_active(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::force_active(function_index, thread_index, first);
}

void hs_evaluate_ai_force_active_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::force_active_by_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_link_activation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::link_activation(function_index, thread_index, first);
}

void hs_evaluate_ai_set_current_state(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::set_current_state(function_index, thread_index, first);
}

void hs_evaluate_ai_set_respawn(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::set_respawn(function_index, thread_index, first);
}

void hs_evaluate_ai_set_return_state(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::set_return_state(function_index, thread_index, first);
}

void hs_evaluate_ai_set_team(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::set_team(function_index, thread_index, first);
}

void hs_evaluate_ai_timer_expire(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::timer_expire(function_index, thread_index, first);
}

void hs_evaluate_ai_timer_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiBehaviourCommands::timer_start(function_index, thread_index, first);
}

void hs_evaluate_ai_allow_charge(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::allow_charge(function_index, thread_index, first);
}

void hs_evaluate_ai_allow_dormant(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::allow_dormant(function_index, thread_index, first);
}

void hs_evaluate_ai_attack(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::attack(function_index, thread_index, first);
}

void hs_evaluate_ai_berserk(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::berserk(function_index, thread_index, first);
}

void hs_evaluate_ai_defend(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::defend(function_index, thread_index, first);
}

void hs_evaluate_ai_disregard(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::disregard(function_index, thread_index, first);
}

void hs_evaluate_ai_follow_distance(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::follow_distance(function_index, thread_index, first);
}

void hs_evaluate_ai_follow_target_ai(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::follow_target_ai(function_index, thread_index, first);
}

void hs_evaluate_ai_follow_target_disable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::follow_target_disable(function_index, thread_index, first);
}

void hs_evaluate_ai_follow_target_players(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::follow_target_players(function_index, thread_index, first);
}

void hs_evaluate_ai_follow_target_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::follow_target_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_grenades(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::grenades(function_index, thread_index, first);
}

void hs_evaluate_ai_look_at_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::look_at_object(function_index, thread_index, first);
}

void hs_evaluate_ai_magically_see_encounter(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::magically_see_encounter(function_index, thread_index, first);
}

void hs_evaluate_ai_magically_see_players(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::magically_see_players(function_index, thread_index, first);
}

void hs_evaluate_ai_magically_see_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::magically_see_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_magically_see_units(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::magically_see_units(function_index, thread_index, first);
}

void hs_evaluate_ai_maneuver_enable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::maneuver_enable(function_index, thread_index, first);
}

void hs_evaluate_ai_playfight(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::playfight(function_index, thread_index, first);
}

void hs_evaluate_ai_prefer_target(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::prefer_target(function_index, thread_index, first);
}

void hs_evaluate_ai_retreat(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::run_retreat(function_index, thread_index, first);
}

void hs_evaluate_ai_set_blind(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::set_blind(function_index, thread_index, first);
}

void hs_evaluate_ai_set_deaf(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::set_deaf(function_index, thread_index, first);
}

void hs_evaluate_ai_stop_looking(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::stop_looking(function_index, thread_index, first);
}

void hs_evaluate_ai_try_to_fight(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::try_to_fight(function_index, thread_index, first);
}

void hs_evaluate_ai_try_to_fight_nothing(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::try_to_fight_nothing(function_index, thread_index, first);
}

void hs_evaluate_ai_try_to_fight_player(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiTargetingCommands::try_to_fight_player(function_index, thread_index, first);
}

void hs_evaluate_ai_attach(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::attach(function_index, thread_index, first);
}

void hs_evaluate_ai_attach_free(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::attach_free(function_index, thread_index, first);
}

void hs_evaluate_ai_attach_units(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::attach_units(function_index, thread_index, first);
}

void hs_evaluate_ai_detach(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::detach(function_index, thread_index, first);
}

void hs_evaluate_ai_detach_units(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::detach_units(function_index, thread_index, first);
}

void hs_evaluate_ai_erase(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::erase(function_index, thread_index, first);
}

void hs_evaluate_ai_erase_all(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::erase_all(function_index, thread_index, first);
}

void hs_evaluate_ai_free(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::run_free(function_index, thread_index, first);
}

void hs_evaluate_ai_free_units(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::free_units(function_index, thread_index, first);
}

void hs_evaluate_ai_kill(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::kill(function_index, thread_index, first);
}

void hs_evaluate_ai_kill_silent(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::kill_silent(function_index, thread_index, first);
}

void hs_evaluate_ai_migrate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::migrate(function_index, thread_index, first);
}

void hs_evaluate_ai_migrate_and_speak(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::migrate_and_speak(function_index, thread_index, first);
}

void hs_evaluate_ai_migrate_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::migrate_by_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_place(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::place(function_index, thread_index, first);
}

void hs_evaluate_ai_renew(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::renew(function_index, thread_index, first);
}

void hs_evaluate_ai_spawn_actor(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::spawn_actor(function_index, thread_index, first);
}

void hs_evaluate_ai_teleport_to_starting_location(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::teleport_to_starting_location(function_index, thread_index, first);
}

void hs_evaluate_ai_teleport_to_starting_location_if_unsupported(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiPlacementCommands::teleport_to_starting_location_if_unsupported(function_index, thread_index, first);
}

void hs_evaluate_ai_command_list(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiCommandListCommands::command_list(function_index, thread_index, first);
}

void hs_evaluate_ai_command_list_advance(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiCommandListCommands::command_list_advance(function_index, thread_index, first);
}

void hs_evaluate_ai_command_list_advance_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiCommandListCommands::command_list_advance_by_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_command_list_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiCommandListCommands::command_list_by_unit(function_index, thread_index, first);
}

void hs_evaluate_ai_command_list_status(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiCommandListCommands::command_list_status(function_index, thread_index, first);
}

void hs_evaluate_ai_conversation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiConversationCommands::conversation(function_index, thread_index, first);
}

void hs_evaluate_ai_conversation_advance(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiConversationCommands::conversation_advance(function_index, thread_index, first);
}

void hs_evaluate_ai_conversation_line(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiConversationCommands::conversation_line(function_index, thread_index, first);
}

void hs_evaluate_ai_conversation_status(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiConversationCommands::conversation_status(function_index, thread_index, first);
}

void hs_evaluate_ai_conversation_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiConversationCommands::conversation_stop(function_index, thread_index, first);
}

void hs_evaluate_ai_exit_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::exit_vehicle(function_index, thread_index, first);
}

void hs_evaluate_ai_go_to_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::go_to_vehicle(function_index, thread_index, first);
}

void hs_evaluate_ai_go_to_vehicle_override(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::go_to_vehicle_override(function_index, thread_index, first);
}

void hs_evaluate_ai_going_to_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::going_to_vehicle(function_index, thread_index, first);
}

void hs_evaluate_ai_vehicle_encounter(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::vehicle_encounter(function_index, thread_index, first);
}

void hs_evaluate_ai_vehicle_enterable_actor_type(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::vehicle_enterable_actor_type(function_index, thread_index, first);
}

void hs_evaluate_ai_vehicle_enterable_actors(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::vehicle_enterable_actors(function_index, thread_index, first);
}

void hs_evaluate_ai_vehicle_enterable_disable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::vehicle_enterable_disable(function_index, thread_index, first);
}

void hs_evaluate_ai_vehicle_enterable_distance(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::vehicle_enterable_distance(function_index, thread_index, first);
}

void hs_evaluate_ai_vehicle_enterable_team(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::AiVehicleCommands::vehicle_enterable_team(function_index, thread_index, first);
}

}
