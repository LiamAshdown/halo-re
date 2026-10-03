/**
 * Hill and ball helpers shared by the king-of-the-hill and oddball engines: hill boundary, occupancy and
 * marker geometry.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "items.h"
#include <stdint.h>
#include "units.h"
#include "networking.h"
#include "cache.h"

#include "halo/game/game1_koth.hpp"
#include "halo/rasterizer/globals.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"

extern "C" {
extern uint8_t hill_pulse_fade_done;
extern uint8_t hill_pulse_grow_done;
extern data_array *player_data;
extern int16_t network_game_mode;
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag;
extern int32_t king_alt_player_score[];
extern int32_t king_alt_team_score[16];
extern int32_t king_alt_score_target;
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern void game_engine_begin_end_game_sequence(void);
extern data_array *object_data;
extern game_time_globals *game_time;
extern game_variant game_engine_variant;
extern uint32_t king_hill_occupant_table[16];
extern int32_t king_hill_occupant_last_tick[16];
extern int32_t king_hill_idle_timeout;
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position,
    float height_offset, datum_index player_filter, int16_t team_filter);
extern void game_engine_koth_relocate_object_hill(uint32_t object_index);
extern uint8_t weapon_must_be_readied(void);
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast);
extern uint8_t shared_hud_text_draw_state;
extern int32_t king_team_hill_seconds_network[16];
extern int32_t king_bucket_credit_ticks[16];
extern int32_t king_hill_broadcast_overrun_value;
extern network_server_globals *network_server;
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6);
extern int32_t king_alt_team_scores_network[16];
extern int32_t king_alt_player_scores_network[16];
extern int32_t king_alt_team_scores_network2[16];
extern int32_t king_alt_scores_network_tail[16];
extern Scenario *global_scenario;
extern int32_t king_starting_location_type;
extern int32_t king_starting_location_count;
extern real_point3d king_hill_boundary_points[12];
extern uint32_t king_hill_boundary_extra[12][2];
extern float king_hill_boundary_min_z;
extern float king_hill_boundary_max_z;
extern real_point3d king_hill_boundary_center;
extern int32_t game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results);
extern void point3d_array_project_to_xy_plane(real_point3d *source, Point2D *destination,
    int32_t count);
extern Globals *global_globals;
extern double sqrt(double x);
extern double fabs(double x);
extern double floor(double x);
extern void game_engine_koth_submit_hill_marker_geometry(uint32_t tag_handle_as_uint,
    uint32_t *position_override, uint32_t *orientation_override, uint32_t param_4,
    uint32_t param_5, float *vertex_source);
extern game_engine_state game_engine_state_value;
extern void unit_reset_gauge_if_flagged(void);
extern void game_engine_koth_alt_scorer_tick(uint32_t player_index);
extern void game_engine_koth_update_occupant_table(uint32_t index);
extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player,
    int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast);
extern uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit);
extern uint8_t king_hill_player_in_hill[16];
extern int32_t king_bucket_last_credit_tick[16];
extern uint8_t game_engine_koth_player_in_hill_bounds(uint32_t player_index);
extern uint8_t game_engine_get_teams_enabled(void);
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role);
extern datum_index object_new(object_placement_data *placement);
extern void object_mark_pending_delete(uint32_t object_index);
extern void game_engine_koth_find_marker_position(real_point3d *out_position, int16_t type_filter);
extern void ctf_flag_object_clear_carrier(datum_index flag_object_index, real_point3d *position);
extern const real_vector3d *global_white_color;
extern king_hill_marker_history king_hill_markers;
extern int16_t rasterizer_vertex_buffer_lock_state;
extern void **rasterizer_dynamic_index_buffer;
extern int16_t rasterizer_dynamic_vertex_slots[];
extern int32_t render_unknown_d98f0[];
extern uint8_t render_unknown_7bf04c[];
extern void *k_render_identity_matrix_ptr;
extern const ColorARGB *global_white_argb;
extern real_vector3d default_axis_b;
extern uint8_t rasterizer_render_states_dirty;
extern uint32_t rasterizer_device_version;
extern void **rasterizer_device;
extern void *rasterizer_dynamic_index_cache_reserve(void);
extern int32_t rasterizer_dynamic_vertex_cache_reserve(void);
extern int32_t rasterizer_dynamic_vertex_cache_lock(void);
extern void *rasterizer_dynamic_index_slot_lock(void);
extern void rasterizer_model_draw_prepare_states(int32_t a);
extern void rasterizer_shader_environment_draw_dispatch(int32_t tag_data, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e);
extern void rasterizer_transparent_geometry_group_build(int32_t tag_data, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e,
    int32_t f, void *g);
extern void rasterizer_model_draw_restore_states(void);
extern uint8_t king_hill_single_occupant_flag;
extern king_globals king_hill_state_globals;
}

namespace halo::game::engine1 {

/**
 * Animates a pair of HUD icon scale/alpha values, one fading down and one growing up over time, consistent
 * with a hill-control pulse indicator.
 *
 * Original register convention: EAX -> fading_player, ECX -> growing_player.
 *
 * @address 0x46f450
 */
