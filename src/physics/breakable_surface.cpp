/**
 * Damage, reset and shattering of breakable collision surfaces.
 */

#include "tags.h"
#include "halo/scenario/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"

#include "halo/physics/breakable_surface.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

extern "C" { extern breakable_surface_globals *breakable_surface_state; }
namespace halo::physics {

/**
 * argument of 0x500090; projectile_response and unit_melee_attack_scan both push it.
 *
 * @address 0x4ffde0
 */
void BreakableSurfaces::apply_damage(damage_data *damage, int32_t surface_index, int32_t collision_surface_index)
{
    int16_t index;
    float *extension;
    GlobalsMaterial *material;

    index = (int16_t)surface_index;
    if ((breakable_surface_state->initialized != 0) && (index != -1) &&
        (damage->damage_effect_tag != k_datum_index_none) && (damage->material_type != -1)) {
        extension = &breakable_surface_state->health[halo::scenario::globals().structure_bsp_index][index];
        if (0.0f < *extension) {
            material = halo::scenario::globals_material_get(damage->material_type);
            if ((material != 0) && (0.0f < material->maximum_vitality)) {
                DamageEffect *effect =
                    (DamageEffect *)halo::cache::globals().tag_instances[(uint16_t)damage->damage_effect_tag].data;

                float *material_damage_modifiers = &effect->dirt;
                float random_amount = halo::math::random_real_range(effect->damage_upper_bound[0],
                                                          effect->damage_upper_bound[1]);
                float blended_amount = (random_amount - effect->damage_lower_bound) *
                                           damage->random_blend +
                                       effect->damage_lower_bound;
                float new_extension = *extension -
                    (blended_amount * material_damage_modifiers[damage->material_type]) /
                        material->maximum_vitality;
                *extension = new_extension;

                if (new_extension <= 0.0f) {
                    breakable_surface_state->active[halo::scenario::globals().structure_bsp_index][index >> 5] &=
                        ~(1u << (index & 0x1f));

                    halo::physics::breakable_surface_shatter((uint16_t)index, damage, collision_surface_index);
                }
            }
        }
    }
}

}

namespace halo::physics {

/**
 * Implements breakable surface damage in blast radius.
 *
 * @address 0x4fff20
 */
void BreakableSurfaces::damage_in_blast_radius(damage_data *damage)
{
    DamageEffect *effect =
        (DamageEffect *)halo::cache::globals().tag_instances[(uint16_t)damage->damage_effect_tag].data;

    if ((breakable_surface_state->initialized != 0) &&
        ((effect->damage_upper_bound[0] != 0.0f) || (effect->damage_upper_bound[1] != 0.0f))) {
        float outer_radius = effect->radius[1];
        int16_t surface_index = 0;
        int32_t index = 0;

        if (0 < halo::scenario::globals().structure_bsp->breakable_surfaces.count) {
            do {
                if ((surface_index == -1) ||
                    ((breakable_surface_state->active[halo::scenario::globals().structure_bsp_index][index >> 5] &
                      (1u << (index & 0x1f))) != 0)) {
                    ScenarioStructureBSPBreakableSurface *surface =
                        &((ScenarioStructureBSPBreakableSurface *)
                              halo::scenario::globals().structure_bsp->breakable_surfaces.pointer)[index];
                    float combined_radius = outer_radius + surface->radius;
                    float dy = damage->origin.y - surface->centroid.y;
                    float dz = damage->origin.z - surface->centroid.z;
                    float dx = damage->origin.x - surface->centroid.x;
                    if (dy * dy + dz * dz + dx * dx <= combined_radius * combined_radius) {
                        breakable_surface_state->health[halo::scenario::globals().structure_bsp_index][index] = 0.0f;
                        breakable_surface_state->active[halo::scenario::globals().structure_bsp_index][index >> 5] &=
                            ~(1u << (index & 0x1f));
                        halo::physics::breakable_surface_shatter((uint16_t)surface_index, damage,
                                                            surface->collision_surface_index);
                    }
                }
                surface_index = surface_index + 1;
                index = (int32_t)surface_index;
            } while (index < halo::scenario::globals().structure_bsp->breakable_surfaces.count);
        }
    }
}

}
