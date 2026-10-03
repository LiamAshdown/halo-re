#include "halo/units/unit.hpp"

extern "C" {
extern double sqrt(double x);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern object * object_iterator_next(object_iterator *iterator);
}

namespace halo::units {

/**
 * Finds the nearest object of type 0 (biped) to a given reference object by scanning all objects and
 * comparing positions.
 *
 * Original register convention: unaff_EDI.
 *
 * @address 0x56bee0
 */
int32_t object_find_nearest_biped(int32_t reference_object_index)
{
    int32_t best_index = -1;
    float best_distance = 3.4028235e+38f;

    object_iterator iter = { _object_mask_biped, 0, 0, 0, 0xffffffff };
    object *obj = object_iterator_next(&iter);
    while (obj != (object *)0) {
        int32_t this_handle = (int32_t)iter.handle;
        if ((this_handle != reference_object_index) && ((obj->vitality_flags & _object_health_frozen_bit) == 0)) {
            float distance;
            if (reference_object_index == -1) {
                distance = 0.0f;
            } else {
                real_point3d a, b;
                object_get_position(&a, (uint32_t)this_handle);
                object_get_position(&b, (uint32_t)reference_object_index);
                float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
                distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
            }
            if (distance < best_distance) {
                best_index = this_handle;
                best_distance = distance;
            }
        }
        obj = object_iterator_next(&iter);
    }
    return best_index;
}

/**
 * Finds the next object (wrapping around) after a given one that has no active target references and isn't
 * marked for deletion.
 *
 * Original register convention: unaff_ESI.
 *
 * @address 0x56bdc0
 */
int32_t object_find_next_untargeted(int32_t starting_object_index)
{
    int32_t result = -1;
    if (starting_object_index == -1) {
        goto from_start;
    }

    {
        object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
        object *obj = object_iterator_next(&iter);
        while ((obj != (object *)0) && ((int32_t)iter.handle != starting_object_index)) {
            obj = object_iterator_next(&iter);
        }
        obj = object_iterator_next(&iter);
        while (obj != (object *)0) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if ((unit->actor_index == k_datum_index_none) && (unit->swarm_actor_index == k_datum_index_none) &&
                ((obj->vitality_flags & _object_health_frozen_bit) == 0)) {
                result = (int32_t)iter.handle;
                if (result != -1) {
                    return result;
                }
                break;
            }
            obj = object_iterator_next(&iter);
        }
    }

from_start:
    {
        object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
        object *obj = object_iterator_next(&iter);
        while (obj != (object *)0) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if ((unit->actor_index == k_datum_index_none) && (unit->swarm_actor_index == k_datum_index_none) &&
                ((obj->vitality_flags & _object_health_frozen_bit) == 0)) {
                return (int32_t)iter.handle;
            }
            obj = object_iterator_next(&iter);
        }
        return result;
    }
}

}
