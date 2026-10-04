#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/game/gamerest_camera.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include <stdlib.h>
#include "halo/physics/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &player_control_globals_ptr = halo::link::ref<player_control_globals *>(halo::game::vars().player_control_globals_ptr);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);

namespace halo::game {

/**
 * Walks the sibling list starting at `start_object`, scoring every biped that passes the
 * frustum/team/tag-flag filters into `out` (up to `capacity` entries) and recursing into every
 * unit's children (so a vehicle's passengers are still visited even though the vehicle itself is
 * never scored). Returns the number of entries written.
 *
 * @address 0x45a0e0
 */
uint16_t CameraObserver::collect_target_candidates(observer_target_cone *cone, datum_index start_object, real_point3d *observer_position, real_vector3d *facing, real max_distance, real sin_max_angle, real cos_max_angle, datum_index exclude_object, int16_t observer_team, int16_t capacity, observer_target_candidate *out)
{
    datum_index object_index;
    object *obj;
    uint32_t type_bit;
    uint16_t count;
    observer_target_candidate temp;
    datum_index candidate_team_player;
    int16_t candidate_team;
    Item *tag;

    count = 0;
    object_index = start_object;
    do {
        obj = halo::game::object_at(object_index);
        type_bit = halo::objects::object_type_mask_of(obj->type);
        if ((type_bit & _object_mask_unit) != 0 && (obj->flags & 1) == 0 &&
            halo::game::unit_data_of(obj)->active_camouflage_power < 1.0f) {

            if (halo::math::vector3d_projection_band_test(*facing, *observer_position, obj->bounding_center,
                                               obj->bounding_radius, max_distance,
                                               sin_max_angle, cos_max_angle) != 0) {
                if ((type_bit & _object_mask_biped) != 0 && (obj->vitality_flags & _object_health_frozen_bit) == 0 &&
                    object_index != exclude_object) {

                    (void)candidate_team_player;
                    candidate_team = obj->owner_team;
                    if (halo::game::teams_are_enemies(candidate_team, observer_team) != 0) {
                        tag = (Item *)halo::game::tag_data_at(obj->definition_tag);
                        if ((tag->item_flags & 0x200000) == 0) {
                            if (CameraObserver::target_score(facing, cone, object_index, &temp, observer_position)  != 0 &&
                                count < (uint16_t)capacity) {
                                out[count] = temp;
                                count = count + 1;
                            }
                        }
                    }
                }
                if (obj->first_child_object != k_datum_index_none && count < (uint16_t)capacity) {
                    count = count + CameraObserver::collect_target_candidates(cone, obj->first_child_object, observer_position, facing, max_distance, sin_max_angle, cos_max_angle, exclude_object, observer_team, (int16_t)(capacity - count), out + count);
                }
            }
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none && count < (uint16_t)capacity);
    return count;
}

/**
 * Generates up to 64 candidates for the current camera cluster, sorts them by
 * camera_observer_target_compare, and returns the first one that still passes
 * camera_observer_target_is_valid.
 *
 * @address 0x459a00
 */
char CameraObserver::find_best_target(real_point3d *observer_position, observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team, observer_target_candidate *out)
{
    observer_target_candidate candidates[64];
    int32_t cluster;
    int16_t start_cluster;
    int16_t candidate_count;
    int16_t i;

    cluster = (int32_t)halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, observer_position);
    if (cluster != -1) {

        start_cluster = *(int16_t *)((cluster & 0x7fffffff) * 0x10 + 8 +
                                     *(int32_t *)&halo::scenario::globals().structure_bsp->leaves.pointer);
        if (start_cluster != -1) {
            candidate_count = CameraObserver::generate_target_candidates(cone, start_cluster, observer_position, facing, exclude_object, team, 64, candidates);
            if (candidate_count < 1) {
                return 0;
            }
            qsort(candidates, (uint32_t)candidate_count, sizeof(observer_target_candidate),
                  (int (*)(const void *, const void *))halo::game::camera_observer_target_compare);
            for (i = 0; i < candidate_count; i = i + 1) {
                if (CameraObserver::target_is_valid(exclude_object, observer_position, &candidates[i].point, candidates[i].object) != 0) {
                    *out = candidates[i];
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**
 * Builds a detection frustum from `cone`'s widest angle/distance bounds, gathers the clusters it
 * touches, then walks each cluster with camera_observer_collect_target_candidates, stopping once
 * `capacity` candidates have been written to `out`.
 *
 * @address 0x459f70
 */
int16_t CameraObserver::generate_target_candidates(observer_target_cone *cone, int16_t start_cluster, real_point3d *observer_position, real_vector3d *facing, datum_index exclude_object, int16_t team, int16_t capacity, observer_target_candidate *out)
{
    real max_distance;
    real max_angle;
    real sin_max_angle;
    real cos_max_angle;
    int16_t cluster_count;
    int16_t i;
    int16_t total;
    int16_t cluster_indices[512];
    datum_index cluster_heads[2048];
    int16_t collected_clusters;

    max_distance = (cone->distance_a <= cone->distance_b) ? cone->distance_b : cone->distance_a;
    max_angle = (cone->angle_a <= cone->angle_b) ? cone->angle_b : cone->angle_a;
    if (max_distance <= 0.0f || max_angle <= 0.0f) {
        return 0;
    }

    sin_max_angle = (real)halo::libm::sin((double)max_angle);
    cos_max_angle = (real)halo::libm::cos((double)max_angle);

    collected_clusters = halo::structures::cluster_flood_fill_with_predicate(observer_position, facing, max_distance,
                                      sin_max_angle, cos_max_angle, 0x200, cluster_indices, start_cluster);
    cluster_count = halo::objects::object_collect_in_clusters(1, collected_clusters, cluster_indices, 0x800,
                                               cluster_heads);

    total = 0;
    if (0 < cluster_count) {
        for (i = 0; i < cluster_count; i = i + 1) {
            total = total + (int16_t)CameraObserver::collect_target_candidates(cone, cluster_heads[i], observer_position, facing, max_distance, sin_max_angle, cos_max_angle, exclude_object, team, (int16_t)(capacity - total), out + total);
            if (capacity <= total) {
                return total;
            }
        }
    }
    return total;
}

/**
 * REWRITTEN 2026-09-27 (static loop) from objdump 0x4596f0..0x4598f4 -- the aim-assist target for a local player
 * (caller: game_engine_build_local_player_control_input). Draft defects fixed: camera_get_type_for_player takes the
 * slot (CX); the autoaim cone query gets the player's desired_zoom_level (EDX, control record +0x24; -1 without a
 * player) instead of 0; the yaw / pitch RATES come from the ROOT-OBJECT VELOCITIES of the target and the player's unit
 * (0x4f6aa0), not from camera-row positions.
 * blam-cc: AX -> local_player_slot, stack -> out_weight_primary, out_weight_secondary, out_yaw_pitch, out_yaw_pitch_rate
 *
 * @address 0x4596f0
 */
uint32_t CameraObserver::get_target_angles(real *out_weight_primary, real *out_weight_secondary, real *out_yaw_pitch, real *out_yaw_pitch_rate, int16_t local_player_slot)
{
    int16_t camera_type;
    datum_index player_index;
    player *p;
    datum_index unit_index;
    int16_t zoom_level;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint8_t *row;
    real_vector3d player_velocity;
    real_vector3d target_velocity;
    real dx, dy, dz, h2, h;

    camera_type = halo::camera::camera_get_type_for_player(local_player_slot);
    *out_weight_primary = 0.0f;
    *out_weight_secondary = 0.0f;
    out_yaw_pitch[1] = 0.0f;
    out_yaw_pitch[0] = 0.0f;
    out_yaw_pitch_rate[1] = 0.0f;
    out_yaw_pitch_rate[0] = 0.0f;
    if (camera_type != 0 && camera_type != 1) {
        return halo::k_dword_none;
    }

    if (local_player_slot == -1 || 0 < local_player_slot) {
        player_index = k_datum_index_none;
    } else {
        player_index = local_player_globals->local_players[local_player_slot];
    }
    p = halo::game::player_at(player_index);
    unit_index = p->unit;

    zoom_level = -1;
    if (local_player_slot != -1) {
        zoom_level = player_control_globals_ptr->local_players[local_player_slot].desired_zoom_level;
    }
    if (halo::game::unit_get_current_weapon_autoaim_cone(unit_index, zoom_level, cone_buffer) == 0) {
        return halo::k_dword_none;
    }

    row = (local_player_slot == -1) ? (uint8_t *)0 : (uint8_t *)&halo::camera::globals().observers[local_player_slot].camera;
    if (CameraObserver::find_best_target((real_point3d *)row, (observer_target_cone *)cone_buffer, (real_vector3d *)(row + 0x20), unit_index, (int16_t)p->team, &candidate) == 0) {
        return halo::k_dword_none;
    }

    *out_weight_primary = candidate.weight_primary;
    *out_weight_secondary = candidate.weight_secondary;
    out_yaw_pitch[0] = (real)halo::libm::atan2((double)candidate.offset.j, (double)candidate.offset.i);
    h2 = candidate.offset.i * candidate.offset.i + candidate.offset.j * candidate.offset.j;
    out_yaw_pitch[1] = (real)halo::libm::atan2((double)candidate.offset.k, halo::libm::sqrt((double)h2));

    halo::objects::object_get_root_object_velocities(unit_index, &player_velocity, (real_vector3d *)0);
    halo::objects::object_get_root_object_velocities(candidate.object, &target_velocity, (real_vector3d *)0);
    dx = target_velocity.i - player_velocity.i;
    dy = target_velocity.j - player_velocity.j;
    dz = target_velocity.k - player_velocity.k;
    h = (real)halo::libm::sqrt((double)h2);
    out_yaw_pitch_rate[0] = (dy * candidate.offset.i - dx * candidate.offset.j) / h2;
    out_yaw_pitch_rate[1] = (dz * h - candidate.offset.k * ((dx * candidate.offset.i + dy * candidate.offset.j) / h)) /
                            (candidate.offset.k * candidate.offset.k + h2);

    return candidate.object;
}

/**
 * Implements the original `camera_observer_target_compare`.
 *
 * @address 0x45a4a0
 */
int32_t CameraObserver::target_compare(const observer_target_candidate *a, const observer_target_candidate *b)
{
    if (b->weight_primary < a->weight_primary) {
        return -1;
    }
    if (b->weight_primary <= a->weight_primary) {
        if (b->weight_secondary < a->weight_secondary) {
            return -1;
        }
        if (b->weight_secondary <= a->weight_secondary) {
            if (a->distance < b->distance) {
                return -1;
            }
            if (a->distance <= b->distance) {
                if (a->angle < b->angle) {
                    return -1;
                }
                if (a->angle <= b->angle) {
                    return (int32_t)((a->object & halo::k_datum_slot_mask) - (b->object & halo::k_datum_slot_mask));
                }
            }
        }
    }
    return 1;
}

/**
 * Computes the closest point on `object`'s look ray to `reference_position`, then the direction,
 * distance and angle (against `facing`) from `reference_position` to that point. Returns 0 when
 * `object` fails camera_observer_target_is_valid's line-of-sight check.
 *
 * @address 0x459cc0
 */
uint32_t CameraObserver::target_direction(real_point3d *candidate_point, real_vector3d *facing, real_point3d *reference_position, datum_index object, datum_index exclude_object, real_vector3d *out_direction, real *out_distance, real *out_angle)
{
    real dot;

    halo::game::vector3d_closest_point_on_segment(object, facing, reference_position, candidate_point);
    if (CameraObserver::target_is_valid(exclude_object, reference_position, candidate_point, object) != 0) {
        out_direction->i = candidate_point->x - reference_position->x;
        out_direction->j = candidate_point->y - reference_position->y;
        out_direction->k = candidate_point->z - reference_position->z;
        *out_distance = halo::math::vector3d_normalize_with_length(*out_direction);
        if (*out_distance != 0.0f) {
            dot = facing->i * out_direction->i + facing->j * out_direction->j + facing->k * out_direction->k;
            if (dot < -1.0f) {
                dot = -1.0f;
            } else if (1.0f < dot) {
                dot = 1.0f;
            }
            *out_angle = (real)halo::libm::acos((double)dot);
            return 1;
        }
    }
    return 0;
}

/**
 * Line-of-sight test for one observer candidate. Walks `exclude_object` (the observer's own
 * unit) up its parent chain to the root, then casts from `observer_position` to
 * `target_position` ignoring that root. An unobstructed cast accepts the candidate; an
 * obstructed one is still accepted when the blocker and the candidate share a root object
 * (result code 3, i.e. the candidate occluded itself or its own vehicle).
 *
 * @address 0x459dd0
 */
char CameraObserver::target_is_valid(datum_index exclude_object, real_point3d *observer_position, real_point3d *target_position, datum_index target_object)
{
    datum_index root;
    datum_index current;
    real_vector3d delta;
    uint8_t scratch[0x50];

    root = k_datum_index_none;
    current = exclude_object;
    if (current != k_datum_index_none) {
        do {
            root = current;
            current = halo::game::object_at(current)->parent_object;
        } while (current != k_datum_index_none);
    }

    delta.i = target_position->x - observer_position->x;
    delta.j = target_position->y - observer_position->y;
    delta.k = target_position->z - observer_position->z;

    if (halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::double_sided | halo::collision_test_flag::ignore_invisible | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::object_vehicle | halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine), observer_position, &delta, root, (collision_result *)scratch) == 0) {
        return 1;
    }
    if (*(int16_t *)scratch != 3) {
        return 0;
    }
    if (halo::objects::object_get_root_object_index(*(datum_index *)(scratch + 0x38)) ==
        halo::objects::object_get_root_object_index(target_object)) {
        return 1;
    }
    return 0;
}

/**
 * Fills in one observer_target_candidate: the closest point on the target's look ray to
 * `reference_position` (via vector3d_closest_point_on_segment), the offset/direction/distance
 * from that point, the angle between that direction and `facing`, and the two falloff-weighted scores. Returns 1 when either weight is
 * positive.
 * REWRITTEN 2026-09-27 (static loop) from objdump 0x459b10..0x459cb2: EAX is the FACING vector (EBX: the
 * closest-point aux vector and the angle's dot product), the stack carries (cone, reference_position). The draft
 * merged facing and cone into one pointer and took the dot product against the cone's four falloff floats.
 *
 * @address 0x459b10
 */
uint32_t CameraObserver::target_score(real_vector3d *facing, observer_target_cone *cone, datum_index target, observer_target_candidate *out, real_point3d *reference_position)
{
    real dot;
    real angle;
    object *target_object;
    Unit *target_tag;

    out->object = target;

    halo::game::vector3d_closest_point_on_segment(target, facing, reference_position, &out->point);

    out->offset.i = out->point.x - reference_position->x;
    out->offset.j = out->point.y - reference_position->y;
    out->offset.k = out->point.z - reference_position->z;
    out->direction = out->offset;
    out->distance = halo::math::vector3d_normalize_with_length(out->direction);

    dot = out->direction.k * facing->k + out->direction.j * facing->j + out->direction.i * facing->i;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }
    angle = (real)halo::libm::acos((double)dot);
    out->angle = angle;

    if (cone == (observer_target_cone *)0) {
        out->weight_primary = 0.0f;
        out->weight_secondary = 0.0f;
    } else {
        out->weight_primary = halo::game::distance_falloff_fraction(angle, cone->angle_a) *
                               halo::game::distance_falloff_fraction(out->distance, cone->distance_a);
        out->weight_secondary = halo::game::distance_falloff_fraction(angle, cone->angle_b) *
                                 halo::game::distance_falloff_fraction(out->distance, cone->distance_b);
        if (0.0f < out->weight_secondary) {
            target_object = halo::game::object_at(target);
            target_tag = (Unit *)halo::game::tag_data_at(target_object->definition_tag);
            if (test_flag(target_tag->unit_flags, halo::tags::unit_tag_flag::inconsequential)) {
                out->weight_secondary = out->weight_secondary *
                    ((GlobalsPlayerControl *)global_globals->player_control.pointer)
                        ->inconsequential_target_scale;

            }
        }
    }

    return (out->weight_primary > 0.0f || out->weight_secondary > 0.0f) ? 1 : 0;
}

/**
 * blam-cc: ESI -> out, AX -> local_player_index
 *
 * @address 0x472020
 */
void SpectateCamera::spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    datum_index unit = look->unit;

    out->marker_offset = 0;
    out->unit = unit;
    out->seat_index = -1;

    if (unit != k_datum_index_none) {
        object *u = halo::game::object_at(unit);

        halo::units::unit_get_camera_position(unit, &out->position);

        if (u->parent_object != k_datum_index_none) {
            object *parent = halo::objects::object_try_and_get(u->parent_object, _object_mask_vehicle);

            if (parent != 0) {
                Unit *vehicle_tag = (Unit *)halo::game::tag_data_at(parent->definition_tag);
                int16_t seat_index = halo::game::unit_data_of(u)->vehicle_seat_index;
                UnitSeat *seat = (UnitSeat *)vehicle_tag->seats.pointer + seat_index;

                out->marker_offset = (uint8_t *)&seat->camera_marker_name;
                out->unit = u->parent_object;
                out->seat_index = seat_index;
                u = halo::game::object_at(u->parent_object);
            }
        }
        if (out->seat_index == -1) {
            Unit *unit_tag = (Unit *)halo::game::tag_data_at(u->definition_tag);

            out->marker_offset = (uint8_t *)&unit_tag->camera_marker_name;
        }
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::CameraObserver::collect_target_candidates; forwards to the C++ implementation.
 * register convention: all eleven of Ghidra's recognized parameters are genuine __cdecl stack
 * arguments (the caller pushes 11 dwords and cleans 0x2c); none of them is a register.
 * // blam-cc: stack -> the whole prototype below
 * blam-cc: stack -> cone, start_object, observer_position, facing, max_distance,
 *
 * @address 0x45a0e0
 */


/**
 * C entry point for halo::game::CameraObserver::find_best_target; forwards to the C++ implementation.
 * register convention: RE-DERIVED from
 * objdump -d -M intel --start-address=0x459a00 --stop-address=0x459b10 bin/halo.exe
 * and from the call site inside camera_observer_update (0x459472..0x459496). Ghidra reports
 * zero parameters for this function; in fact it takes FIVE stack arguments plus EBX:
 * mov edx,[esp+0xe10] / mov edi,[esp+0xe0c] / mov ecx,[esp+0xe18] / mov edx,[esp+0xe1c]
 * -> E0+0x10, E0+0x04, E0+0x0c, E0+0x08, and E0+0x14 (read at 0x459ad6 for the rep movsd
 * of 0xe dwords == one observer_target_candidate) = arg0..arg4
 * the caller pushes, in order, out / team / player->unit / facing / cone, so the stack
 * arguments are (cone, facing, exclude_object, team, out) -- the same five values the
 * previous version of this file guessed, but they are stack arguments, not registers.
 * ebx is NEVER written before use here (`mov edx,ebx` at 0x459a0c, `mov ecx,ebx` at
 * 0x459ab6, `push ebx` at 0x459a69) and the callee it reaches, 0x459dd0, dereferences it
 * as three floats, so EBX is a real_point3d * -- the observer position. The caller sets it
 * with `lea ebx,[esp+0x40]` immediately before the call, pointing at the position the
 * camera routine (0x446a90 / 0x447290) just produced.
 * // blam-cc: EBX -> observer_position, stack -> (cone, facing, exclude_object, team, out)
 * blam-cc: EBX -> observer_position, stack -> (cone, facing, exclude_object, team, out)
 *
 * @address 0x459a00
 */
char camera_observer_find_best_target(real_point3d *observer_position, observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team, observer_target_candidate *out)
{
    return halo::game::CameraObserver::find_best_target(observer_position, cone, facing, exclude_object, team, out);
}

/**
 * C entry point for halo::game::CameraObserver::generate_target_candidates; forwards to the C++ implementation.
 * register convention: cone pointer in EDI (unaff_EDI); everything else is a stack argument.
 * // blam-cc: EDI -> cone, stack -> (start_cluster, observer_position, facing,
 * //          exclude_object, team, capacity, out)
 * blam-cc: EDI -> cone, stack -> (start_cluster, observer_position, facing,
 *
 * @address 0x459f70
 */


/**
 * C entry point for halo::game::CameraObserver::get_target_angles; forwards to the C++ implementation.
 * register convention: local-player slot in AX (in_AX); out_weight_primary/out_weight_secondary
 * and the yaw/pitch and rate-of-change output pairs are the recognized stack parameters.
 * // blam-cc: in_AX -> local_player_slot, stack -> out_weight_primary, out_weight_secondary,
 * //          out_yaw_pitch, out_yaw_pitch_rate
 * blam-cc: AX -> local_player_slot, stack -> out_weight_primary, out_weight_secondary, out_yaw_pitch, out_yaw_pitch_rate
 *
 * @address 0x4596f0
 */
uint32_t camera_observer_get_target_angles(real *out_weight_primary, real *out_weight_secondary, real *out_yaw_pitch, real *out_yaw_pitch_rate, int16_t local_player_slot)
{
    return halo::game::CameraObserver::get_target_angles(out_weight_primary, out_weight_secondary, out_yaw_pitch, out_yaw_pitch_rate, local_player_slot);
}

/**
 * C entry point for halo::game::CameraObserver::target_compare; forwards to the C++ implementation.
 * register convention: both entries are the recognized stack parameters (param_1, param_2);
 * no register arguments (this is a plain qsort callback).
 * // blam-cc: stack -> a, b
 *
 * @address 0x45a4a0
 */
int32_t camera_observer_target_compare(const observer_target_candidate *a, const observer_target_candidate *b)
{
    return halo::game::CameraObserver::target_compare(a, b);
}

/**
 * C entry point for halo::game::CameraObserver::target_direction; forwards to the C++ implementation.
 * register convention: candidate/closest point in EAX (in_EAX), observer facing vector in ECX
 * (in_ECX), reference/observer position in ESI (unaff_ESI); object handle, an unused second
 * value, and the three output pointers are the recognized stack parameters (param_1..param_5).
 * // blam-cc: EAX -> candidate_point, ECX -> facing, ESI -> reference_position,
 * //          stack -> object, unused, out_direction, out_distance, out_angle
 * blam-cc: EAX -> candidate_point, ECX -> facing, ESI -> reference_position,
 *
 * @address 0x459cc0
 */
uint32_t camera_observer_target_direction(real_point3d *candidate_point, real_vector3d *facing, real_point3d *reference_position, datum_index object, datum_index exclude_object, real_vector3d *out_direction, real *out_distance, real *out_angle)
{
    return halo::game::CameraObserver::target_direction(candidate_point, facing, reference_position, object, exclude_object, out_direction, out_distance, out_angle);
}

/**
 * C entry point for halo::game::CameraObserver::target_is_valid; forwards to the C++ implementation.
 * register convention: EAX, ECX and EDI are all live on entry (EDI is inherited from the caller,
 * which is why Ghidra reports no parameters at all); the candidate handle is the single stack
 * parameter.
 * // blam-cc: EAX -> exclude_object, ECX -> observer_position, EDI -> target_position,
 * //          stack -> target_object
 * blam-cc: EAX -> exclude_object, ECX -> observer_position, EDI -> target_position,
 *
 * @address 0x459dd0
 */


/**
 * C entry point for halo::game::CameraObserver::target_score; forwards to the C++ implementation.
 * register convention: cone pointer in EAX (in_EAX, Ghidra's "param_1"), target object handle in
 * ECX (in_ECX), output candidate pointer in ESI (unaff_ESI); reference position is the
 * recognized stack parameter (param_2). A second stack slot the caller reserves is never
 * read by this function, so it is not a parameter here.
 * // blam-cc: EAX -> cone, ECX -> object, ESI -> out, stack -> reference_position
 * blam-cc: EAX -> facing, ECX -> object, ESI -> out, stack -> cone, reference_position
 *
 * @address 0x459b10
 */


/**
 * C entry point for halo::game::SpectateCamera::spectate_fp_camera_position; forwards to the C++ implementation.
 * register convention: local-player index in AX; the output struct pointer is Ghidra's
 * blam-cc: ESI -> out, AX -> local_player_index
 * blam-cc: ECX -> unit_index, EDI -> out
 * blam-cc: ESI -> out, AX -> local_player_index
 *
 * @address 0x472020
 */
void chimera__spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index)
{
    halo::game::SpectateCamera::spectate_fp_camera_position(out, local_player_index);
}

}