void Koth::animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player)
{
    player *fading = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)fading_player * sizeof(player));
    player *growing = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)growing_player * sizeof(player));

    if (!hill_pulse_fade_done) {
        float speed = fading->speed - 0.02f;

        fading->speed = speed;
        if (speed >= 1.0f) {
            speed = speed - 0.15f;
            fading->speed = speed;
            if (speed <= 1.0f) {
                speed = 1.0f;
            }
            fading->speed = speed;
        }
        fading->speed = (fading->speed <= 0.9f) ? 0.9f : fading->speed;
    }

    if (!hill_pulse_grow_done) {
        float speed = growing->speed + 0.1f;

        growing->speed = speed;
        if (speed <= 1.0f) {
            speed = speed + 0.1f;
            growing->speed = speed;
            if (speed > 1.0f) {
                speed = 1.0f;
            }
            growing->speed = speed;
        }
        if (growing->speed > 1.5f) {
            growing->speed = 1.5f;
        }
    }
}

/**
 * Alternate per-tick King-of-the-Hill scorer that increments per-player and per-team hill-occupancy counters
 * against a fixed target and triggers the end-of-game sequence once that target is met.
 *
 * Original register convention: EAX -> player_index.
 *
 * @address 0x46c230
 */
void Koth::alt_scorer_tick(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (network_game_mode == 2) {
        king_alt_player_score[player_index & 0xffff]++;
        king_alt_team_score[p->team]++;
        if (king_alt_score_target - king_alt_team_score[p->team] == 900) {
            game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0
                ? 5 + 2 * (p->team != 0) : 3, 0xffffffff, 1);
        }
        if (king_alt_score_target - king_alt_team_score[p->team] == 0x708) {
            game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0
                ? 4 + 2 * (p->team != 0) : 2, 0xffffffff, 1);
        }
    }

    if (king_alt_team_score[p->team] < king_alt_score_target) {
        return;
    }
    game_engine_begin_end_game_sequence();
}

/**
 * Checks whether a hill has gone unclaimed for too long and, if so, announces it via the kill feed and
 * relocates it; otherwise records the current tick as the hill's last-active time.
 *
 * @address 0x46c5d0
 */
void Koth::ball_idle_tick(uint32_t object_handle, object *obj)
{
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    real_point3d position;
    object_header *hdr;
    int32_t tick;

    if (halo::items::item_get_effective_position((datum_index)object_handle, &position) != 1) {
        return;
    }

    hdr = (object_header *)object_data->data + (object_handle & 0xffff);
    if ((hdr->flags & 0x08) != 0) {
        return;
    }

    custom_waypoint_register((datum_index)0xffffffff, (int16_t)0, &position, 0.0f,
        (datum_index)0xffffffff, (int16_t)0xffffffff);

    tick = game_time->game_time;
    if ((uint32_t)(game_time->game_time - item->held_game_time) > 0x4b0) {
        if (network_game_mode != 2) {
            return;
        }
        if (halo::items::weapon_must_be_readied((datum_index)object_handle) == 0 || (obj->flags >> 0xb & 1) == 0 || obj->parent_object != (datum_index)0xffffffff) {
            goto check_relocation;
        }
        if ((*(uint8_t *)((uint8_t *)obj + 0x22c) & 0x40) != 0) {
            data_iterator iter;
            void *element;
            iter.data = object_data;
            iter.next_index = 0;
            iter.index = (datum_index)0xffffffff;
            iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
            element = halo::memory::data_iterator_next(&iter);
            while (element != 0) {
                chimera__kill_feed((datum_index)0xffffffff, 0x26, (uint32_t)0xffffffff, 1, 0);
                element = halo::memory::data_iterator_next(&iter);
            }
        }
        game_engine_koth_relocate_object_hill(object_handle);
    }
    tick = game_time->game_time;
    if (network_game_mode != 2) {
        return;
    }
check_relocation:
    if (game_engine_variant.engine.oddball.ball_type < 1 || game_engine_variant.engine.oddball.ball_type > 2) {
        int16_t team = ((object *)obj)->owner_team;
        if (king_hill_occupant_last_tick[team] == -1 ||
            king_hill_occupant_last_tick[team] + king_hill_idle_timeout < tick) {
            if (obj->parent_object == (datum_index)0xffffffff && (obj->flags & _object_at_rest_bit) != 0) {
                obj->flags |= _object_changed_bit;
            }
            king_hill_occupant_last_tick[team] = tick;
        }
    }
}

/**
 * Requests or (when mode!=0) formats and broadcasts a network HUD message reporting per-team hill-occupation
 * time in seconds.
 *
 * @address 0x46b7f0
 */
void Koth::broadcast_hill_times(int32_t mode, int32_t machine_index)
{
    int32_t encoded_bits;

    if (mode == 0) {
        void *field = &king_team_hill_seconds_network[0];
        void *no_extra = (void *)0;
        (void)no_extra;
        encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x13, 0, &field, 0, 1, 0);
    } else {
        int32_t seconds[107];
        int32_t i;

        for (i = 0; i < 107; i++) {
            seconds[i] = ((int32_t *)king_bucket_credit_ticks)[i];
        }
        for (i = 0; i < 16; i++) {
            seconds[i] = seconds[i] / 30;
        }

        {
            void *seconds_field = seconds;
            void *count_field = &king_team_hill_seconds_network[0];

            int32_t zero_extra = 0;
            (void)count_field;
            (void)zero_extra;
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x13, 0, (void **)&seconds_field, (uint32_t)&king_team_hill_seconds_network[0], 1, 0);
        }

        for (i = 0; i < 16; i++) {
            king_team_hill_seconds_network[i] = seconds[i];
        }
        king_hill_broadcast_overrun_value = seconds[106];
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
        }
    }
}

/**
 * Requests or broadcasts the King-of-the-Hill variant's team score/target state over the network, converting
 * tick counts to seconds where needed.
 *
 * @address 0x46d060
 */
