#include "halo/game/gamerest_camera.hpp"
#include "halo/game/records.hpp"
#include "projectiles.h"
#include "halo/core/datum.hpp"
#include "halo/units/unit.hpp"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace halo::game {

/**
 * REWRITTEN from objdump. EAX = the player; stack = (the projectile origin, the aim direction, rotated in place).
 *   Using the unit's weapon autoaim cone (0x459e80: EAX = unit_noop(unit), DX = its zoom level +0x320), the
 *   first-person camera position/direction (deterministic, or with the weapon offset in a seat) and the best
 *   target inside the cone (0x459a00), the aim is pulled toward the target and toward what the camera looks at:
 *   a 128-unit segment from the camera (pushed forward by its distance from the unit) finds that point. The
 *   aim is rotated toward (target dir * fraction + look dir * (1 - fraction)) by at most cone[4]. The target
 *   goes to player +0x40 and the tick to +0x44; the target is returned. The draft called six helpers without
 *   arguments.
 *
 * @address 0x4593b0
 */
uint32_t CameraObserver::update(datum_index player_index, real_point3d *observer_position, real_vector3d *fallback_facing)
{
    ::player *player = halo::game::player_at(player_index);
    datum_index unit = ((struct player *)player)->unit;
    datum_index target = (datum_index)k_datum_index_none;
    uint32_t aim_unit = halo::units::UnitView(unit).resolve_camera_object();
    uint8_t *aim_unit_obj = (uint8_t *)halo::game::object_at(aim_unit);
    real cone[5];

    if (halo::game::unit_get_current_weapon_autoaim_cone(aim_unit, (int16_t)((unit_object *)aim_unit_obj)->unit.zoom_level, cone)) {
        int16_t seat_state = 0;
        real_point3d camera_position;
        real_vector3d camera_direction;
        real_vector3d target_direction;
        real_vector3d look_direction;
        real_vector3d blend;
        real fraction = 0.0f;
        union {
            observer_target_candidate candidate;
            collision_result collision;
            uint8_t bytes[0x50];
        } record;

        uint8_t *unit_obj;
        real dx, dy, dz, distance;
        real_vector3d camera_forward;
        real_point3d probe_origin;
        real_vector3d probe_delta;

        unit = ((struct player *)player)->unit;
        if (halo::camera::camera_get_seat_camera_state(unit, &seat_state) == 0) {
            halo::camera::first_person_camera_deterministic((Point3D *)&camera_position, unit, (Vector3D *)&camera_direction);
        } else {
            halo::camera::first_person_camera_apply_weapon_offset(&camera_position, unit, &camera_direction);
        }

        target_direction = *fallback_facing;
        memset(&record, 0, sizeof(record));
        if (halo::game::camera_observer_find_best_target(&camera_position, (observer_target_cone *)cone, &camera_direction,
                ((struct player *)player)->unit, (int16_t)*(uint16_t *)&((struct player *)player)->team, &record.candidate)) {
            target_direction.i = record.candidate.point.x - observer_position->x;
            target_direction.j = record.candidate.point.y - observer_position->y;
            target_direction.k = record.candidate.point.z - observer_position->z;
            if (halo::math::vector3d_normalize_with_length(target_direction) == 0.0f) {
                target_direction = *fallback_facing;
            }
            fraction = record.candidate.weight_primary;
            target = record.candidate.object;
        }

        unit_obj = (uint8_t *)halo::game::object_at(aim_unit);
        dx = camera_position.x - ((object *)unit_obj)->position.x;
        dy = camera_position.y - ((object *)unit_obj)->position.y;
        dz = camera_position.z - ((object *)unit_obj)->position.z;
        distance = (real)halo::libm::sqrt((double)(dx * dx + dy * dy + dz * dz));
        camera_forward = camera_direction;
        halo::math::vector3d_normalize_with_length(camera_forward);
        probe_origin.x = camera_forward.i * distance + camera_position.x;
        probe_origin.y = camera_forward.j * distance + camera_position.y;
        probe_origin.z = camera_forward.k * distance + camera_position.z;
        probe_delta.i = camera_direction.i * 128.0f;
        probe_delta.j = camera_direction.j * 128.0f;
        probe_delta.k = camera_direction.k * 128.0f;
        halo::physics::collision_test_movement_segment(0x1000e9, &probe_origin, &probe_delta, ((struct player *)player)->unit, &record.collision);

        look_direction.i = record.collision.point.x - observer_position->x;
        look_direction.j = record.collision.point.y - observer_position->y;
        look_direction.k = record.collision.point.z - observer_position->z;
        if (halo::math::vector3d_normalize_with_length(look_direction) == 0.0f) {
            look_direction = *fallback_facing;
        }
        blend.i = look_direction.i * (1.0f - fraction) + target_direction.i * fraction;
        blend.j = look_direction.j * (1.0f - fraction) + target_direction.j * fraction;
        blend.k = look_direction.k * (1.0f - fraction) + target_direction.k * fraction;
        halo::math::vector3d_normalize(blend);
        halo::math::vector3d_rotate_toward(&blend, *fallback_facing, fallback_facing, (real)halo::libm::sin((double)cone[4]),
            (real)halo::libm::cos((double)cone[4]));
    }

    ((struct player *)player)->observer_target = target;
    ((struct player *)player)->observer_state = game_time->game_time;
    return (uint32_t)target;
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::CameraObserver::update; forwards to the C++ implementation.
 * register convention: player index in EAX (in_EAX); observer position and a fallback facing
 * direction are the two recognized stack parameters (param_1, param_2).
 * // blam-cc: in_EAX -> player_index, stack -> observer_position, fallback_facing
 *
 * @address 0x4593b0
 */
uint32_t camera_observer_update(datum_index player_index, real_point3d *observer_position, real_vector3d *fallback_facing)
{
    return halo::game::CameraObserver::update(player_index, observer_position, fallback_facing);
}

}
