/**
 * Hill and ball helpers shared by the king-of-the-hill and oddball engines: hill boundary, occupancy and
 * marker geometry.
 */

#include "tags.h"
#include "halo/networking/game_mode.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
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
#include "halo/rasterizer/render_device.hpp"
#include "halo/render/d3d9.hpp"
#include "rasterizer.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"
#include "halo/effects/api.hpp"

static auto &hill_pulse_fade_done = halo::link::ref<uint8_t>(halo::game::vars().hill_pulse_fade_done);
static auto &hill_pulse_grow_done = halo::link::ref<uint8_t>(halo::game::vars().hill_pulse_grow_done);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &game_engine_teams_enabled_flag = halo::link::ref<uint8_t>(halo::game::vars().game_engine_teams_enabled_flag);
static auto &king_alt_player_score = halo::link::ref<int32_t []>(halo::game::vars().king_alt_player_score);
static auto &king_alt_team_score = halo::link::ref<int32_t [16]>(halo::game::vars().king_alt_team_score);
static auto &king_alt_score_target = halo::link::ref<int32_t>(halo::game::vars().king_alt_score_target);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &king_hill_occupant_table = halo::link::ref<uint32_t [16]>(halo::game::vars().king_hill_occupant_table);
static auto &king_hill_occupant_last_tick = halo::link::ref<int32_t [16]>(halo::game::vars().king_hill_occupant_last_tick);
static auto &king_hill_idle_timeout = halo::link::ref<int32_t>(halo::game::vars().king_hill_idle_timeout);
static auto &shared_hud_text_draw_state = halo::link::ref<uint8_t>(halo::game::vars().shared_hud_text_draw_state);
static auto &king_team_hill_seconds_network = halo::link::ref<int32_t [16]>(halo::game::vars().king_team_hill_seconds_network);
static auto &king_bucket_credit_ticks = halo::link::ref<int32_t [16]>(halo::game::vars().king_bucket_credit_ticks);
static auto &king_hill_broadcast_overrun_value = halo::link::ref<int32_t>(halo::game::vars().king_hill_broadcast_overrun_value);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &king_alt_team_scores_network = halo::link::ref<int32_t [16]>(halo::game::vars().king_alt_team_scores_network);
static auto &king_alt_player_scores_network = halo::link::ref<int32_t [16]>(halo::game::vars().king_alt_player_scores_network);
static auto &king_alt_team_scores_network2 = halo::link::ref<int32_t [16]>(halo::game::vars().king_alt_team_scores_network2);
static auto &king_alt_scores_network_tail = halo::link::ref<int32_t [16]>(halo::game::vars().king_alt_scores_network_tail);
static auto &king_starting_location_type = halo::link::ref<int32_t>(halo::game::vars().king_starting_location_type);
static auto &king_starting_location_count = halo::link::ref<int32_t>(halo::game::vars().king_starting_location_count);
static auto &king_hill_boundary_points = halo::link::ref<real_point3d [12]>(halo::game::vars().king_hill_boundary_points);
static auto &king_hill_boundary_extra = halo::link::ref<uint32_t [12][2]>(halo::game::vars().king_hill_boundary_extra);
static auto &king_hill_boundary_min_z = halo::link::ref<float>(halo::game::vars().king_hill_boundary_min_z);
static auto &king_hill_boundary_max_z = halo::link::ref<float>(halo::game::vars().king_hill_boundary_max_z);
static auto &king_hill_boundary_center = halo::link::ref<real_point3d>(halo::game::vars().king_hill_boundary_center);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &king_hill_player_in_hill = halo::link::ref<uint8_t [16]>(halo::game::vars().king_hill_player_in_hill);
static auto &king_bucket_last_credit_tick = halo::link::ref<int32_t [16]>(halo::game::vars().king_bucket_last_credit_tick);
static auto &global_white_color = halo::link::ref<const real_vector3d *>(halo::effects::vars().global_white_color);
static auto &king_hill_markers = halo::link::ref<king_hill_marker_history>(halo::game::vars().king_hill_markers);
static auto &rasterizer_dynamic_vertex_slots = halo::link::ref<rasterizer_dynamic_vertex_slot [k_rasterizer_dynamic_vertex_slots]>(halo::game::vars().rasterizer_dynamic_vertex_slots);
static auto &rasterizer_dynamic_vertex_caches = halo::link::ref<rasterizer_dynamic_vertex_cache [k_rasterizer_vertex_type_count]>(halo::rasterizer::vars().rasterizer_dynamic_vertex_caches);
static auto &rasterizer_vertex_buffer_slots = halo::link::ref<rasterizer_vertex_buffer_slot [k_rasterizer_vertex_buffer_slots]>(halo::rasterizer::vars().rasterizer_vertex_buffer_slots);
namespace {
constexpr size_t k_hill_marker_vertex_size = 0x44;
constexpr size_t k_hill_marker_vertex_copy_size = 0x3c;
}