void Koth::broadcast_team_scores(int32_t mode, int32_t machine_index)
{
    int32_t encoded_bits;

    if (mode == 0) {
        void *field = &king_alt_team_scores_network[0];
        encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x12, 0, &field, 0, 1, 0);
    } else {
        int32_t target_and_team[17];
        int32_t player_scores[16];
        int32_t i;

        target_and_team[0] = king_alt_score_target;
        for (i = 0; i < 16; i++) {
            target_and_team[i + 1] = king_alt_team_score[i];
            player_scores[i] = ((int32_t *)king_alt_player_score)[i];
        }

        if (game_engine_variant.engine.oddball.ball_type != 2) {
            for (i = 0; i < 16; i++) {
                target_and_team[i + 1] /= 30;
                player_scores[i] /= 30;
            }
        }

        {
            void *fields0 = target_and_team;
            void *fields1 = &king_alt_team_scores_network[0];
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x12, 0, (void **)&fields0,
                (uint32_t)&fields1, 1, 0);
        }

        for (i = 0; i < 16; i++) {
            king_alt_player_scores_network[i] = player_scores[i];
        }
        for (i = 0; i < 16; i++) {
            king_alt_team_scores_network2[i] = target_and_team[i + 1];
        }
        for (i = 0; i < 16; i++) {
            king_alt_scores_network_tail[i] = ((int32_t *)king_alt_player_score)[32 + i];
        }
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
        }
    }
}

/**
 * Builds a 2D convex-hull boundary around a set of valid starting locations and computes its bounding-box
 * center and vertical extent.
 *
 * @address 0x46a240
 */
void Koth::build_hill_boundary(void)
{
    real_point3d points[12];
    int32_t indices[12];
    Point2D hull_points[12];
    uint32_t extra[12][2];
    int32_t count;
    int16_t hull_count;
    int32_t i;

    count = game_engine_find_valid_starting_locations(
        (real_point3d *)0, 0.0f, 0.0f, 8, (int16_t)king_starting_location_type, 0xc, indices);
    king_starting_location_count = count;
    if (count == 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        ScenarioNetgameFlags *loc =
            &((ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer)[indices[i]];
        points[i].x = loc->position.x;
        points[i].y = loc->position.y;
        points[i].z = loc->position.z;
    }

    if (count == 1) {
        points[1].x = points[0].x + 1.0f;
        points[1].y = points[0].y - 1.0f;
        points[1].z = points[0].z;
        points[2].x = points[0].x + 1.0f;
        points[2].y = points[0].y + 1.0f;
        points[2].z = points[0].z;
        points[3].x = points[0].x - 1.0f;
        points[3].y = points[0].y + 1.0f;
        points[3].z = points[0].z;
        points[0].y = points[0].y - 1.0f;
        points[0].x = points[0].x - 1.0f;
        count = 4;
    }

    point3d_array_project_to_xy_plane(points, hull_points, count);
    hull_count = halo::math::polygon2d_convex_hull_build(reinterpret_cast<real_point2d *>(hull_points), count, reinterpret_cast<int16_t *>(hull_points));
    king_starting_location_count = hull_count;

    if (hull_count > 0) {
        for (i = 0; i < hull_count; i++) {
            int16_t src = ((int16_t *)hull_points)[i];
            king_hill_boundary_points[i] = points[src];
            extra[i][0] = ((uint32_t *)hull_points)[src * 2];
            extra[i][1] = ((uint32_t *)hull_points)[src * 2 + 1];
        }
        for (i = 0; i < hull_count; i++) {
            king_hill_boundary_extra[i][0] = extra[i][0];
            king_hill_boundary_extra[i][1] = extra[i][1];
        }
    }

    king_hill_boundary_center.x = king_hill_boundary_points[0].x;
    king_hill_boundary_center.y = king_hill_boundary_points[0].y;
    king_hill_boundary_center.z = king_hill_boundary_points[0].z;
    {
        real_point3d minv = king_hill_boundary_points[0];
        real_point3d maxv = king_hill_boundary_points[0];

        for (i = 0; i < hull_count; i++) {
            real_point3d *p = &king_hill_boundary_points[i];
            if (p->x < minv.x) minv.x = p->x;
            if (p->y < minv.y) minv.y = p->y;
            if (p->z < minv.z) minv.z = p->z;
            if (p->x >= maxv.x) maxv.x = p->x;
            if (p->y >= maxv.y) maxv.y = p->y;
            if (p->z >= maxv.z) maxv.z = p->z;
        }

        king_hill_boundary_min_z = minv.z - 0.1f;
        king_hill_boundary_max_z = maxv.z + 0.8f;
        king_hill_boundary_center.x = (maxv.x + minv.x) * 0.5f;
        king_hill_boundary_center.y = (maxv.y + minv.y) * 0.5f;
        king_hill_boundary_center.z = (maxv.z + minv.z) * 0.5f;
    }
}

/**
 * Generates and submits a fence-like quad mesh (with per-edge normals and UVs) along the starting-location
 * boundary polygon.
 *
 * @address 0x46a670
 */
void Koth::build_hill_boundary_fence(void)
{
    uint32_t hill_shader_tag;
    int32_t count = king_starting_location_count;
    float total_length = 0.0f;
    double length_period;
    float running_length = 0.0f;
    float previous_u = 0.0f;
    int32_t i;

    hill_shader_tag = *(uint32_t *)((uint8_t *)global_globals->multiplayer_information.pointer + 0x38);

    if (count > 0) {
        for (i = 0; i < count; i++) {
            real_point3d *cur = &king_hill_boundary_points[i];
            real_point3d *nxt = &king_hill_boundary_points[(i + 1 == count) ? 0 : i + 1];
            float dx = nxt->x - cur->x, dy = nxt->y - cur->y, dz = nxt->z - cur->z;
            total_length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) + total_length;
        }
    }

    length_period = floor((double)(total_length + 0.5f));

    if (count > 0) {
        float inv_scale = (float)(1.0 / length_period);

        for (i = 0; i < count; i++) {
            real_point3d *cur = &king_hill_boundary_points[i];
            real_point3d *nxt = &king_hill_boundary_points[(i + 1 == count) ? 0 : i + 1];
            float z_a = cur->z + 0.8f;
            float z_b = nxt->z + 0.8f;
            float nx, ny, nz, nlen;
            float u_end;
            koth_fence_corner quad[4];
            int32_t c;

            for (c = 0; c < 4; c++) {
                int32_t j;
                for (j = 0; j < 0x11; j++) {
                    ((float *)&quad[c])[j] = 0.0f;
                }
            }

            running_length = (float)sqrt((double)((nxt->x - cur->x) * (nxt->x - cur->x) +
                                    (nxt->y - cur->y) * (nxt->y - cur->y) +
                                    (nxt->z - cur->z) * (nxt->z - cur->z))) + running_length;
            u_end = (float)(length_period / (double)total_length) * running_length;

            nx = -(nxt->y - cur->y) * (z_a - cur->z);
            ny = (nxt->x - cur->x) * (z_a - cur->z);
            nz = 0.0f;
            nlen = (float)sqrt((double)(nx * nx + ny * ny + nz * nz));
            if (fabs((double)nlen) >= 0.0001) {
                float inv = 1.0f / nlen;
                nx *= inv; ny *= inv; nz *= inv;
            }

            quad[0].position[0] = cur->x; quad[0].position[1] = cur->y; quad[0].position[2] = cur->z;
            quad[1].position[0] = cur->x; quad[1].position[1] = cur->y; quad[1].position[2] = z_a;
            quad[2].position[0] = nxt->x; quad[2].position[1] = nxt->y; quad[2].position[2] = z_b;
            quad[3].position[0] = nxt->x; quad[3].position[1] = nxt->y; quad[3].position[2] = nxt->z;

            for (c = 0; c < 4; c++) {
                quad[c].normal[0] = nx; quad[c].normal[1] = ny; quad[c].normal[2] = nz;
            }

            quad[0].u = previous_u * inv_scale; quad[0].v = 1.0f;
            quad[1].u = previous_u * inv_scale; quad[1].v = 0.2f;
            quad[2].u = u_end * inv_scale;      quad[2].v = 0.2f;
            quad[3].u = u_end * inv_scale;      quad[3].v = 1.0f;

            {
                float length_period_f = (float)length_period;
                game_engine_koth_submit_hill_marker_geometry(hill_shader_tag, (uint32_t *)0, (uint32_t *)0,
                    *(uint32_t *)&length_period_f, 0x3f800000, (float *)quad);
            }

            previous_u = u_end;
        }
    }
}

