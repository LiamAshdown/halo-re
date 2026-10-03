#include "halo/game/gamerest_player.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"


namespace halo::game {

/**
 * Applies ScenarioPlayerStartingProfile[starting_profile_index] to unit_handle: optionally (when
 * reset_stats is set) resets the unit's vitality and zeroes its accumulated shield/health
 * modifiers and both grenade counts, then gives it the profile's primary and secondary weapons
 * (deleting the created weapon object if unit_pickup_weapon reports it could not be equipped), and
 * finally adds the profile's health/shield modifiers and grenade counts into the unit's own.
 * No-op if unit_handle or starting_profile_index is the wildcard, or if the (revalidated) unit
 * has no controlling player.
 *
 * @address 0x473c50
 */
void LocalPlayerUnit::apply_starting_profile(int16_t starting_profile_index, uint8_t reset_stats)
{
    datum_index unit_handle = unit;
    object *obj;
    unit_data *unit;
    ScenarioPlayerStartingProfile *profile;
    datum_index weapon_object;
    int8_t *profile_grenade_counts;
    int32_t i;

    if (unit_handle == (datum_index)-1 || starting_profile_index == -1) {
        return;
    }

    obj = halo::objects::object_try_and_get(unit_handle, _object_mask_unit);
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    if (unit->controlling_player == (datum_index)-1) {
        return;
    }

    profile = (ScenarioPlayerStartingProfile *)((uint8_t *)halo::scenario::globals().scenario->player_starting_profile.pointer
                                                 + (uint32_t)(uint16_t)starting_profile_index * sizeof(ScenarioPlayerStartingProfile));

    if (reset_stats != 0) {
        halo::units::unit_drop_inventory_weapons_except_current(unit_handle);
        obj->shield_vitality = 0.0f;
        obj->body_vitality = 0.0f;
        unit->grenade_counts[0] = 0;
        unit->grenade_counts[1] = 0;
    }

    if (profile->primary_weapon.tag_id.index != 0xffff || profile->primary_weapon.tag_id.id != 0xffff) {
        weapon_object = halo::game::player_spawn_starting_profile_weapon(&profile->primary_weapon, unit_handle);
        if (weapon_object != (datum_index)-1) {
            if (halo::units::unit_pickup_weapon((int16_t)(reset_stats != 0), weapon_object, unit_handle) == 0) {
                halo::objects::object_delete(weapon_object);
            }
        }
    }

    if (profile->secondary_weapon.tag_id.index != 0xffff || profile->secondary_weapon.tag_id.id != 0xffff) {
        weapon_object = halo::game::player_spawn_starting_profile_weapon(&profile->secondary_weapon, unit_handle);
        if (weapon_object != (datum_index)-1) {
            if (halo::units::unit_pickup_weapon(0, weapon_object, unit_handle) == 0) {
                halo::objects::object_delete(weapon_object);
            }
        }
    }

    obj->shield_vitality = obj->shield_vitality + profile->starting_shield_modifier;
    obj->body_vitality = obj->body_vitality + profile->starting_health_modifier;

    profile_grenade_counts = &profile->starting_fragmentation_grenade_count;
    for (i = 0; i < 2; i = i + 1) {
        unit->grenade_counts[i] = unit->grenade_counts[i] + profile_grenade_counts[i];
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::LocalPlayerUnit::apply_starting_profile; forwards to the C++ implementation.
 * register convention: EAX -> starting_profile_index, ECX -> unit_handle, stack -> reset_stats.
 * // blam-cc: EAX -> starting_profile_index, ECX -> unit_handle
 * blam-cc: EAX -> unit_handle; UNSURE full behavior (resets vitality when reset_stats is set)
 * blam-cc: ESI -> weapon_tag, stack -> owner_unit_handle;
 * blam-cc: EAX -> starting_profile_index, ECX -> unit_handle, stack -> reset_stats
 *
 * @address 0x473c50
 */
void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle, uint8_t reset_stats)
{
    halo::game::LocalPlayerUnit(unit_handle).apply_starting_profile(starting_profile_index, reset_stats);
}

}
