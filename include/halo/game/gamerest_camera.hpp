#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "camera.h"

namespace halo::game {

/**
 * Observer-camera target selection: candidate generation, scoring and per-tick update.
 */
class CameraObserver {
public:
    CameraObserver() = delete;

    static uint16_t collect_target_candidates(observer_target_cone *cone, datum_index start_object, real_point3d *observer_position, real_vector3d *facing, real max_distance, real sin_max_angle, real cos_max_angle, datum_index exclude_object, int16_t observer_team, int16_t capacity, observer_target_candidate *out);
    static char find_best_target(real_point3d *observer_position, observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team, observer_target_candidate *out);
    static int16_t generate_target_candidates(observer_target_cone *cone, int16_t start_cluster, real_point3d *observer_position, real_vector3d *facing, datum_index exclude_object, int16_t team, int16_t capacity, observer_target_candidate *out);
    static uint32_t get_target_angles(real *out_weight_primary, real *out_weight_secondary, real *out_yaw_pitch, real *out_yaw_pitch_rate, int16_t local_player_slot);
    static uint32_t get_target_id(datum_index *out_id, int16_t local_player_slot);
    static int32_t target_compare(const observer_target_candidate *a, const observer_target_candidate *b);
    static uint32_t target_direction(real_point3d *candidate_point, real_vector3d *facing, real_point3d *reference_position, datum_index object, datum_index exclude_object, real_vector3d *out_direction, real *out_distance, real *out_angle);
    static char target_is_valid(datum_index exclude_object, real_point3d *observer_position, real_point3d *target_position, datum_index target_object);
    static uint32_t target_score(real_vector3d *facing, observer_target_cone *cone, datum_index target, observer_target_candidate *out, real_point3d *reference_position);
    static uint32_t update(datum_index player_index, real_point3d *observer_position, real_vector3d *fallback_facing);
};

/**
 * Spectator first-person camera helpers.
 */
class SpectateCamera {
public:
    SpectateCamera() = delete;

    static void spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index);
};

}  // namespace halo::game