/**
 * Dispatches the King-of-the-Hill per-tick scoring for a player across however many hill slots they occupy,
 * applying a configurable score multiplier and periodic sound cues.
 *
 * @address 0x46c3e0
 */
uint32_t Koth::dispatch_player_scoring(uint32_t player_index)
{
    uint32_t idx = player_index & 0xffff;
    player *p = (player *)((uint8_t *)player_data->data + idx * sizeof(player));
    int32_t occupied_slots;
    int32_t i;
    uint32_t result = 0;

    p->hud_message_index = (datum_index)0xffffffff;
    p->hud_message_player = (datum_index)0xffffffff;

    game_engine_koth_update_occupant_table(player_index);

    occupied_slots = 0;
    if (game_engine_variant.engine.oddball.ball_count > 0) {
        for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
            if (king_hill_occupant_table[i] == player_index) {
                occupied_slots++;
            }
        }
    }
    result = (uint32_t)occupied_slots;

    p->speed = 1.0f;
    if (occupied_slots > 0) {
        if (game_engine_variant.engine.oddball.trait_with_ball != 1) {
            unit_reset_gauge_if_flagged();
        }
        if (game_engine_variant.engine.oddball.speed_with_ball == 1) {
            p->speed = 1.0f;
        } else if (game_engine_variant.engine.oddball.speed_with_ball == 2) {
            p->speed = 1.25f;
        } else {
            p->speed = 0.75f;
        }
    }

    if ((current_game_engine == 0 || game_engine_state_value == 0) &&
        game_engine_variant.engine.oddball.ball_type != 2 && occupied_slots > 0) {
        int32_t remaining = occupied_slots;
        do {
            if (game_engine_variant.engine.oddball.ball_type == 0) {
                p->hud_message_index = (datum_index)0x29;
                p->hud_message_player = (datum_index)player_index;
            }
            game_engine_koth_alt_scorer_tick(player_index);
            remaining--;
        } while (remaining != 0);
    }

    if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type < 3 && occupied_slots > 0) {
        p->hud_message_index = (datum_index)0x23;
        p->hud_message_player = (datum_index)player_index;
    }

    if (p->unit != (datum_index)0xffffffff) {
        unit_data *unit = (unit_data *)((uint8_t *)
            ((object_header *)object_data->data)[(uint32_t)p->unit & 0xffff].data +
            k_unit_data_offset);
        if (unit->current_weapon_index != -1) {
            datum_index weapon = unit->weapons[unit->current_weapon_index];
            if (weapon != (datum_index)0xffffffff) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon & 0xffff].data;
                uint32_t *tag_data = (uint32_t *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & 0xffff].data;
                if ((*(uint32_t *)((uint8_t *)tag_data + 0x308) >> 3 & 1) != 0) {
                    int32_t score = king_alt_player_score[idx];
                    if (score > 0 && score % 0x96 == 0 && score < king_alt_score_target) {
                        game_engine_queue_multiplayer_sound(0x2a, 0xffffffff, 0);
                    }

                    *(int16_t *)((uint8_t *)weapon_obj + 0x2b8) = (int16_t)(score / 30);
                }
            }
        }
    }

    return result;
}

