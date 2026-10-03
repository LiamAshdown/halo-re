#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/x87.hpp"


namespace halo::units {

/**
 * Engine function biped_build_update_delta_unit_grenade_count_mod1.
 *
 * @address 0x55e9ff
 */
void halo::units::biped_build_update_delta_unit_grenade_count_mod1(uint32_t flags, object *object_base, float magnitude, float dir_x, float dir_y, float dir_z, char already_idle, uint8_t *state_out)
{
    unit_data *unit = (unit_data *)((uint8_t *)object_base + k_unit_data_offset);

    if ((flags & 0x4100) != 0) {
        double angle = halo::math::random_real_range(0.0, 6.2831855);
        dir_x = (float)halo::x87::fcos(angle);
        dir_y = (float)halo::x87::fsin(angle);
        dir_z = 0.0f;
    }
    object_base->angular_velocity.i += dir_x * magnitude;
    object_base->angular_velocity.j += dir_y * magnitude;
    object_base->angular_velocity.k += dir_z * magnitude;

    UnitView(0).rotate_basis_about_axis();

    {
        int8_t state = unit->animation_state;
        if (state == 0x27 || state == 0x28) {
            state_out[0] = 0x28;
        } else if (state == 0x14 || already_idle != 0) {
            state_out[0] = 0x14;
        }
    }
}

/**
 * Engine function biped_network_baseline_take.
 *
 * @address 0x55b3d0
 */
void BipedView::network_baseline_take()
{
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)halo::objects::object_try_and_get(object_index, 1);

    if (obj == 0) {
        return;
    }
    ((struct biped_object *)obj)->biped.network_update_sequence++;
    ((struct biped_object *)obj)->biped.network_shield_vitality = ((unit_object *)obj)->base.shield_vitality * 0.33333334f;
    *(uint32_t *)&((struct biped_object *)obj)->biped.network_body_vitality = *(uint32_t *)&((unit_object *)obj)->base.body_vitality;
    ((struct biped_object *)obj)->biped.unknown_526 = 1;
    ((struct biped_object *)obj)->biped.network_delta_sequence = 0;
    ((struct biped_object *)obj)->biped.network_shield_stunned = (uint8_t)(((unit_object *)obj)->base.shield_stun_ticks > 0);
    ((struct biped_object *)obj)->biped.network_grenade_counts = *(int16_t *)(obj + 0x31e);
}

}