static auto &k_render_identity_matrix_ptr = halo::link::ref<void *>(halo::effects::vars().k_render_identity_matrix_ptr);
static auto &global_white_argb = halo::link::ref<const ColorARGB *>(halo::networking::vars().global_white_argb);
static auto &default_axis_b = halo::link::ref<const real_vector3d *>(halo::game::vars().default_axis_b);
static auto &king_hill_single_occupant_flag = halo::link::ref<uint8_t>(halo::game::vars().king_hill_single_occupant_flag);
static auto &king_hill_state_globals = halo::link::ref<king_globals>(halo::game::vars().king_hill_state_globals);

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
    player *fading = halo::game::player_at(fading_player);
    player *growing = halo::game::player_at(growing_player);

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
    player *p = halo::game::player_at(player_index);

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        king_alt_player_score[player_index & halo::k_datum_slot_mask]++;
        king_alt_team_score[p->team]++;
        if (king_alt_score_target - king_alt_team_score[p->team] == 900) {
            halo::game::game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0
                ? 5 + 2 * (p->team != 0) : 3, halo::k_dword_none, 1);
        }
        if (king_alt_score_target - king_alt_team_score[p->team] == halo::game::k_ticks_per_minute) {
            halo::game::game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0
                ? 4 + 2 * (p->team != 0) : 2, halo::k_dword_none, 1);
        }
    }

    if (king_alt_team_score[p->team] < king_alt_score_target) {
        return;
    }
    halo::game::game_engine_begin_end_game_sequence();
}

/**
 * Checks whether a hill has gone unclaimed for too long and, if so, announces it via the kill feed and
 * relocates it; otherwise records the current tick as the hill's last-active time.
 *
 * @address 0x46c5d0
 */