/**
 * Finds a valid (or, failing that, a random) type-2 scenario starting location and returns its position, used
 * to place the King-of-the-Hill marker.
 *
 * Original register convention: stack -> out_position, CX -> type_filter.
 *
 * @address 0x46beb0
 */
void Koth::find_marker_position(real_point3d *out_position, int16_t type_filter)
{
    int32_t index = -1;

    real_point3d found;

    if (game_engine_variant.engine.oddball.random_start == 0) {
        game_engine_find_valid_starting_locations((real_point3d *)0, 0.0f, 0.0f, 2, type_filter, 1, &index);
    }

    if (index == -1) {
        int32_t flag_count = (int32_t)global_scenario->netgame_flags.count;
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
        int32_t matching = 0;
        int32_t i;

        for (i = 0; i < flag_count; i++) {
            if (flags[i].type == 2) {
                matching++;
            }
        }

        if (matching != 0) {
            int32_t pick;
            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
            pick = (int16_t)(((halo::math::globals().random_seed_global >> 0x10) *
                              (uint32_t)(int32_t)(int16_t)matching) >> 0x10);

            for (i = 0; i < flag_count; i++) {
                if (flags[i].type == 2) {
                    if (pick == 0) {
                        index = i;
                        break;
                    }
                    pick--;
                }
            }
        }
    }

    if (index != -1) {
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;

        found.x = flags[index].position.x;
        found.y = flags[index].position.y;
        found.z = flags[index].position.z;
    }
    out_position->x = found.x;
    out_position->y = found.y;
    out_position->z = found.z;
}

/**
 * Determines whether a player is currently eligible to score hill time, using different rules for team versus
 * non-team hill configurations.
 *
 * @address 0x46ce10
 */
uint8_t Koth::player_eligible_to_score(uint32_t object_handle, uint32_t player_index)
{
    object *obj = ((object_header *)object_data->data)[object_handle & 0xffff].data;

    if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type < 3) {
        game_engine_broadcast_kill_feed_by_relationship(player_index, 0x20, 0x21, 0x22, player_index, 0);
        return 1;
    }

    {
        player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
        uint8_t eligible = 1;
        if (p->unit != (datum_index)0xffffffff) {
            uint16_t found = unit_find_weapon_index_by_flag((uint32_t)p->unit, 3);
            eligible = 1 - (found != 0);
            if (eligible != 0) {
                *(uint32_t *)((uint8_t *)obj + 0x22c) |= 0x40;
            }
        }
        return eligible;
    }
}

/**
 * Returns whether the unit of the player is inside the current hill bounds.
 *
 * Original register convention: EAX -> player_index.
 *
 * @address 0x46aa60
 */
uint8_t Koth::player_in_hill_bounds(uint32_t player_index)
{
    player *p;
    datum_index unit;
    object *unit_obj;
    float z;

    if (player_index == 0xffffffff) {
        return 0;
    }
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit = p->unit;
    if (unit == (datum_index)0xffffffff) {
        return 0;
    }
    unit_obj = ((object_header *)object_data->data)[unit & 0xffff].data;
    z = unit_obj->bounding_center.z;
    if (z >= king_hill_boundary_min_z && z < king_hill_boundary_max_z) {
        real_point2d point;
        point.x = unit_obj->bounding_center.x;
        point.y = unit_obj->bounding_center.y;
        return halo::math::polygon2d_point_inside_margin((real_point2d *)king_hill_boundary_extra, (int16_t)king_starting_location_count, point, 0.0f);
    }
    return 0;
}

/**
 * Per-tick handler for a King-of-the-Hill player: if the player is validly standing in the hill and the game
 * is not already decided, credits hill time to their team, fires countdown/warning sound cues...
 *
 * @address 0x46ab00
 */
void Koth::player_tick(uint32_t player_index)
{
    uint32_t idx = player_index & 0xffff;
    player *p = (player *)((uint8_t *)player_data->data + idx * sizeof(player));

    *(uint32_t *)&((struct player *)p)->hud_message_index = 0xffffffff;
    *(uint32_t *)&((struct player *)p)->hud_message_player = 0xffffffff;
    king_hill_player_in_hill[idx] = 0;

    if (p->unit != (datum_index)0xffffffff &&
        (current_game_engine == 0 || game_engine_state_value == 0) &&
        game_engine_koth_player_in_hill_bounds(player_index) != 0) {
        uint8_t hosting = (network_game_mode == 2);

        king_hill_player_in_hill[idx] = 1;
        if (hosting) {
            *(int16_t *)&((struct player *)p)->objective_time += 1;
        }

        if (king_bucket_last_credit_tick[p->team] < game_time->game_time && network_game_mode == 2) {
            int32_t limit_ticks = game_engine_variant.score_limit * 0x708;
            int32_t bucket;

            king_bucket_credit_ticks[p->team]++;
            king_bucket_last_credit_tick[p->team] = game_time->game_time;
            bucket = king_bucket_credit_ticks[p->team];

            if (limit_ticks - bucket == 900) {
                game_engine_queue_multiplayer_sound(game_engine_get_teams_enabled() != 0
                    ? 5 + 2 * (p->team != 0) : 3, 0xffffffff, 1);
            }
            if (limit_ticks - bucket == 0x708) {
                game_engine_queue_multiplayer_sound(game_engine_get_teams_enabled() != 0
                    ? 4 + 2 * (p->team != 0) : 2, 0xffffffff, 1);
            }
            bucket = king_bucket_credit_ticks[p->team];
            if (bucket > 0 && bucket % 0x96 == 0 && bucket < limit_ticks) {
                game_engine_queue_multiplayer_sound(0x2a, player_index, 1);
            }
            if (limit_ticks <= king_bucket_credit_ticks[p->team]) {
                game_engine_begin_end_game_sequence();
            }
        }

        *(uint32_t *)&((struct player *)p)->hud_message_index = 0x22;
        *(uint32_t *)&((struct player *)p)->hud_message_player = player_index;
    }
}

