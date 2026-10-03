#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Non-owning handle for a packed AI reference (encounter, platoon range or squad selector as used by
 * scripts). Members resolve the reference and apply an operation to the actors or squads it names.
 */
class ReferenceView {
public:
    uint32_t handle;
    explicit constexpr ReferenceView(uint32_t h) : handle(h) {}

    void clear_unknown_00();
    uint8_t has_available();
    void set_unknown_00();
    void set_unknown_01();
    void set_unknown_02(char flag);
    void activate_squads();
    static void actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor);
    void actor_iterator_new(ai_reference_actor_iterator *out_iterator);
    static actor * actor_iterator_next(ai_reference_actor_iterator *iterator);
    datum_index build_object_list();
    void clear_search_target();
    void detach_actors_from_encounters();
    void expand_to_platoon_range(ai_reference_platoon_range *out_range);
    void face_starting_location(uint8_t idle_only);
    void flee_if_ready(uint32_t readiness_param);
    void for_each_squad();
    uint32_t get_stat_pair(int16_t stat_kind, int32_t *out_member_count, uint32_t *out_extra);
    void invoke_squad_callback_406f80();
    void mark_squads_unknown_11();
    int16_t max_activity_stage();
    void notify_actors(uint8_t flag);
    void notify_squad_index();
    static uint8_t parse(char *reference_string, Scenario *scenario, uint32_t *out_packed_reference);
    void refill_grenades();
    void reset_or_wake_awareness(char flag);
    int32_t resolve_squad_datum();
    void respawn_all_players();
    void respawn_member(datum_index unit_index);
    void respawn_placed_members(uint32_t respawn_reference);
    void set_combat_alert_flag(uint8_t new_flag);
    void set_search_target_area();
    void set_search_target_point(uint32_t reference_value);
    void set_squads_unknown_14(char flag);
    void set_unknown_1cb(char flag);
    static void spawn_starting_location_object(datum_index unit_index, uint32_t packed_reference);
    void squad_iterator_new(ai_reference_squad_iterator *out_iterator);
    static encounter_squad_state * squad_iterator_next(ai_reference_squad_iterator *iterator);
    void squad_set_unknown_10(uint8_t value);
    void units_exit_vehicles();
    void assign_team_and_request_order(int16_t value);
    void request_order(int16_t order_code);
};

}
