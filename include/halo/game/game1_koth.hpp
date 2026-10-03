#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Hill and ball helpers shared by the king-of-the-hill and oddball engines: hill boundary, occupancy and
 * marker geometry. Stateless service class: every function is a static member and the state it acts on lives
 * in the engine globals.
 */
class Koth {
public:
    static void animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player);
    static void alt_scorer_tick(uint32_t player_index);
    static void ball_idle_tick(uint32_t object_handle, object *obj);
    static void broadcast_hill_times(int32_t mode, int32_t machine_index);
    static void broadcast_team_scores(int32_t mode, int32_t machine_index);
    static void build_hill_boundary(void);
    static void build_hill_boundary_fence(void);
    static uint32_t dispatch_player_scoring(uint32_t player_index);
    static void find_marker_position(real_point3d *out_position, int16_t type_filter);
    static uint8_t player_eligible_to_score(uint32_t object_handle, uint32_t player_index);
    static uint8_t player_in_hill_bounds(uint32_t player_index);
    static void player_tick(uint32_t player_index);
    static void relocate_hill_marker(int32_t ball_index);
    static void relocate_object_hill(uint32_t object_index);
    static void reset_hill_marker_history(void);
    static void submit_hill_marker_geometry(uint32_t tag_handle_as_uint, uint32_t *position_override, uint32_t *orientation_override, uint32_t param_4, uint32_t param_5, float *vertex_source);
    static void update_hill_occupancy_state(void);
    static void update_occupant_table(uint32_t index);
};

}
