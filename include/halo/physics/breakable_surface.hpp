#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::physics {

/**
 * Damage, reset and shattering of breakable collision surfaces.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct BreakableSurfaces {
    static void apply_damage(damage_data *damage, int32_t surface_index, int32_t collision_surface_index);
    static void damage_in_blast_radius(damage_data *damage);
    static void breakable_surface_shatter(uint16_t breakable_surface_index, damage_data *damage, int32_t collision_surface_index);
};

}
