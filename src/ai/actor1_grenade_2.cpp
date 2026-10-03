#include "halo/ai/actor_grenade.hpp"
#include "halo/math/api.hpp"
#include "halo/items/api.hpp"

namespace c_actor_compute_grenade_throw_vector {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern double sqrt(double x);
extern double fabs(double x);
extern uint8_t actor_validate_grenade_impact_point(datum_index actor_index, real_point3d *candidate_point);
extern uint32_t actor_solve_grenade_lob(datum_index actor_index, real_point3d *point);
}
}

extern "C" uint32_t actor_compute_grenade_throw_vector(datum_index actor_index, real_point3d *grenade_position, real_vector3d *out_vector);

/**
 * actor_compute_grenade_throw_vector: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_compute_grenade_throw_vector.c.txt.
 *
 * @address 0x410a60
 */
uint32_t halo::ai::grenade_ops::compute_grenade_throw_vector(real_point3d *grenade_position, real_vector3d *out_vector)
{
    using namespace c_actor_compute_grenade_throw_vector;
    datum_index actor_index = datum;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint32_t target_object = 0xffffffff;
    real_vector3d direction;
    float speed;

    if (*(datum_index *)&((struct actor *)a)->grenade_target_prop_index != k_datum_index_none) {
        uint8_t *p = (uint8_t *)prop_data->data + (*(datum_index *)&((struct actor *)a)->grenade_target_prop_index & 0xffff) * 0x138;
        int16_t kind = ((prop *)p)->state;

        if (kind >= 2 && kind <= 3) {
            target_object = ((prop *)p)->object_index;
        }
        if (kind < 0 || kind > 1) {
            real_point3d point = *(real_point3d *)&((prop *)p)->last_known_position.x;

            point.z += 0.2f;
            actor_validate_grenade_impact_point(actor_index, &point);
        }
    }
    actor_solve_grenade_lob(actor_index, grenade_position);

    direction = *(real_vector3d *)&((actor *)a)->grenade_unknown_6bc;
    if (((actor *)a)->active_unit_index == k_datum_index_none) {
        real length = (real)sqrt(direction.j * direction.j + direction.i * direction.i);

        if (!(fabs(length) < 9.999999747378752e-05)) {
            real inverse = 1.0f / length;
            real flat_i = direction.i * inverse;
            real flat_j = direction.j * inverse;
            real_vector3d *facing = (real_vector3d *)(a + 0x174);

            if (length > 0.0f && !(flat_j * facing->j + flat_i * facing->i >= 0.8660254f)) {

                real_vector3d turned = *facing;
                real side = flat_j * facing->i - flat_i * facing->j;
                real sign = side > 0.0f ? 1.0f : -1.0f;
                real horizontal;

                halo::math::vector3d_rotate_about_axis(turned, *halo::math::globals().global_up3d_pointer, sign * 0.5f, 0.8660254f);
                horizontal = (real)sqrt(direction.j * direction.j + direction.i * direction.i);
                direction.i = turned.i * horizontal;
                direction.j = turned.j * horizontal;
            }
        }
    }
    speed = ((actor *)a)->grenade_unknown_6c8;
    out_vector->i = direction.i * speed;
    out_vector->j = direction.j * speed;
    out_vector->k = speed * direction.k;
    return target_object;
}

extern "C" uint32_t actor_compute_grenade_throw_vector(datum_index actor_index, real_point3d *grenade_position, real_vector3d *out_vector)
{
    return halo::ai::grenade_ops(actor_index).compute_grenade_throw_vector(grenade_position, out_vector);
}

namespace c_actor_died_unit_grenade_count_mod {
extern "C" {

extern void actor_delete(datum_index actor_index, uint32_t flag);
extern void encounter_recompute_morale(datum_index encounter_index);
}
}

extern "C" void actor_died_unit_grenade_count_mod(object *unit_object, const uint8_t *actor_tag_data, datum_index weapon_object_index, datum_index actor_index, datum_index encounter_index);

/**
 * actor_died_unit_grenade_count_mod: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_died_unit_grenade_count_mod.c.txt.
 *
 * @address 0x428d35
 */
void halo::ai::grenade_ops::died_unit_grenade_count_mod(object *unit_object, const uint8_t *actor_tag_data, datum_index weapon_object_index, datum_index actor_index, datum_index encounter_index)
{
    using namespace c_actor_died_unit_grenade_count_mod;
    *(int16_t *)((uint8_t *)unit_object + 0x31e) = 0;

    if (weapon_object_index != (datum_index)k_datum_index_none) {
        float min_fraction = *(const float *)(actor_tag_data + 0x1d8);
        float max_fraction = *(const float *)(actor_tag_data + 0x1dc);

        if (min_fraction > 0.0f || max_fraction > 0.0f) {
            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
            halo::items::weapon_set_loaded_ammo_fraction(
                (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f * (max_fraction - min_fraction) + min_fraction);
        }

        {
            int16_t min_count = *(const int16_t *)(actor_tag_data + 0x1e0);
            int16_t max_count = *(const int16_t *)(actor_tag_data + 0x1e2);

            if (min_count > 0 || max_count > 0) {
                int16_t count;
                halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;

                count = (int16_t)((uint32_t)(((int32_t)(int16_t)(max_count + 1) - min_count) * (int32_t)(halo::math::globals().random_seed_global >> 0x10)) >> 0x10) + min_count;
                halo::items::weapon_set_ammo_counts(&count);
            }
        }
    }

    actor_delete(actor_index, 1);
    if (encounter_index != (datum_index)k_datum_index_none) {
        encounter_recompute_morale(encounter_index);
    }
}

extern "C" void actor_died_unit_grenade_count_mod(object *unit_object, const uint8_t *actor_tag_data, datum_index weapon_object_index, datum_index actor_index, datum_index encounter_index)
{
    halo::ai::grenade_ops::died_unit_grenade_count_mod(unit_object, actor_tag_data, weapon_object_index, actor_index, encounter_index);
}