void Koth::ball_idle_tick(uint32_t object_handle, object *obj)
{
    item_data *item = halo::game::item_data_of(obj);
    real_point3d position;
    object_header *hdr;
    int32_t tick;

    if (halo::items::item_get_effective_position((datum_index)object_handle, &position) != 1) {
        return;
    }

    hdr = (object_header *)halo::objects::globals().object_data->data + (object_handle & halo::k_datum_slot_mask);
    if ((hdr->flags & 0x08) != 0) {
        return;
    }

    halo::game::custom_waypoint_register((datum_index)halo::k_dword_none, (int16_t)0, &position, "ball_blue", 0.0f,
        (datum_index)halo::k_dword_none, (int16_t)halo::k_word_none);

    tick = game_time->game_time;
    if ((uint32_t)(game_time->game_time - item->held_game_time) > 0x4b0) {
        if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
            return;
        }
        if (halo::items::weapon_must_be_readied((datum_index)object_handle) != 0 && (obj->flags >> 0xb & 1) != 0 && obj->parent_object == (datum_index)halo::k_dword_none) {
            if ((((weapon_object *)obj)->weapon.flags & _weapon_game_object_taken_bit) != 0) {
                data_iterator iter;
                void *element;
                iter.data = halo::objects::globals().object_data;
                iter.next_index = 0;
                iter.index = (datum_index)halo::k_dword_none;
                iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                element = halo::memory::data_iterator_next(&iter);
                while (element != 0) {
                    halo::game::chimera__kill_feed((datum_index)halo::k_dword_none, 0x26, (uint32_t)halo::k_dword_none, 1, 0);
                    element = halo::memory::data_iterator_next(&iter);
                }
            }
            halo::game::game_engine_koth_relocate_object_hill(object_handle);
        }
    }
    tick = game_time->game_time;
    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    if (game_engine_variant.engine.oddball.ball_type < 1 || game_engine_variant.engine.oddball.ball_type > 2) {
        int16_t team = ((object *)obj)->owner_team;
        if (king_hill_occupant_last_tick[team] == -1 ||
            king_hill_occupant_last_tick[team] + king_hill_idle_timeout < tick) {
            if (obj->parent_object == (datum_index)halo::k_dword_none && (obj->flags & _object_at_rest_bit) != 0) {
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
        encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::koth_hill_times), 0, &field, 0, 1, 0);
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
            encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1, halo::networking::message_id(halo::networking::delta_message::koth_hill_times), 0, (void **)&seconds_field, (uint32_t)&king_team_hill_seconds_network[0], 1, 0);
        }

        for (i = 0; i < 16; i++) {
            king_team_hill_seconds_network[i] = seconds[i];
        }
        king_hill_broadcast_overrun_value = seconds[106];
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            halo::networking::network_session_send_to_machine(machine_index, halo::networking::globals().server, 1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
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
        encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::koth_team_scores), 0, &field, 0, 1, 0);
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
            encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1, halo::networking::message_id(halo::networking::delta_message::koth_team_scores), 0, (void **)&fields0,
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
            halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            halo::networking::network_session_send_to_machine(machine_index, halo::networking::globals().server, 1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
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

    count = halo::game::game_engine_find_valid_starting_locations(
        (real_point3d *)0, 0.0f, 0.0f, 8, (int16_t)king_starting_location_type, 0xc, indices);
    king_starting_location_count = count;
    if (count == 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        ScenarioNetgameFlags *loc =
            &((ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer)[indices[i]];
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

    halo::game::point3d_array_project_to_xy_plane(points, hull_points, count);
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
            total_length = (float)halo::libm::sqrt((double)(dx * dx + dy * dy + dz * dz)) + total_length;
        }
    }

    length_period = halo::libm::floor((double)(total_length + 0.5f));

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

            running_length = (float)halo::libm::sqrt((double)((nxt->x - cur->x) * (nxt->x - cur->x) +
                                    (nxt->y - cur->y) * (nxt->y - cur->y) +
                                    (nxt->z - cur->z) * (nxt->z - cur->z))) + running_length;
            u_end = (float)(length_period / (double)total_length) * running_length;

            nx = -(nxt->y - cur->y) * (z_a - cur->z);
            ny = (nxt->x - cur->x) * (z_a - cur->z);
            nz = 0.0f;
            nlen = (float)halo::libm::sqrt((double)(nx * nx + ny * ny + nz * nz));
            if (halo::libm::fabs((double)nlen) >= 0.0001) {
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
                halo::game::game_engine_koth_submit_hill_marker_geometry(hill_shader_tag, (uint32_t *)0, (uint32_t *)0,
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
    uint32_t idx = player_index & halo::k_datum_slot_mask;
    player *p = (player *)((uint8_t *)player_data->data + idx * sizeof(player));
    int32_t occupied_slots;
    int32_t i;
    uint32_t result = 0;

    p->hud_message_index = (datum_index)halo::k_dword_none;
    p->hud_message_player = (datum_index)halo::k_dword_none;

    halo::game::game_engine_koth_update_occupant_table(player_index);

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
            halo::game::unit_reset_gauge_if_flagged(player_index);
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
            halo::game::game_engine_koth_alt_scorer_tick(player_index);
            remaining--;
        } while (remaining != 0);
    }

    if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type < 3 && occupied_slots > 0) {
        p->hud_message_index = (datum_index)0x23;
        p->hud_message_player = (datum_index)player_index;
    }

    if (p->unit != (datum_index)halo::k_dword_none) {
        unit_data *unit = (unit_data *)((uint8_t *)
            halo::game::object_at((uint32_t)p->unit) +
            k_unit_data_offset);
        if (unit->current_weapon_index != -1) {
            datum_index weapon = unit->weapons[unit->current_weapon_index];
            if (weapon != (datum_index)halo::k_dword_none) {
                object *weapon_obj = halo::game::object_at(weapon);
                uint32_t *tag_data = (uint32_t *)halo::game::tag_data_at(weapon_obj->definition_tag);
                if ((halo::game::weapon_flag_set(tag_data, halo::tags::weapon_tag_flag::must_be_readied)) != 0) {
                    int32_t score = king_alt_player_score[idx];
                    if (score > 0 && score % halo::game::seconds_to_ticks(5) == 0 && score < king_alt_score_target) {
                        halo::game::game_engine_queue_multiplayer_sound(0x2a, halo::k_dword_none, 0);
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
        halo::game::game_engine_find_valid_starting_locations((real_point3d *)0, 0.0f, 0.0f, 2, type_filter, 1, &index);
    }

    if (index == -1) {
        int32_t flag_count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;
        int32_t matching = 0;
        int32_t i;

        for (i = 0; i < flag_count; i++) {
            if (flags[i].type == 2) {
                matching++;
            }
        }

        if (matching != 0) {
            int32_t pick;
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
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
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;

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
    object *obj = halo::game::object_at(object_handle);

    if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type < 3) {
        halo::game::game_engine_broadcast_kill_feed_by_relationship(player_index, 0x20, 0x21, 0x22, player_index, 0);
        return 1;
    }

    {
        player *p = halo::game::player_at(player_index);
        uint8_t eligible = 1;
        if (p->unit != (datum_index)halo::k_dword_none) {
            uint16_t found = halo::units::unit_find_weapon_index_by_flag((uint32_t)p->unit, 3);
            eligible = 1 - (found != 0);
            if (eligible != 0) {
                ((weapon_object *)obj)->weapon.flags |= _weapon_game_object_taken_bit;
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

    if (player_index == halo::k_dword_none) {
        return 0;
    }
    p = halo::game::player_at(player_index);
    unit = p->unit;
    if (unit == (datum_index)halo::k_dword_none) {
        return 0;
    }
    unit_obj = halo::game::object_at(unit);
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
    uint32_t idx = player_index & halo::k_datum_slot_mask;
    player *p = (player *)((uint8_t *)player_data->data + idx * sizeof(player));

    *(uint32_t *)&((struct player *)p)->hud_message_index = 0xffffffff;
    *(uint32_t *)&((struct player *)p)->hud_message_player = 0xffffffff;
    king_hill_player_in_hill[idx] = 0;

    if (p->unit != (datum_index)halo::k_dword_none &&
        (current_game_engine == 0 || game_engine_state_value == 0) &&
        halo::game::game_engine_koth_player_in_hill_bounds(player_index) != 0) {
        uint8_t hosting = (halo::networking::globals().game_mode == halo::networking::k_game_mode_host);

        king_hill_player_in_hill[idx] = 1;
        if (hosting) {
            ((struct player *)p)->objective_time_words.low += 1;
        }

        if (king_bucket_last_credit_tick[p->team] < game_time->game_time && halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
            int32_t limit_ticks = game_engine_variant.score_limit * halo::game::k_ticks_per_minute;
            int32_t bucket;

            king_bucket_credit_ticks[p->team]++;
            king_bucket_last_credit_tick[p->team] = game_time->game_time;
            bucket = king_bucket_credit_ticks[p->team];

            if (limit_ticks - bucket == 900) {
                halo::game::game_engine_queue_multiplayer_sound(halo::game::game_engine_get_teams_enabled() != 0
                    ? 5 + 2 * (p->team != 0) : 3, halo::k_dword_none, 1);
            }
            if (limit_ticks - bucket == halo::game::k_ticks_per_minute) {
                halo::game::game_engine_queue_multiplayer_sound(halo::game::game_engine_get_teams_enabled() != 0
                    ? 4 + 2 * (p->team != 0) : 2, halo::k_dword_none, 1);
            }
            bucket = king_bucket_credit_ticks[p->team];
            if (bucket > 0 && bucket % halo::game::seconds_to_ticks(5) == 0 && bucket < limit_ticks) {
                halo::game::game_engine_queue_multiplayer_sound(0x2a, player_index, 1);
            }
            if (limit_ticks <= king_bucket_credit_ticks[p->team]) {
                halo::game::game_engine_begin_end_game_sequence();
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

            halo::objects::object_placement_data_initialize(&placement, ball_tag, (datum_index)halo::k_dword_none);
            placement.owner_team = (int16_t)ball_index;
            halo::game::game_engine_koth_find_marker_position(&placement.position, (int16_t)ball_index);

            new_object = halo::objects::object_new(&placement);

            hdr = (object_header *)halo::objects::globals().object_data->data + ((uint32_t)new_object & halo::k_datum_slot_mask);
            header_flags = hdr->flags;
            hdr->flags = header_flags & ~_object_header_in_pvs_pass_bit;
            if ((header_flags & _object_header_active_bit) == 0) {
                halo::objects::object_mark_pending_delete((uint32_t)new_object);
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
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        object *obj = halo::game::object_at(object_index);
        real_point3d discarded_position;

        halo::game::game_engine_koth_find_marker_position(&discarded_position, ((object *)obj)->owner_team);

        if (game_engine_variant.engine.oddball.ball_count < 3) {
            halo::game::game_engine_queue_multiplayer_sound(0x1e, halo::k_dword_none, 1);
        }
        halo::game::ctf_flag_object_clear_carrier(object_index, &discarded_position);
        ((weapon_object *)obj)->weapon.flags &= ~(uint32_t)_weapon_game_object_taken_bit;
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

#pragma pack(push, 1)
/** One 0x44-byte vertex of the hill marker quad as the dynamic vertex cache holds it. */
struct hill_marker_vertex {
    real_point3d position;       // 0x00
    real_vector3d normal;        // 0x0c
    real_vector3d tangent;       // 0x18
    uint32_t unknown_24[5];      // 0x24
    uint16_t unknown_38;         // 0x38
    uint16_t unknown_3a;         // 0x3a
    uint32_t center_bits[2];     // 0x3c both k_float_half_bits (0.5f)
};
static_assert(sizeof(hill_marker_vertex) == 0x44);

/** The 0x74-byte shading block of the marker draw record, copied verbatim from the position override. */
struct hill_marker_shading {
    real_vector3d tint;          // 0x00
    uint16_t unknown_0c;         // 0x0c
    uint8_t unknown_0e[0x42 - 0x0e];
    uint16_t unknown_42;         // 0x42
    uint16_t unknown_44;         // 0x44
    ColorARGB color;             // 0x46
    real_vector3d axis;          // 0x56
    uint32_t unknown_62;         // 0x62
    uint32_t scale_bits;         // 0x66 1.0f in the default block
    uint32_t unknown_6a;         // 0x6a
    uint8_t unknown_6e[0x74 - 0x6e];
};
static_assert(sizeof(hill_marker_shading) == 0x74);

/** The 0xd8-byte draw record Koth::submit_hill_marker_geometry fills in for the marker model draw. */
struct hill_marker_draw_record {
    uint32_t unknown_00;         // 0x00
    uint32_t unknown_04;         // 0x04 set to 1
    uint16_t unknown_08;         // 0x08 set to 1
    void *matrix;                // 0x0a k_render_identity_matrix_ptr
    hill_marker_shading shading; // 0x0e
    uint8_t unknown_82[0x98 - 0x82];
    float center[3];             // 0x98 the centroid of the four vertices
    uint8_t unknown_a4[0xc8 - 0xa4];
    void *position_table;        // 0xc8 the orientation override's first word, else the hill marker positions
    void *state_table;           // 0xcc the orientation override's second word, else the hill marker states
    uint32_t param_4;            // 0xd0
    uint32_t param_5;            // 0xd4
};
#pragma pack(pop)
static_assert(sizeof(hill_marker_draw_record) == 0xd8);
static_assert(offsetof(hill_marker_draw_record, shading) == 0x0e);
static_assert(offsetof(hill_marker_draw_record, center) == 0x98);
static_assert(offsetof(hill_marker_draw_record, position_table) == 0xc8);
static_assert(offsetof(hill_marker_draw_record, param_5) == 0xd4);

/**
 * Draws one quad of the moving King-of-the-Hill marker with the given shader tag. The four source vertices
 * (model vertices, 0x44 bytes each) are copied into the dynamic vertex cache with the last two floats set to 0.5,
 * the two triangles of the quad are written into the dynamic index cache, both caches are unlocked, and the
 * shader is drawn through a model draw context whose sort position is the centre of the quad.
 *
 * The lighting override (or a white, unlit default) and the animation override (or the marker history, which
 * supplies the change colours and function values) fill the draw context; param_4 and param_5 are the float
 * bit patterns of the base map u and v scales. Transparent shader types are batched through the transparent
 * geometry group, all others are drawn directly.
 *
 * @address 0x46b2f0
 */
void Koth::submit_hill_marker_geometry(uint32_t tag_handle_as_uint, uint32_t *position_override, uint32_t *orientation_override, uint32_t param_4, uint32_t param_5, float *vertex_source)
{
    const int32_t k_vertex_count = 4;
    const int32_t k_triangle_count = 2;
    const render_lighting *lighting_override = (const render_lighting *)position_override;
    int32_t index_slot;
    int32_t vertex_slot;

    halo::rasterizer::globals().vertex_buffer_lock_state = 9;
    index_slot = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(k_triangle_count);
    vertex_slot = halo::rasterizer::rasterizer_dynamic_vertex_cache_reserve(_rasterizer_vertex_type_model_uncompressed, k_vertex_count);
    if (index_slot == -1 || vertex_slot == -1) {
        halo::rasterizer::globals().vertex_buffer_lock_state = 0;
        return;
    }

    {
        uint8_t *vertices = (uint8_t *)halo::rasterizer::rasterizer_dynamic_vertex_cache_lock(vertex_slot);
        uint16_t *indices = (uint16_t *)halo::render::rasterizer_dynamic_index_slot_lock(index_slot);
        int32_t vertex;
        size_t word;

        for (vertex = 0; vertex < k_vertex_count; vertex++) {
            const uint8_t *source = (const uint8_t *)vertex_source + vertex * k_hill_marker_vertex_size;
            uint8_t *destination = vertices + vertex * k_hill_marker_vertex_size;
            float *tail = (float *)(destination + k_hill_marker_vertex_copy_size);

            for (word = 0; word < k_hill_marker_vertex_copy_size / sizeof(uint32_t); word++) {
                ((uint32_t *)destination)[word] = ((const uint32_t *)source)[word];
            }
            tail[0] = 0.5f;
            tail[1] = 0.5f;
        }

        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 2;
        indices[4] = 3;
        indices[5] = 0;
    }

    {
        rasterizer_dynamic_vertex_slot &slot = rasterizer_dynamic_vertex_slots[vertex_slot];
        int32_t buffer_handle = rasterizer_dynamic_vertex_caches[slot.vertex_type].buffer_handle;

        halo::rasterizer::render_device().buffer_unlock(halo::rasterizer::globals().dynamic_index_buffer);
        if (buffer_handle != 0) {
            halo::rasterizer::render_device().buffer_unlock(
                (void *)(uintptr_t)rasterizer_vertex_buffer_slots[buffer_handle - 1].hardware_buffer);
        }
    }

    {
        Shader *shader = (Shader *)halo::game::tag_data_at(tag_handle_as_uint);
        const float *corner = vertex_source;
        const size_t stride = k_hill_marker_vertex_size / sizeof(float);
        real_point3d center;
        rasterizer_model_draw_context context;
        uint32_t *context_words = (uint32_t *)&context;
        size_t word;

        center.x = (corner[3 * stride] + corner[2 * stride] + corner[stride] + corner[0]) * 0.25f;
        center.y = (corner[3 * stride + 1] + corner[2 * stride + 1] + corner[stride + 1] + corner[1]) * 0.25f;
        center.z = (corner[3 * stride + 2] + corner[2 * stride + 2] + corner[stride + 2] + corner[2]) * 0.25f;

        for (word = 0; word < sizeof(context) / sizeof(uint32_t); word++) {
            context_words[word] = 0;
        }
        context.object_index = 1;
        context.node_matrices = (uint32_t)(uintptr_t)k_render_identity_matrix_ptr;
        context.node_count = 1;

        if (lighting_override != (const render_lighting *)0) {
            context.lighting = *lighting_override;
        } else {
            context.lighting.ambient_color = *(const ColorRGB *)global_white_color;
            context.lighting.distant_light_count = 0;
            context.lighting.point_light_count = 0;
            context.lighting.reflection_tint = *global_white_argb;
            context.lighting.shadow_vector.i = 0.0f;
            context.lighting.shadow_vector.j = 1.0f;
            context.lighting.shadow_vector.k = 0.0f;
            context.lighting.shadow_color = *(const ColorRGB *)default_axis_b;
        }

        if (orientation_override != (uint32_t *)0) {
            context.change_colors = orientation_override[0];
            context.function_values = orientation_override[1];
        } else {
            context.change_colors = (uint32_t)(uintptr_t)&king_hill_markers.position[0];
            context.function_values = (uint32_t)(uintptr_t)&king_hill_markers.state[0];
        }
        context.center = center;
        context.base_map_u_scale = *(float *)&param_4;
        context.base_map_v_scale = *(float *)&param_5;

        if (halo::rasterizer::fields::models_enabled != 0) {
            halo::rasterizer::globals().render_states_dirty = 1;
            halo::rasterizer::fields::sky_pass_active = 0;
            if ((uint32_t)halo::rasterizer::globals().device_version < halo::d3d9::k_pixel_shader_version_1_1) {
                halo::rasterizer::render_device().set_render_state((uint32_t)halo::d3d9::render_state::lighting, 1);
            }
        }

        halo::rasterizer::rasterizer_model_draw_prepare_states(&context, 1);
        if (shader->shader_type == 1 || (4 < shader->shader_type && shader->shader_type < 0xc)) {
            halo::rasterizer::rasterizer_transparent_geometry_group_build(nullptr, (uint8_t *)shader, 0, nullptr,
                index_slot, k_triangle_count, nullptr, vertex_slot, &center);
        } else {
            halo::rasterizer::rasterizer_shader_environment_draw_dispatch(vertex_slot, (uint8_t *)shader, 0, nullptr,
                index_slot, k_triangle_count, nullptr);
        }
        halo::rasterizer::rasterizer_model_draw_restore_states();

        if (halo::rasterizer::fields::models_enabled != 0 &&
            (uint32_t)halo::rasterizer::globals().device_version < halo::d3d9::k_pixel_shader_version_1_1) {
            halo::rasterizer::render_device().set_render_state((uint32_t)halo::d3d9::render_state::lighting, 0);
        }
    }
    halo::rasterizer::globals().vertex_buffer_lock_state = 0;
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
    auto check_streak = []() {
        if (king_hill_state_globals.hill_ticks == 300) {
            halo::game::game_engine_queue_multiplayer_sound(0x28, halo::k_dword_none, 1);
        }
    };

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
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
                    king_hill_state_globals.occupant = (datum_index)halo::k_dword_none;
                    return;
                }
                halo::game::game_engine_queue_multiplayer_sound(0x27, halo::k_dword_none, 1);
                king_hill_state_globals.hill_ticks = 0;
                king_hill_state_globals.occupant = (datum_index)halo::k_dword_none;
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
                check_streak();
                return;
            }
        }
        king_hill_state_globals.hill_state = _king_hill_empty;
        king_hill_state_globals.hill_ticks = 0;
        king_hill_state_globals.occupant = (datum_index)halo::k_dword_none;
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
                check_streak();
                return;
            }
        } else {
            if (team0_count != 0) {
                king_hill_state_globals.hill_state = _king_hill_contested;
                if (king_hill_state_globals.hill_ticks > 300) {
                    halo::game::game_engine_queue_multiplayer_sound(0x27, halo::k_dword_none, 1);
                }
                king_hill_state_globals.hill_ticks = 0;
                return;
            }
            new_state = _king_hill_team_1;
            if (king_hill_state_globals.hill_state == _king_hill_team_1) {
                king_hill_state_globals.hill_ticks++;
                king_hill_state_globals.hill_state = new_state;
                check_streak();
                return;
            }
        }
        king_hill_state_globals.hill_ticks = 0;
        king_hill_state_globals.hill_state = new_state;
    }
    check_streak();
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
            king_hill_occupant_table[i] = halo::k_dword_none;
        }
    }

    p = halo::game::player_at(index);
    unit = p->unit;
    if (unit != (datum_index)halo::k_dword_none) {
        unit_data *unit_obj = (unit_data *)((uint8_t *)
            halo::game::object_at(unit) + k_unit_data_offset);
        int16_t slot = unit_obj->current_weapon_index;
        if (slot != -1) {
            datum_index weapon = unit_obj->weapons[slot];
            if (weapon != (datum_index)halo::k_dword_none) {
                object *weapon_obj = halo::game::object_at(weapon);
                uint32_t *tag_data = (uint32_t *)halo::game::tag_data_at(weapon_obj->definition_tag);
                if ((halo::game::weapon_flag_set(tag_data, halo::tags::weapon_tag_flag::must_be_readied)) != 0) {
                    int16_t team = ((struct object *)weapon_obj)->owner_team;
                    king_hill_occupant_table[team] = index;
                }
            }
        }
    }
}

}