/**
 * Relocates the moving King-of-the-Hill marker: detaches the current hill object, picks a new valid random
 * position, and (re)spawns the marker there.
 *
 * Original register convention: ESI -> ball_index.
 *
 * @address 0x46bfe0
 */
void Koth::relocate_hill_marker(int32_t ball_index)
{
    if (game_engine_variant.engine.oddball.ball_type < 1 || game_engine_variant.engine.oddball.ball_type > 2) {
        GlobalsMultiplayerInformation *mp_info =
            (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
        uint32_t ball_tag = (uint32_t)((mp_info->ball.tag_id.id << 16) | mp_info->ball.tag_id.index);

        if ((int32_t)ball_tag != -1) {
            object_placement_data placement;
            datum_index new_object;
            object_header *hdr;
            uint8_t header_flags;

            object_placement_data_initialize(&placement, ball_tag, (datum_index)0xffffffff);
            placement.owner_team = (int16_t)ball_index;
            game_engine_koth_find_marker_position(&placement.position, (int16_t)ball_index);

            new_object = object_new(&placement);

            hdr = (object_header *)object_data->data + ((uint32_t)new_object & 0xffff);
            header_flags = hdr->flags;
            hdr->flags = header_flags & ~_object_header_in_pvs_pass_bit;
            if ((header_flags & _object_header_active_bit) == 0) {
                object_mark_pending_delete((uint32_t)new_object);
            }
        }
    }
}

/**
 * In team play, finds a new hill location for the given object index, plays a related sound when few hills
 * have been used, and clears the object's 'needs relocation' flag.
 *
 * Original register convention: EAX -> object_index.
 *
 * @address 0x46c1a0
 */
void Koth::relocate_object_hill(uint32_t object_index)
{
    if (network_game_mode == 2) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        real_point3d discarded_position;

        game_engine_koth_find_marker_position(&discarded_position, ((object *)obj)->owner_team);

        if (game_engine_variant.engine.oddball.ball_count < 3) {
            game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);
        }
        ctf_flag_object_clear_carrier(object_index, &discarded_position);
        *(uint32_t *)((uint8_t *)obj + 0x22c) &= 0xffffffbf;
    }
}

/**
 * Seeding all four tracked slots with the current hill location.
 *
 * @address 0x46b250
 */
void Koth::reset_hill_marker_history(void)
{
    int32_t i;
    for (i = 0; i < 4; i++) {
        king_hill_markers.position[i].x = global_white_color[0].i;
        king_hill_markers.position[i].y = global_white_color[0].j;
        king_hill_markers.position[i].z = global_white_color[0].k;
        king_hill_markers.state[i] = 0;
    }
}

/**
 * Assembles and submits a small render/decal geometry batch (positions, colors, default hill-marker placement)
 * to draw the moving King-of-the-Hill marker.
 *
 * Original register convention: stack -> tag_handle_as_uint, position_override, orientation_override, param_4,
 * param_5;.
 *
 * @address 0x46b2f0
 */
