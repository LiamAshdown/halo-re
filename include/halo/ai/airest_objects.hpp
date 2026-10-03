#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Non-owning handle to an actor in the actor data array.
 */
class AiActorView {
public:
    datum_index handle;
    explicit constexpr AiActorView(datum_index h) : handle(h) {}

    int32_t get_activity_stage();
    void link_to_unassigned_list();
    void unlink_from_unassigned_list();
    void get_move_speed_for_range(float param_a, float param_b, float param_dist, float *out_a, float *out_b);
};

/**
 * Operations tying AI actors to objects: attention records, pursuit records, nearby-actor processing
 * and object reference clean-up.
 */
class AiObjects {
public:
    static uint32_t type_get_morale_grade(int16_t actor_type_index, uint8_t *command_reference);
    static void clear_object_references(datum_index object_index);
    static ai_object_attention_record * object_attention_find_or_create(datum_index object_index);
    static void object_attention_remove(datum_index object_index);
    static void object_process_nearby_actors(uint32_t ai_reference, datum_index vehicle_index, char *seat_name, char allow_boarding_actors);
    static uint8_t pursuit_check_object(datum_index object_index, datum_index encounter_index, int16_t type, int32_t min_last_tick, char create_if_missing, int16_t *out_count, uint32_t *out_last_tick);
    static uint8_t pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type, int32_t min_last_tick);
    static void refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority, int16_t stimulus_value);
    static void create_actor(datum_index actor_variant_tag, datum_index unit_index);
    static void set_squad_reference(datum_index object_index, uint32_t packed_reference);
    static void add_component(datum_index component_index, uint32_t unit_index, datum_index swarm_index);
};

/**
 * Non-owning handle to an object list header. Members apply an AI operation to every unit in the list.
 */
class ObjectListView {
public:
    datum_index handle;
    explicit constexpr ObjectListView(datum_index h) : handle(h) {}

    void clear_orders_with_weapon();
    void detach_actors_from_encounters();
    void initialize_shield_stun_thresholds(float override_max_body_vitality, float override_max_shield_vitality);
    int16_t max_flee_grade();
    void remap_units_and_children(uint32_t packed_reference, char notify);
    void reset_or_wake_awareness(char flag);
    void respawn_members(uint32_t packed_reference);
    void set_unit_flag_400(char flag);
    void set_unit_flag_800(char flag);
    void set_unit_flag_800000(char flag);
    void spawn_members(uint32_t packed_reference);
    uint8_t start_user_animation_until_failure(datum_index graph_tag_id, const char *animation_name, uint8_t interpolate);
    void update_vitality_fractions(float body_delta, float shield_delta);
};

/**
 * Non-owning handle to a unit object that has (or may get) an AI actor.
 */
class AiUnitView {
public:
    datum_index handle;
    explicit constexpr AiUnitView(datum_index h) : handle(h) {}

    void clear_actor_vocalization();
    void dispatch_actor_event_d(int32_t unused);
    void flee_if_ready(uint32_t readiness_param);
    void remap_actor_to_squad(uint32_t packed_reference, char notify);
    void set_actor_unknown_0a(uint8_t value);
};

}
