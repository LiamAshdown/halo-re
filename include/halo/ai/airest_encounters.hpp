#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Interface of the platoon condition rules a scenario can attach to a platoon: each rule decides from the
 * platoon's average vitality, living count and member count whether the condition currently holds. Concrete
 * rules are stateless objects in static storage.
 */
class PlatoonCondition {
public:
    virtual bool test(float vitality, int16_t living, int16_t members) const = 0;

protected:
    ~PlatoonCondition() = default;
};

/**
 * Returns the rule for a scenario platoon condition code (1 to 9), or null when the code names no rule.
 */
const PlatoonCondition *platoon_condition(int32_t code);

/**
 * Non-owning handle to an encounter record in the encounter data array. It carries only the datum
 * index; every member is the former free function that took the encounter index first.
 */
class EncounterView {
public:
    datum_index handle;
    explicit constexpr EncounterView(datum_index h) : handle(h) {}

    uint8_t drift_zone_bias(int16_t squad_offset, float bias);
    int32_t record_recent_zone(int16_t zone_id);
    void stamp_team_from_unit(datum_index unit_index);
    void release_actors_filtered(int32_t platoon_index, int32_t squad_index, uint8_t is_dead);
    void starting_location_derive_placement_flags(int16_t starting_location_index, uint8_t *out_a, int16_t *out_b, uint8_t *out_c, int16_t *out_d, int16_t *out_edx, int16_t *out_esi);
    uint8_t activate();
    void advance_grenade_timers();
    void build_firing_position_claims(datum_index *out_claims);
    void choose_vocalizations();
    void deactivate();
    void decay_squad_spawn_delays();
    uint8_t evaluate_platoon_condition(const ai_platoon_condition *condition);
    void evaluate_support_needs(datum_index self_actor_index, int16_t mode, uint8_t phase, uint8_t *out_crowded, uint8_t *out_flanked, uint8_t *out_a, uint8_t *out_b, uint8_t *out_reachable_a, uint8_t *out_reachable_b, uint8_t *out_any);
    void gather_occupied_clusters(uint32_t *out_clusters, uint8_t record_per_actor, uint32_t *other_clusters);
    void process_squad_reinforcements();
    void propagate_platoon_state_to_actors();
    void recompute_morale();
    void redistribute_squads_toward_targets();
    void release_stale_props();
    void set_team(int16_t team);
    void spawn_squads(int16_t platoon_filter, int16_t squad_filter);
    void squad_clear_spawn_delay(int16_t squad_index);
    void squad_reset_starting_location_mask(int16_t squad_index);
    uint8_t squad_spawn_actor(int16_t squad_index, uint32_t unit_type_index, uint32_t unused);
    uint32_t squad_spawn_reinforcement(int16_t squad_index);
    void update_platoon_defending_flag();
    int16_t pick_random_starting_location(int16_t squad_index);
    datum_index recent_object_get_or_create(int16_t type, int32_t min_last_tick, char create_if_missing);
    void recent_object_list_clear();
};

/**
 * Encounter-system level operations that do not act on one encounter: initialisation, per-tick update,
 * squad reinforcement bookkeeping and name lookups against the scenario.
 */
class Encounters {
public:
    static void release_actors_and_swarms();
    static int32_t release_inactive_encounters(char *buffer, uint8_t *has_more, ai_release_state *state);
    static int release_inactive_swarms(char *buffer, uint8_t *has_more);
    static int32_t find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index, const Actor *requested_actor_data, const ActorVariant *requested_actor_variant_data, char match_by_index);
    static int __cdecl priority_compare(const ai_priority_target_record *record_a, const ai_priority_target_record *record_b);
    static int16_t resolve_actor_type(ScenarioSquad *squad);
    static void merge(uint32_t source_reference, uint32_t target_encounter_index, char notify, char is_platoon_merge);
    static void add_actor(int16_t squad_index, datum_index actor_index, datum_index encounter_index, uint8_t keep_team);
    static int32_t definition_find_platoon_index_by_name(ScenarioEncounter *encounter_definition, char *name);
    static int32_t definition_find_squad_index_by_name(ScenarioEncounter *encounter_definition, char *name);
    static void create(int16_t *squad_cursor, ScenarioEncounter *definition, int16_t *platoon_cursor);
    static void remove_actor(datum_index actor_index, uint8_t skip_counters);
    static void initialize();
    static void note_hostile_object(datum_index object_index);
    static void recompute_dirty();
    static void reset();
    static void spawn_initial();
    static void update();
    static void update_activation();
    static datum_index find_nearest_squad_member(datum_index actor_index, const actor_firing_positions *reference, datum_index exclude_index, char stamp_group);
    static int32_t find_encounter_index_by_name(Scenario *scenario, char *name);
};

}