void Koth::submit_hill_marker_geometry(uint32_t tag_handle_as_uint, uint32_t *position_override, uint32_t *orientation_override, uint32_t param_4, uint32_t param_5, float *vertex_source)
{
    float fVar5;
    int32_t iVar6;
    float local_f0;
    float fStack_ec;
    float fStack_e8;
    int32_t iStack_fc = 0;

    rasterizer_vertex_buffer_lock_state = 9;
    fVar5 = *(float *)rasterizer_dynamic_index_cache_reserve();
    local_f0 = fVar5;
    iVar6 = rasterizer_dynamic_vertex_cache_reserve();
    if (fVar5 == fVar5 && iVar6 != -1) {
        uint8_t *dest_block;
        int32_t base;
        int32_t offset;
        float *src;
        uint32_t *dst;
        int32_t count;
        int32_t tag_data;
        int32_t i;

        base = rasterizer_dynamic_vertex_cache_lock();
        dest_block = (uint8_t *)rasterizer_dynamic_index_slot_lock();
        offset = (int32_t)vertex_source - base;
        src = vertex_source + 9;
        dst = (uint32_t *)(base + 0xc);
        count = 4;
        do {
            dst[-3] = ((uint32_t *)src)[-9];
            dst[-2] = ((uint32_t *)src)[-8];
            dst[-1] = ((uint32_t *)src)[-7];
            {
                uint32_t *from = (uint32_t *)(offset + (int32_t)dst);
                dst[0] = from[0];
                dst[1] = from[1];
                dst[2] = from[2];
            }
            dst[3] = ((uint32_t *)src)[-3];
            dst[4] = ((uint32_t *)src)[-2];
            dst[5] = ((uint32_t *)src)[-1];
            dst[6] = ((uint32_t *)src)[0];
            dst[7] = ((uint32_t *)src)[1];
            dst[8] = ((uint32_t *)src)[2];
            dst[9] = ((uint32_t *)src)[3];
            dst[10] = ((uint32_t *)src)[4];
            *(uint16_t *)(dst + 0xb) = *(uint16_t *)(src + 5);
            *(uint16_t *)((uint8_t *)dst + 0x2e) = *(uint16_t *)((uint8_t *)src + 0x16);
            dst[0xc] = 0x3f000000;
            dst[0xd] = 0x3f000000;
            src = src + 0x11;
            dst = dst + 0x11;
            count--;
        } while (count != 0);

        *(uint16_t *)dest_block = 0;
        *(uint16_t *)(dest_block + 2) = 1;
        *(uint16_t *)(dest_block + 4) = 2;
        *(uint16_t *)(dest_block + 6) = 2;
        *(uint16_t *)(dest_block + 8) = 3;
        *(uint16_t *)(dest_block + 10) = 0;

        ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*rasterizer_dynamic_index_buffer + 0x30)))(rasterizer_dynamic_index_buffer);

        {
            int16_t sub_index = rasterizer_dynamic_vertex_slots[iStack_fc * 8];
            if (render_unknown_d98f0[sub_index * 3] != 0) {
                void **sub = (void **)(render_unknown_7bf04c + render_unknown_d98f0[sub_index * 3] * 10);
                ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*sub + 0x30)))(sub);
            }
        }

        tag_data = *(int32_t *)((uint8_t *)halo::cache::globals().tag_instances[tag_handle_as_uint & 0xffff].data + 0);
        local_f0 = (vertex_source[0x33] + vertex_source[0x22] + vertex_source[0x11] + vertex_source[0]) * 0.25f;
        fStack_ec = (vertex_source[0x34] + vertex_source[0x23] + vertex_source[0x12] + vertex_source[1]) * 0.25f;
        {
            float fVar1 = vertex_source[0x35];
            float fVar2 = vertex_source[0x24];
            float fVar3 = vertex_source[0x13];
            float fVar4 = vertex_source[2];
            fStack_e8 = (fVar1 + fVar2 + fVar3 + fVar4) * 0.25f;
        }

        {
            uint8_t record[0xd8];
            for (i = 0; i < (int32_t)sizeof(record); i++) record[i] = 0;

            *(uint32_t *)(record + 4) = 1;
            *(uint16_t *)(record + 8) = 1;
            *(void **)(record + 0xa) = k_render_identity_matrix_ptr;

            if (position_override == (uint32_t *)0) {
                *(real_vector3d *)(record + 0xe) = *global_white_color;
                *(uint16_t *)(record + 0x1a) = 0;
                *(uint16_t *)(record + 0x50) = 0;
                for (i = 0; i < 16; i++) {
                    (record + 0x54)[i] = ((const uint8_t *)global_white_argb)[i];
                }
                *(real_vector3d *)(record + 0x64) = default_axis_b;
                *(uint32_t *)(record + 0x70) = 0;
                *(uint32_t *)(record + 0x74) = 0x3f800000;
                *(uint32_t *)(record + 0x78) = 0;
            } else {
                uint32_t *dstp = (uint32_t *)(record + 0xe);
                uint32_t *srcp = position_override;
                for (i = 0; i < 0x1d; i++) {
                    dstp[i] = srcp[i];
                }
            }

            if (orientation_override == (uint32_t *)0) {
                *(void **)(record + 0xc8) = &king_hill_markers.position[0];
                *(void **)(record + 0xcc) = &king_hill_markers.state[0];
            } else {
                *(void **)(record + 0xc8) = (void *)orientation_override[0];
                *(void **)(record + 0xcc) = (void *)orientation_override[1];
            }
            *(uint32_t *)(record + 0xd0) = param_4;
            *(uint32_t *)(record + 0xd4) = param_5;
            *(float *)(record + 0x98) = local_f0;
            *(float *)(record + 0x9c) = fStack_ec;
            *(float *)(record + 0xa0) = fStack_e8;

            if (halo::rasterizer::globals::models_enabled != 0) {
                rasterizer_render_states_dirty = 1;
                halo::rasterizer::globals::sky_pass_active = 0;
                if (rasterizer_device_version < 0xffff0101) {
                    ((void (__stdcall *)(void **, int32_t, int32_t))(*(void ***)((uint8_t *)*rasterizer_device + 0xe4)))(
                        rasterizer_device, 0x89, 1);
                }
            }

            rasterizer_model_draw_prepare_states(1);

            {
                int16_t kind = *(int16_t *)((uint8_t *)tag_data + 0x24);
                if (kind == 1 || (4 < kind && kind < 0xc)) {
                    rasterizer_transparent_geometry_group_build(tag_data, 0, 0, 0, 2, 0, iStack_fc, &local_f0);
                } else {
                    rasterizer_shader_environment_draw_dispatch(tag_data, 0, 0, 0, 2, 0);
                }
            }
            rasterizer_model_draw_restore_states();

            if (halo::rasterizer::globals::models_enabled != 0 && rasterizer_device_version < 0xffff0101) {
                ((void (__stdcall *)(void **, int32_t, int32_t))(*(void ***)((uint8_t *)*rasterizer_device + 0xe4)))(
                    rasterizer_device, 0x89, 0);
            }
        }

        rasterizer_vertex_buffer_lock_state = 0;
        return;
    }
    rasterizer_vertex_buffer_lock_state = 0;
}

