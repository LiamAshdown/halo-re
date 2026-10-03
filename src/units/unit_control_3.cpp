#include "halo/objects/record_access.hpp"
#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &global_zero_vector3d_pointer = halo::link::ref<const real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

namespace halo::units {

/**
 * Applies a steering-deviation based damage/light-intensity effect while at least one of the unit's
 * ground-contact markers is active.
 *
 * @address 0x574f30
 */
void UnitView::update_steering_deviation_effects(real_vector3d *reference_direction, uint8_t *contact_points)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    uint8_t *fall_table = (uint8_t *)global_globals->falling_damage.pointer;
    int32_t impact_effect_tag = *(int32_t *)(fall_table + 0x48);

    if (impact_effect_tag == -1 && *(int32_t *)&((struct Vehicle *)tag)->crash_sound.tag_id == -1) {
        return;
    }

    {
        real_vector3d deviation;
        double length;

        deviation.i = obj->velocity.i - reference_direction->i;
        deviation.j = obj->velocity.j - reference_direction->j;
        deviation.k = obj->velocity.k - reference_direction->k;
        length = halo::libm::sqrt((double)(deviation.i * deviation.i + deviation.j * deviation.j +
                                deviation.k * deviation.k));

        if (length > 0.02) {
            uint8_t *physics_tag = halo::objects::tag_record_bytes(*(uint32_t *)&((Unit *)tag)->base.physics.tag_id);
            int32_t count = *(int32_t *)(physics_tag + 0x74);
            int32_t i = 0;

            while ((contact_points[i * 0x130] & 2) == 0) {
                i++;
                if (i >= count) {
                    return;
                }
            }

            {
                float fraction = (float)((length - 0.02) * 45.454544);
                float clamped = (fraction < 0.0f) ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);

                if (impact_effect_tag != -1) {
                    damage_data dd = {0};
                    dd.team_index = -1;
                    dd.responsible_player = k_datum_index_none;
                    dd.responsible_object = k_datum_index_none;
                    *(int16_t *)&dd.location_cluster_index = -1;
                    dd.epicentre = obj->bounding_center;
                    dd.direction = deviation;
                    dd.random_blend = clamped;
                    dd.multiplier = 1.0f;
                    dd.material_type = -1;
                    dd.damage_effect_tag = impact_effect_tag;
                    halo::objects::object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
                }

                if (*(int32_t *)&((struct Vehicle *)tag)->crash_sound.tag_id != -1) {
                    halo::sound::sound_start_at_object_marker(unit_index, (Point3D *)global_zero_vector3d_pointer, (Vector3D *)halo::math::globals().global_forward3d_pointer,
                    *(int32_t *)&((struct Vehicle *)tag)->crash_sound.tag_id, -1, clamped, 0);
                }
            }
        }
    }
}

}
