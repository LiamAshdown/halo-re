#include "halo/units/unit.hpp"
#include "halo/models/api.hpp"
#include "halo/math/api.hpp"
#include "halo/sound/api.hpp"

extern "C" {
extern void *global_zero_vector3d_pointer;
}

namespace halo::units {

/**
 * Advances *state by one frame (animation_state_advance) and, if that frame starts a sound, plays it attached
 * to object_index's marker (node 0) via sound_start_at_object_marker, at the fixed creation origin/forward
 * and full volume.
 *
 * @address 0x56ec10
 */
uint16_t unit_reset_light_effect(animation_state *state, uint32_t animation_graph_tag_index, datum_index object_index)
{
    int32_t sound_tag_id;
    uint16_t result = halo::models::animation_state_advance(animation_graph_tag_index, state, &sound_tag_id, _animation_random_global);

    if (sound_tag_id != -1) {
        halo::sound::sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)halo::math::globals().global_forward3d_pointer, (datum_index)sound_tag_id, 0, 1.0f, 0);
    }
    return result;
}

}