/**
 * Updates which players occupy the hill and the resulting hill state.
 *
 * @address 0x46acb0
 */
void Koth::update_hill_occupancy_state(void)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    if (current_game_engine == 0 || game_engine_variant.teams == 0) {
        int32_t count = 0;
        int32_t occupant_candidate = -1;
        int32_t occupant = -1;

        element = halo::memory::data_iterator_next(&iter);
        if (element != 0) {
            do {
                if (king_hill_single_occupant_flag != 0) {
                    count++;
                    occupant = occupant_candidate;
                }
                element = halo::memory::data_iterator_next(&iter);
            } while (element != 0);

            if (count > 1) {
                king_hill_state_globals.hill_state = _king_hill_contested;
                if (king_hill_state_globals.hill_ticks < 0x12d) {
                    king_hill_state_globals.hill_state = _king_hill_contested;
                    king_hill_state_globals.hill_ticks = 0;
                    king_hill_state_globals.occupant = (datum_index)0xffffffff;
                    return;
                }
                game_engine_queue_multiplayer_sound(0x27, 0xffffffff, 1);
                king_hill_state_globals.hill_ticks = 0;
                king_hill_state_globals.occupant = (datum_index)0xffffffff;
                return;
            }
            if (count != 0) {
                if (king_hill_state_globals.hill_state == _king_hill_held &&
                    occupant == (int32_t)king_hill_state_globals.occupant) {
                    king_hill_state_globals.hill_ticks++;
                } else {
                    king_hill_state_globals.hill_ticks = 0;
                    king_hill_state_globals.occupant = (datum_index)occupant;
                }
                king_hill_state_globals.hill_state = _king_hill_held;
                goto check_streak;
            }
        }
        king_hill_state_globals.hill_state = _king_hill_empty;
        king_hill_state_globals.hill_ticks = 0;
        king_hill_state_globals.occupant = (datum_index)0xffffffff;
        return;
    }
    {
        int32_t team0_count = 0;
        int32_t team1_count = 0;
        int32_t new_state;

        element = halo::memory::data_iterator_next(&iter);
        if (element == 0) {
            king_hill_state_globals.hill_state = _king_hill_empty;
            king_hill_state_globals.hill_ticks = 0;
            return;
        }
        do {
            if (king_hill_single_occupant_flag != 0) {
                if (*(int32_t *)((uint8_t *)element + 0x20) == 0) {
                    team0_count++;
                } else {
                    team1_count++;
                }
            }
            element = halo::memory::data_iterator_next(&iter);
        } while (element != 0);

        if (team1_count == 0) {
            if (team0_count == 0) {
                king_hill_state_globals.hill_state = _king_hill_empty;
                king_hill_state_globals.hill_ticks = 0;
                return;
            }
            new_state = _king_hill_team_0;
            if (king_hill_state_globals.hill_state == _king_hill_team_0) {
                king_hill_state_globals.hill_ticks++;
                king_hill_state_globals.hill_state = new_state;
                goto check_streak;
            }
        } else {
            if (team0_count != 0) {
                king_hill_state_globals.hill_state = _king_hill_contested;
                if (king_hill_state_globals.hill_ticks > 300) {
                    game_engine_queue_multiplayer_sound(0x27, 0xffffffff, 1);
                }
                king_hill_state_globals.hill_ticks = 0;
                return;
            }
            new_state = _king_hill_team_1;
            if (king_hill_state_globals.hill_state == _king_hill_team_1) {
                king_hill_state_globals.hill_ticks++;
                king_hill_state_globals.hill_state = new_state;
                goto check_streak;
            }
        }
        king_hill_state_globals.hill_ticks = 0;
        king_hill_state_globals.hill_state = new_state;
    }
check_streak:
    if (king_hill_state_globals.hill_ticks == 300) {
        game_engine_queue_multiplayer_sound(0x28, 0xffffffff, 1);
    }
}

/**
 * Removes an index from all slots of the hill-occupant tracking table, then re-adds it under its owning team's
 * slot if the associated object's tag supports it.
 *
 * Original register convention: ESI -> index.
 *
 * @address 0x46c320
 */
void Koth::update_occupant_table(uint32_t index)
{
    int32_t i;
    player *p;
    datum_index unit;

    if (game_engine_variant.engine.oddball.ball_type >= 1 && game_engine_variant.engine.oddball.ball_type <= 2) {
        return;
    }

    for (i = 0; i < 16; i++) {
        if (king_hill_occupant_table[i] == index) {
            king_hill_occupant_table[i] = 0xffffffff;
        }
    }

    p = (player *)((uint8_t *)player_data->data + (index & 0xffff) * sizeof(player));
    unit = p->unit;
    if (unit != (datum_index)0xffffffff) {
        unit_data *unit_obj = (unit_data *)((uint8_t *)
            ((object_header *)object_data->data)[unit & 0xffff].data + k_unit_data_offset);
        int16_t slot = unit_obj->current_weapon_index;
        if (slot != -1) {
            datum_index weapon = unit_obj->weapons[slot];
            if (weapon != (datum_index)0xffffffff) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon & 0xffff].data;
                uint32_t *tag_data = (uint32_t *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & 0xffff].data;
                if ((*(uint32_t *)((uint8_t *)tag_data + 0x308) >> 3 & 1) != 0) {
                    int16_t team = ((struct object *)weapon_obj)->owner_team;
                    king_hill_occupant_table[team] = index;
                }
            }
        }
    }
}

}
