#include "halo/game/constants.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/items/items.hpp"
#include "halo/models/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/items/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/items/records.hpp"
#include "halo/objects/record_access.hpp"

static auto &k_weapon_zoom_fov_maximum = halo::link::ref<real>(halo::items::vars().k_weapon_zoom_fov_maximum);
static auto &k_weapon_zoom_fov_minimum = halo::link::ref<real>(halo::items::vars().k_weapon_zoom_fov_minimum);
static auto &k_empty_string = halo::link::ref<char [1]>(halo::networking::vars().k_empty_string);
static auto &k_weapon_minimum_age_ticks = halo::link::ref<int32_t>(halo::items::vars().k_weapon_minimum_age_ticks);
static auto &weapon_bottomless_clip = halo::link::ref<uint8_t>(halo::items::vars().weapon_bottomless_clip);
static auto &global_zero_vector3d_pointer = halo::link::ref<const real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

namespace halo::items {

/**
 * Builds a HUD-facing summary of a weapon's heat/age and per-magazine reload state.
 *
 * @address 0x4c29d0
 */
void weapon_ref::build_hud_ammo_state(weapon_hud_ammo_state *out)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int16_t i;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    out->heat = wd->heat;
    out->age = wd->age;
    out->overheated = (uint8_t)(wd->flags & _weapon_overheated_bit);
    out->magazine_count = (int16_t)weapon_tag->magazines.count;

    for (i = 0; i < weapon_tag->magazines.count; i++) {
        weapon_magazine_state *magazine = &wd->magazines[i];
        WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + i;
        weapon_hud_magazine_state *out_magazine = &out->magazines[i];

        out_magazine->reloading = (magazine->state == _weapon_magazine_reloading ||
                                    magazine->state == _weapon_magazine_chambering) ? 1 : 0;
        out_magazine->idle = (magazine->state == _weapon_magazine_idle) ? 1 : 0;
        out_magazine->rounds_loaded = magazine->rounds_loaded;
        out_magazine->rounds_loaded_maximum = magazine_tag->rounds_loaded_maximum;
        out_magazine->rounds_unloaded = magazine->rounds_unloaded;
        out_magazine->rounds_reserved_maximum = magazine_tag->rounds_reserved_maximum;
    }
}

/**
 * Converts a magnification factor for one zoom level into a target field of view, falling back
 * to the unclamped base FOV when the magnification is trivial (1.0) or the resulting FOV would
 * fall outside the tag-independent [min, max] zoom FOV range.
 *
 * @address 0x4c2e50
 */
real weapon_ref::clamp_zoom_fov(int16_t zoom_level, real base_fov)
{
    datum_index item_index = datum;
    real magnification;
    real fov;

    magnification = halo::items::weapon_get_zoom_magnification(item_index, zoom_level);
    if (magnification == 1.0f) {
        return base_fov;
    }
    fov = base_fov / magnification;
    if (fov <= k_weapon_zoom_fov_minimum || fov >= k_weapon_zoom_fov_maximum) {
        return base_fov;
    }
    return fov;
}

/**
 * Forces a weapon back to idle unless its current state is one of the three that may persist
 * (7 = charged_primary, 8 = charged_secondary, 10 = put_away).
 *
 * @address 0x4c5630
 */
void weapon_ref::force_settled_state()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    int8_t state;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    state = wd->state;

    if (state < 7 || (state > 8 && state != 10)) {
        halo::items::weapon_set_state(item_index, _weapon_state_idle, 1);
    }
}

/**
 * Returns the frame count (category 0) or key_frame_index (category 1) of one of a weapon's
 * first-person animations, or 0 when the graph/index is not available. For weapon_type 1 and
 * category 0 with mode 0 or 2, substitutes a fixed override animation's frame count instead.
 *
 * @address 0x4c2f80
 */
int16_t weapon_ref::get_first_person_animation_time(int16_t animation_index, int16_t category, int16_t mode)
{
    datum_index item_index = datum;
    object *item_obj;
    Weapon *weapon_tag;
    datum_index graph_tag_id;
    int16_t result = 0;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    graph_tag_id = halo::objects::tag_handle(weapon_tag->first_person_animations);

    if (graph_tag_id != k_datum_index_none) {
        ModelAnimations *graph = (ModelAnimations *)halo::cache::globals().tag_instances[(uint16_t)graph_tag_id].data;

        if (graph->first_person_weapons.count != 0) {
            ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *fp_weapon =
                (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)graph->first_person_weapons.pointer;

            if (fp_weapon != 0 && animation_index >= 0 &&
                animation_index < fp_weapon->animations.count) {
                int16_t resolved = ((uint16_t *)fp_weapon->animations.pointer)[animation_index];

                if (resolved != -1) {
                    ModelAnimationsAnimation *animations =
                        (ModelAnimationsAnimation *)graph->animations.pointer;

                    if (category == 0) {
                        result = animations[resolved].frame_count;
                    } else if (category == 1) {
                        result = animations[resolved].key_frame_index;
                    }

                    if (category == 0 && weapon_tag->weapon_type == 1) {
                        int16_t override_index;

                        if (fp_weapon->animations.count < 0x18) {
                            override_index = -1;
                        } else {
                            override_index = ((uint16_t *)fp_weapon->animations.pointer)[0x17];
                        }
                        if (mode == 0 || mode == 2) {
                            result = animations[override_index].frame_count;
                        }
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Returns the weapon's tag-defined label string, or the empty string for an invalid item.
 *
 * @address 0x4c24d0
 */
char * weapon_ref::get_label()
{
    datum_index item_index = datum;
    object *item_obj;
    Weapon *weapon_tag;

    if (item_index == k_datum_index_none) {
        return k_empty_string;
    }

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    return weapon_tag->label.string;
}

/**
 * Advances a weapon's zoom level by one, or -1/0 once the top level is passed. Leaves the
 * level unchanged while the first magazine is mid-reload.
 *
 * @address 0x4c2cf0
 */
int32_t weapon_ref::get_next_zoom_level(int32_t current_level)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int16_t level;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (weapon_tag->magazines.count < 1 || wd->magazines[0].state != _weapon_magazine_reloading) {
        level = (int16_t)current_level;
        if (level >= 0 && level < weapon_tag->zoom_levels - 1) {
            return current_level + 1;
        }
        return (level != weapon_tag->zoom_levels - 1) ? 0 : -1;
    }
    return current_level;
}

/**
 * Interpolates a weapon's zoom magnification for one zoom level across its tag-defined range.
 *
 * @address 0x4c2d70
 */
real weapon_ref::get_zoom_magnification(int16_t zoom_level)
{
    datum_index item_index = datum;
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (zoom_level >= 0 && zoom_level < weapon_tag->zoom_levels) {
        real fraction;
        real range0;
        real range1;

        if (weapon_tag->zoom_levels < 2) {
            fraction = 0.0f;
        } else {
            fraction = (real)zoom_level / (real)(weapon_tag->zoom_levels - 1);
        }
        range0 = (weapon_tag->zoom_magnification_range[0] <= 0.0f) ? 1.0f : weapon_tag->zoom_magnification_range[0];
        range1 = (weapon_tag->zoom_magnification_range[1] <= 0.0f) ? 1.0f : weapon_tag->zoom_magnification_range[1];

        return (real)(halo::libm::pow((double)range1 / range0, fraction) * range0);
    }
    return 1.0f;
}

/**
 * Reports whether an item currently has any active trigger effect state, in-progress magazine
 * reload, or non-idle weapon state.
 *
 * @address 0x4c3070
 */
int32_t weapon_ref::has_active_state()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);

    if (wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 &&
        wd->magazines[0].state == 0 && wd->magazines[1].state == 0 && wd->state == 0) {
        return 0;
    }
    return 1;
}

/**
 * The weapon row's "is old enough" hook (object_type_definition +0x74). An object that has never
 * been stamped (object.network_update_tick == -1) always counts as old enough; otherwise it is old
 * enough once the game tick has advanced past the stamped tick plus this type's minimum age.
 *
 * @address 0x4c6290
 */
uint8_t weapon_ref::is_old_enough()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    int32_t stamp = obj->network_update_tick;

    if (stamp == -1) {
        return 1;
    }
    return stamp + k_weapon_minimum_age_ticks <= halo::game::globals().game_time->game_time;
}

/**
 * Reports whether an item's weapon (per the odd combination above) should be treated as out of
 * ammo/charge.
 *
 * @address 0x4c2c70
 */
uint8_t weapon_ref::is_out_of_ammo()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (wd->age < 1.0f) {
        if (halo::game::globals().current_engine == 0) return 1;
        if (weapon_tag->magazines.count < 1) return 1;
        {
            WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer;
            if (magazine_tag->rounds_loaded_maximum < 1) return 1;
        }
        if (wd->magazines[0].rounds_loaded != 0 || wd->magazines[0].rounds_unloaded != 0) return 1;
    }
    return 0;
}

/**
 * Reports whether a weapon's first magazine is currently mid-reload.
 *
 * @address 0x4c2ad0
 */
int32_t weapon_ref::is_reloading()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    return weapon_tag->magazines.count > 0 && wd->magazines[0].state == _weapon_magazine_reloading;
}

/**
 * Starts chambering a round: only when the magazine is idle or chamber-pending and every
 * trigger/weapon state is idle. Sets the magazine to "chambering" for chamber_time ticks.
 *
 * @address 0x4c3b00
 */
void weapon_ref::magazine_begin_chamber(int16_t magazine_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponMagazine *magazine_tag;
    weapon_magazine_state *magazine;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
    magazine = &wd->magazines[magazine_index];

    if ((magazine->state == 0 || magazine->state == _weapon_magazine_chamber_pending) &&
        wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 && wd->state == 0) {
        halo::items::weapon_set_state(item_index, magazine_index + 3, 0);
        halo::items::weapon_play_trigger_tag_effect(item_index, halo::objects::tag_handle(magazine_tag->chambering_effect), 0, 0);
        magazine->state = _weapon_magazine_chambering;
        magazine->state_ticks = (int16_t)(magazine_tag->chamber_time * halo::game::k_ticks_per_second_f);
    }
}

/**
 * Host-side continuation of a magazine reload: moves rounds_reloaded worth of ammunition from
 * reserve into the magazine, and either starts loading the next round or finishes the reload.
 *
 * @address 0x4c3900
 */
void weapon_ref::magazine_reload_tick(int16_t magazine_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    WeaponMagazine *magazine_tag;
    weapon_magazine_state *magazine;
    int16_t old_unloaded;
    int16_t new_loaded;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    id = halo::items::item_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
    magazine = &wd->magazines[magazine_index];

    if (magazine_tag->flags & 1) {
        magazine->rounds_loaded = 0;
    }

    old_unloaded = magazine->rounds_unloaded;
    new_loaded = (old_unloaded <= magazine_tag->rounds_reloaded) ? old_unloaded : magazine_tag->rounds_reloaded;
    new_loaded = magazine->rounds_loaded + new_loaded;
    if (new_loaded > magazine_tag->rounds_loaded_maximum) {
        new_loaded = magazine_tag->rounds_loaded_maximum;
    }

    {
        int skip = 0;
        if (halo::game::globals().current_engine == 0) {
            if (weapon_bottomless_clip != 0 || (id->flags & _item_held_by_player_bit) == 0) skip = 1;
        } else if (weapon_bottomless_clip != 0) {
            skip = 1;
        }
        if (!skip) {
            magazine->rounds_unloaded = (magazine->rounds_loaded - new_loaded) + old_unloaded;
        }
    }

    magazine->rounds_loaded = new_loaded;
    magazine->state = _weapon_magazine_chamber_pending;
    magazine->state_ticks = 0;

    if (magazine->rounds_unloaded > 0 && new_loaded < magazine_tag->rounds_loaded_maximum &&
        (magazine_tag->flags & 1) == 0 && (wd->control_flags & (_weapon_control_primary_trigger_bit | _weapon_control_secondary_trigger_bit | _weapon_control_not_current_bit)) == 0) {
        halo::items::weapon_trigger_begin_reload(item_index, magazine_index, 0);
        return;
    }
    if (item_obj->network_role == 0 && halo::networking::globals().game_mode == 2) {
        halo::items::weapon_notify_reload_step(item_index, magazine_index);
    }
    item_obj->flags = item_obj->flags | _object_changed_bit;
}

/**
 * Client-side prediction of one reload step: moves rounds_reloaded worth of ammunition from
 * reserve into the magazine (unless bottomless_clip is set) and, if there is still room and the
 * weapon isn't inhibited, chains into another reload step locally.
 *
 * @address 0x4c3a20
 */
void weapon_ref::magazine_reload_tick_predicted(int16_t magazine_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponMagazine *magazine_tag;
    weapon_magazine_state *magazine;
    int16_t old_unloaded;
    int16_t new_loaded;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
    magazine = &wd->magazines[magazine_index];

    if (magazine_tag->flags & 1) {
        magazine->rounds_loaded = 0;
    }

    old_unloaded = magazine->rounds_unloaded;
    new_loaded = (old_unloaded <= magazine_tag->rounds_reloaded) ? old_unloaded : magazine_tag->rounds_reloaded;
    new_loaded = magazine->rounds_loaded + new_loaded;
    if (new_loaded > magazine_tag->rounds_loaded_maximum) {
        new_loaded = magazine_tag->rounds_loaded_maximum;
    }

    if (weapon_bottomless_clip == 0) {
        magazine->rounds_unloaded = (magazine->rounds_loaded - new_loaded) + old_unloaded;
    }
    magazine->rounds_loaded = new_loaded;

    if (magazine->rounds_unloaded > 0 && new_loaded < magazine_tag->rounds_loaded_maximum &&
        (magazine_tag->flags & 1) == 0 && (wd->control_flags & (_weapon_control_primary_trigger_bit | _weapon_control_secondary_trigger_bit | _weapon_control_not_current_bit)) == 0) {
        magazine->state = _weapon_magazine_chamber_pending;
        magazine->state_ticks = 0;
        halo::items::weapon_trigger_begin_reload(item_index, magazine_index, 0);
    }
}

/**
 * Tests the tag-defined must_be_readied flag (bit 3) on an item's weapon flags.
 *
 * @address 0x4c2ea0
 */
uint32_t weapon_ref::must_be_readied()
{
    datum_index item_index = datum;
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    return (weapon_tag->weapon_flags >> 3) & 1;
}

/**
 * The weapon row's query_create hook (object_type_definition +0x28). Zeroes weapon_data.state,
 * resets overheat_effect_handle to -1, splits each tag magazine's rounds_total_initial between
 * loaded (up to rounds_loaded_maximum) and unloaded/reserved, resets every trigger's idle_ticks,
 * effect_handle and empty_ticks to their idle values, and clears the network replication bytes
 * when the game is networked. Always reports success.
 *
 * @address 0x4c1420
 */
uint8_t weapon_ref::create()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    Weapon *tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    weapon_data *wd = halo::items::weapon_data_of(obj);
    int16_t i;

    wd->state = 0;
    wd->overheat_effect_handle = (datum_index)k_datum_index_none;

    if (tag->magazines.count > 0) {
        WeaponMagazine *tag_magazine = (WeaponMagazine *)tag->magazines.pointer;

        for (i = 0; i < tag->magazines.count; i++) {
            int16_t loaded = tag_magazine[i].rounds_loaded_maximum;
            if (tag_magazine[i].rounds_total_initial < loaded) {
                loaded = tag_magazine[i].rounds_total_initial;
            }
            wd->magazines[i].rounds_loaded = loaded;
            wd->magazines[i].rounds_unloaded = tag_magazine[i].rounds_total_initial - loaded;
        }
    }

    if (tag->triggers.count > 0) {
        for (i = 0; i < tag->triggers.count; i++) {
            wd->triggers[i].effect_handle = (datum_index)k_datum_index_none;
            wd->triggers[i].idle_ticks = 0x7f;
            wd->triggers[i].empty_ticks = 0;
        }
    }

    if (halo::networking::globals().game_mode == 1 || halo::networking::globals().game_mode == 2) {
        wd->network_state_valid = 0;
        wd->network_baseline_index = 0;
        wd->network_sequence = 0;
        obj->network_state_009 = 0;
    }

    return 1;
}

/**
 * Initializes a newly-created weapon object's starting magazine counts, at-rest/acceleration
 * flags and resting height from its scenario placement record. Returns the same object index it
 * was given.
 *
 * @address 0x4c1350
 */
datum_index weapon_ref::new_from_placement(ScenarioWeapon *placement)
{
    datum_index weapon_object_index = datum;
    object *weapon_obj;
    Weapon *weapon_tag;
    weapon_data *wd;
    item_data *id;
    int16_t rounds;

    weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)weapon_object_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)weapon_obj->definition_tag].data;
    wd = halo::items::weapon_data_of(weapon_obj);
    id = halo::items::item_data_of(weapon_obj);

    if (weapon_tag->magazines.count > 0) {
        WeaponMagazine *magazine = (WeaponMagazine *)weapon_tag->magazines.pointer;

        rounds = placement->rounds_reserved;
        if (magazine->rounds_reserved_maximum < rounds) {
            rounds = magazine->rounds_reserved_maximum;
        }
        wd->magazines[0].rounds_unloaded = rounds;

        rounds = magazine->rounds_loaded_maximum;
        if (placement->rounds_loaded <= rounds) {
            rounds = placement->rounds_loaded;
        }
        wd->magazines[0].rounds_loaded = rounds;
    }

    if ((placement->flags & 1) == 0) {
        weapon_obj->flags = weapon_obj->flags & ~(uint32_t)_object_at_rest_bit;
    } else {
        weapon_obj->flags = weapon_obj->flags | _object_at_rest_bit;
    }
    weapon_obj->flags = weapon_obj->flags | 0x20000;

    if ((placement->flags & 4) == 0) {
        id->flags = id->flags | _item_does_not_accelerate_bit;
    } else {
        id->flags = id->flags & ~(uint32_t)_item_does_not_accelerate_bit;
    }

    if ((placement->flags & 1) == 0) {
        weapon_obj->position.z = weapon_obj->position.z + 0.05f;
    }

    return weapon_object_index;
}

/**
 * Plays whichever tag (sound or effect) is referenced by a tag id, at the item's owning object
 * (or its holder, when the item has no-collision and is attached). Returns the resulting effect
 * handle for an 'effe' tag; sound playback always reports -1.
 * REWRITTEN (from objdump 0x4c47d0..0x4c4894): the creator is the item's parent when that parent is a unit
 * (object_try_and_get mask 3), else -1; the effect or sound attaches to the item, or to its parent when the
 * item has no collision. The two stack arguments are raw float bits (a/b scale for an effect, the scale of a
 * sound). Effect: effect_new_on_object(EAX creator, ECX tag, stack: object, -1, a, b, 0, 0). Sound:
 * effect_try_and_get(EDX tag) (result unused) then sound_start_at_object_marker(ESI creator, ECX = the zero
 * point, EAX = the forward vector, stack: tag, -1, a, 0). Anything else returns -1.
 *
 * @address 0x4c47d0
 */
uint32_t weapon_ref::play_trigger_tag_effect(datum_index tag_id, real scale_a, real scale_b)
{
    datum_index item_index = datum;
    object *item_obj;
    tag_group group;
    datum_index attach_to = item_index;
    datum_index creator = k_datum_index_none;
    real a_scale = scale_a;
    real b_scale = scale_b;

    if (tag_id == k_datum_index_none) {
        return k_datum_index_none;
    }
    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    if ((item_obj->flags & _object_no_collision_bit) != 0 && item_obj->parent_object != k_datum_index_none) {
        attach_to = item_obj->parent_object;
    }
    if (item_obj->parent_object != k_datum_index_none &&
        halo::objects::object_try_and_get(item_obj->parent_object, _object_mask_unit) != 0) {
        creator = item_obj->parent_object;
    }
    group = halo::cache::globals().tag_instances[(uint16_t)tag_id].group_tag;
    if (group == 0x65666665) {
        return halo::effects::effect_new_on_object(creator, tag_id, attach_to, -1, a_scale, b_scale, 0, 0);
    }
    if (group == 0x736e6421) {
        halo::effects::effect_try_and_get(tag_id);
        halo::sound::sound_start_at_object_marker(creator, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)halo::math::globals().global_forward3d_pointer, tag_id, -1, a_scale, 0);
    }
    return k_datum_index_none;
}

/**
 * Reports whether an item's weapon currently prevents throwing a grenade: either the tag flag
 * is set, its state is one of the "busy" states (5..10), or the item handle is invalid.
 *
 * @address 0x4c2f30
 */
uint32_t weapon_ref::prevents_grenade_throwing()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int8_t state;

    if (item_index == k_datum_index_none) {
        return 1;
    }

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    state = (int8_t)wd->state;
    if (state > 4 && state < 11) {
        return 1;
    }
    return (weapon_tag->weapon_flags >> 6) & 1;
}

/**
 * Reports whether an item's weapon currently prevents a melee attack: either the tag flag is
 * set, its first trigger is charging/charged, or the item handle itself is invalid.
 *
 * @address 0x4c2ee0
 */
uint32_t weapon_ref::prevents_melee_attack()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int8_t effect_state;

    if (item_index == k_datum_index_none) {
        return 1;
    }

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    effect_state = wd->triggers[0].effect_state;
    if (effect_state == _weapon_trigger_effect_charging || effect_state == _weapon_trigger_effect_charged) {
        return 1;
    }
    return (weapon_tag->weapon_flags >> 9) & 1;
}

/**
 * Starts a weapon's "ready" state, kicks off its first-person ready animation/sound, and seeds
 * the action_ticks cooldown that keeps weapon_update from accepting triggers until it elapses.
 *
 * @address 0x4c2840
 */
void weapon_ref::ready()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    uint32_t action_handle;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    halo::items::weapon_reset_triggers(item_index);
    halo::items::weapon_set_state(item_index, _weapon_state_ready, 1);

    action_handle = halo::interface::local_player_index_for_weapon(item_index);
    halo::interface::first_person_weapon_process_action(action_handle, 0xc);
    if ((int16_t)action_handle == -1) {
        halo::interface::hud_play_pickup_notification(item_index, 0xc);
    }

    halo::items::weapon_play_trigger_tag_effect(item_index, halo::objects::tag_handle(weapon_tag->ready_effect), 0.0f, 0.0f);
    wd->action_ticks = halo::items::weapon_get_first_person_animation_time(item_index, 10, 0, -1);

    if (item_obj->network_role == 0) {
        item_obj->flags = item_obj->flags | _object_changed_bit;
    }
}

/**
 * Finishes an overcharged trigger's recovery by consuming (deleting) the item itself, since it
 * is presumably a single-use weapon/equipment charge.
 *
 * @address 0x4c4940
 */
void weapon_ref::reload_recovery_finish()
{
    datum_index item_index = datum;
    object *item_obj;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    halo::items::weapon_play_trigger_tag_effect(item_index,
        *(datum_index *)&((Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data)->overheat_detonation.tag_id, 0, 0);

    if (item_obj->network_role == 0) {
        halo::objects::object_delete_unparented(item_index);
    } else if (item_obj->network_role != 3) {
        return;
    }
    halo::objects::object_delete_recursive(item_index, 0);
}

/**
 * Resets every trigger's effect state to the "reset" sentinel and every magazine back to idle,
 * nudging along any magazine that was mid-reload so its animation isn't left stranded. Used by
 * weapon_ready, weapon_put_away and the network ammo-correction resync path.
 *
 * @address 0x4c4b50
 */
void weapon_ref::reset_triggers()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    int16_t i;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    for (i = 0; i < weapon_tag->triggers.count; i++) {
        wd->triggers[i].effect_state = _weapon_trigger_effect_reset;
        wd->triggers[i].effect_state_ticks = 0;
    }

    for (i = 0; i < weapon_tag->magazines.count; i++) {
        weapon_magazine_state *magazine = &wd->magazines[i];

        if (magazine->state == _weapon_magazine_reloading) {
            int16_t fresh_length = halo::items::weapon_get_first_person_animation_time(item_index, 7, 0, -1);
            if (magazine->state_ticks * 2 < fresh_length) {
                if (item_obj->network_role != 1) {
                    halo::items::weapon_magazine_reload_tick(item_index, i);
                }
            } else if (item_obj->network_role == 0) {
                halo::items::weapon_notify_reload_cancel(item_index, i);
            }
        }
        magazine->state = 0;
        magazine->state_ticks = 0;
    }
}

/**
 * Sets each magazine's reserve ammo count from a caller-supplied array, clamped to each
 * magazine's rounds_reserved_maximum, and re-clamps rounds_loaded so it never exceeds the new
 * reserve figure.
 *
 * @address 0x4c5820
 */
void weapon_ref::set_ammo_counts(int16_t *reserve_counts)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int16_t i;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    for (i = 0; i < weapon_tag->magazines.count; i++) {
        WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + i;
        weapon_magazine_state *magazine = &wd->magazines[i];
        int16_t reserve = magazine_tag->rounds_reserved_maximum;

        if (reserve_counts[i] < reserve) {
            reserve = reserve_counts[i];
        }
        magazine->rounds_unloaded = reserve;
        if (magazine->rounds_loaded <= reserve) {
            reserve = magazine->rounds_loaded;
        }
        magazine->rounds_loaded = reserve;
    }
}

/**
 * Refreshes a weapon's control_flags/primary_trigger pair, which unit_update rebuilds every
 * tick from its own unit_control_flags and the analog trigger.
 *
 * @address 0x4c2990
 */
void weapon_ref::set_control_flags(uint16_t control_flags, real primary_trigger)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);

    wd->control_flags = control_flags;
    wd->primary_trigger = halo::math::transition_function_evaluate(0, primary_trigger);
}

/**
 * Sets a weapon's ammo or battery level from a 0..1 fraction. A weapon with no magazines, or
 * whose first trigger has a positive age_generated_per_round, is treated as a battery weapon and
 * gets its age set to the complement of the fraction; otherwise the first magazine's loaded
 * count is set to round(rounds_loaded_maximum * fraction), and the reserve is adjusted by the
 * same delta so the total ammo held is unchanged.
 *
 * @address 0x4c58c0
 */
void weapon_ref::set_loaded_ammo_fraction(real fraction)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int32_t is_battery;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (weapon_tag->magazines.count == 0) {
        is_battery = 1;
    } else {
        is_battery = 0;
        {
            int16_t i;
            WeaponTrigger *triggers = (WeaponTrigger *)weapon_tag->triggers.pointer;
            for (i = 0; i < weapon_tag->triggers.count; i++) {
                if (triggers[i].age_generated_per_round > 0.0f) {
                    is_battery = 1;
                    break;
                }
            }
        }
    }

    if (fraction >= 0.0f) {
        if (fraction > 1.0f) fraction = 1.0f;
    } else {
        fraction = 0.0f;
    }

    if (!is_battery) {
        if (weapon_tag->magazines.count > 0) {
            WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer;
            int16_t new_loaded = (int16_t)(int32_t)halo::libm::floor((double)((real)magazine_tag->rounds_loaded_maximum * fraction) + 0.5);
            int16_t old_loaded = wd->magazines[0].rounds_loaded;

            wd->magazines[0].rounds_loaded = new_loaded;
            wd->magazines[0].rounds_unloaded = wd->magazines[0].rounds_unloaded + (new_loaded - old_loaded);
        }
        return;
    }
    wd->age = 1.0f - fraction;
}

/**
 * Writes a weapon's ready_timer directly; unit_update calls it when the holder Object tag has flag 0x800000
 * (integrated_light_cntrls_weapon).
 *
 * @address 0x4c2b20
 */
void weapon_ref::set_ready_timer(real value)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    wd->ready_timer = value;
}

/**
 * Transitions a weapon into a new state/animation mode. Refuses the transition (returns 0)
 * when not forced, the weapon is mid-fire (state 1 or 2), and the requested state would move it
 * backwards; otherwise selects the matching first-person animation permutation from the
 * weapon's animation graph (when the tag data for it exists) and notifies the holding unit.
 *
 * @address 0x4c5670
 */
int32_t weapon_ref::set_state(int16_t new_state, int8_t force)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    int16_t current_state;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);

    current_state = (int8_t)wd->state;
    if (force == 0 && current_state != 0) {
        if (current_state < 1) return 0;
        if (current_state > 2) return 0;
        if (new_state < current_state) return 0;
    }

    {
        Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
        datum_index graph_tag_id = halo::objects::tag_handle(weapon_tag->base.base.animation_graph);

        if (graph_tag_id != k_datum_index_none) {
            ModelAnimations *graph = (ModelAnimations *)halo::cache::globals().tag_instances[(uint16_t)graph_tag_id].data;
            if (graph->weapons.count != 0) {
                ModelAnimationsAnimationGraphWeaponAnimations *weapon_anims =
                    (ModelAnimationsAnimationGraphWeaponAnimations *)graph->weapons.pointer;
                if (weapon_anims != 0) {
                    int16_t animation_index = -1;
                    switch (new_state) {
                    case 0: animation_index = 0; break;
                    case 1: animation_index = 9; break;
                    case 2: animation_index = 10; break;
                    case 3: animation_index = 5; break;
                    case 4: animation_index = 6; break;
                    case 5: case 6: animation_index = 3; break;
                    case 7: case 8: animation_index = 8; break;
                    case 9: animation_index = 1; break;
                    case 10: animation_index = 2; break;
                    default: break;
                    }

                    int16_t animation = (animation_index >= 0 && animation_index < weapon_anims->animations.count)
                        ? (int16_t)((ModelAnimationsWeaponAnimation *)weapon_anims->animations.pointer)[animation_index].animation
                        : -1;

                    if (animation_index >= 0 && (animation != -1 || new_state == 0)) {
                        item_obj->animation_index = halo::models::animation_choose_random_permutation(graph_tag_id, animation, static_cast<animation_random_stream>(1));
                        item_obj->animation_frame = 0;
                        wd->state = (int8_t)new_state;
                    }
                }
            }
        }
    }
    {
        datum_index parent = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data->parent_object;
        datum_index unit_index = k_datum_index_none;

        if (parent != k_datum_index_none && halo::objects::object_try_and_get(parent, _object_mask_unit) != 0) {
            unit_index = parent;
        }
        if (halo::objects::object_try_and_get(unit_index, _object_mask_unit) != 0) {
            halo::units::unit_dispatch_seat_overlay_command(unit_index, new_state);
        }
    }
    return 1;
}

/**
 * While chambering, primes the corresponding trigger's ejection_port_recovery meter to 1.0 when
 * its tag trigger both has a positive ejection_port_recovery_time and the ejects_during_chamber
 * flag (bit 7 of the low flags byte).
 *
 * @address 0x4c5580
 */
void weapon_ref::set_state_indicator_flags()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *triggers;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    triggers = (WeaponTrigger *)weapon_tag->triggers.pointer;

    if (wd->state == _weapon_state_chamber_primary) {
        if (triggers[0].ejection_port_recovery_time > 0.0f && (int8_t)triggers[0].flags < 0) {
            wd->triggers[0].ejection_port_recovery = 1.0f;
        }
    } else if (wd->state == _weapon_state_chamber_secondary) {
        if (triggers[1].ejection_port_recovery_time > 0.0f && (int8_t)triggers[1].flags < 0) {
            wd->triggers[1].ejection_port_recovery = 1.0f;
        }
    }
}

/**
 * Member form of the original weapon_stop_object_effect: stop object effect.
 *
 * @address 0x4c48a0
 */
uint32_t weapon_ref::stop_object_effect(datum_index tag_id)
{
    datum_index item_index = datum;
    object *item_obj;

    if (tag_id == k_datum_index_none) {
        return k_datum_index_none;
    }

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    if ((item_obj->flags & _object_no_collision_bit) != 0 && item_obj->parent_object != k_datum_index_none) {
        item_index = item_obj->parent_object;
    }
    if (item_index != k_datum_index_none) {
        return halo::effects::effect_new_at_texture_coordinate(tag_id, item_index, -1, -1, -1);
    }
    return k_datum_index_none;
}

/**
 * Moves reserve ammunition from one item into another. If both items share the same weapon tag,
 * rounds are moved magazine-for-magazine out of the source's own reserve; otherwise the target's
 * magazine_objects list is searched for an entry naming the source's tag (a battery/energy-cell
 * style pickup), and if found the whole source object is consumed. Returns non-zero (with the
 * magazine count packed into the upper bytes, an artifact of the original code) once any
 * magazine has been processed, and reports the amount actually moved through *out_transferred.
 *
 * @address 0x4c2610
 */
uint32_t weapon_ref::transfer_ammunition(datum_index source_item_index, int16_t requesting_player_index, int16_t *out_transferred)
{
    datum_index target_item_index = datum;
    object *target_obj;
    Weapon *target_tag;
    object *source_obj;
    datum_index source_definition_tag;
    uint8_t any_transferred;
    int16_t magazine_index;
    uint32_t result;

    target_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)target_item_index].data;
    target_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)target_obj->definition_tag].data;
    source_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)source_item_index].data;
    source_definition_tag = source_obj->definition_tag;
    any_transferred = 0;
    result = 0;

    if (target_tag->magazines.count > 0) {
        for (magazine_index = 0; magazine_index < target_tag->magazines.count; magazine_index++) {
            WeaponMagazine *magazine_tag = (WeaponMagazine *)target_tag->magazines.pointer + magazine_index;
            weapon_data *target_wd = halo::items::weapon_data_of(target_obj);
            int16_t *target_rounds_unloaded = &target_wd->magazines[magazine_index].rounds_unloaded;
            int16_t moved = 0;

            if (*target_rounds_unloaded < magazine_tag->rounds_reserved_maximum) {
                int16_t space_available = magazine_tag->rounds_reserved_maximum - *target_rounds_unloaded;

                if (target_obj->definition_tag == source_definition_tag) {
                    weapon_data *source_wd = halo::items::weapon_data_of(source_obj);
                    int16_t *source_rounds_unloaded = &source_wd->magazines[magazine_index].rounds_unloaded;

                    moved = space_available;
                    if (*source_rounds_unloaded <= space_available) {
                        moved = *source_rounds_unloaded;
                    }
                    if (moved > 0) {
                        *source_rounds_unloaded = *source_rounds_unloaded - moved;
                        if (halo::objects::tag_handle(target_tag->pickup_sound) != k_datum_index_none &&
                            requesting_player_index != -1) {
                            halo::sound::sound_start_unspatialized(halo::objects::tag_handle(target_tag->pickup_sound), 1.0f);
                        }
                        if (*source_rounds_unloaded == 0) {
                            halo::objects::object_delete(source_item_index);
                        }
                        if (source_obj->network_role == 0 &&
                            source_wd->magazines[magazine_index].state == _weapon_magazine_reloading) {
                            halo::items::weapon_notify_ammo_pickup(source_item_index, magazine_index, moved);
                        }
                    }
                    any_transferred = 1;
                } else {
                    int16_t object_index;

                    for (object_index = 0; object_index < magazine_tag->magazine_objects.count; object_index++) {
                        WeaponMagazineObject *magazine_object =
                            (WeaponMagazineObject *)magazine_tag->magazine_objects.pointer + object_index;

                        if (halo::objects::tag_handle(magazine_object->equipment) == source_definition_tag) {
                            moved = space_available;
                            if (magazine_object->rounds <= space_available) {
                                moved = magazine_object->rounds;
                            }
                            if (moved > 0) {
                                int32_t source_role;

                                if (requesting_player_index != -1) {
                                    halo::items::equipment_definition_play_pickup_sound(
                                        halo::objects::tag_handle(magazine_object->equipment));
                                }
                                source_role = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)source_item_index].data->network_role;
                                if (source_role == 0) {
                                    halo::objects::object_delete_unparented(source_item_index);
                                }
                                if (source_role == 0 || source_role == 3) {
                                    halo::objects::object_delete_recursive(source_item_index, 0);
                                }
                                any_transferred = 1;
                                break;
                            }
                        }
                    }
                }
                *target_rounds_unloaded = *target_rounds_unloaded + moved;
                *out_transferred = moved;
            }
        }
        result = (uint32_t)any_transferred | ((uint32_t)(uint16_t)magazine_index << 8);
    }
    return result;
}

/**
 * Reports whether both triggers are idle and the weapon's own state is idle.
 *
 * @address 0x4c30c0
 */
int32_t weapon_ref::triggers_idle()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = halo::items::weapon_data_of(item_obj);

    if (wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 && wd->state == 0) {
        return 1;
    }
    return 0;
}

}

namespace halo::items {

void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out)
{
    halo::items::weapon_ref(item_index).build_hud_ammo_state(out);
}

real weapon_clamp_zoom_fov(datum_index item_index, int16_t zoom_level, real base_fov)
{
    return halo::items::weapon_ref(item_index).clamp_zoom_fov(zoom_level, base_fov);
}

void weapon_force_settled_state(datum_index item_index)
{
    halo::items::weapon_ref(item_index).force_settled_state();
}

int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index, int16_t category, int16_t mode)
{
    return halo::items::weapon_ref(item_index).get_first_person_animation_time(animation_index, category, mode);
}

char * weapon_get_label(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).get_label();
}

int32_t weapon_get_next_zoom_level(int32_t current_level, datum_index item_index)
{
    return halo::items::weapon_ref(item_index).get_next_zoom_level(current_level);
}

real weapon_get_zoom_magnification(datum_index item_index, int16_t zoom_level)
{
    return halo::items::weapon_ref(item_index).get_zoom_magnification(zoom_level);
}

int32_t weapon_has_active_state(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).has_active_state();
}

uint8_t weapon_is_old_enough(uint32_t object_index)
{
    return halo::items::weapon_ref(object_index).is_old_enough();
}

uint8_t weapon_is_out_of_ammo(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).is_out_of_ammo();
}

int32_t weapon_is_reloading(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).is_reloading();
}

void weapon_magazine_begin_chamber(datum_index item_index, int16_t magazine_index)
{
    halo::items::weapon_ref(item_index).magazine_begin_chamber(magazine_index);
}

void weapon_magazine_reload_tick(datum_index item_index, int16_t magazine_index)
{
    halo::items::weapon_ref(item_index).magazine_reload_tick(magazine_index);
}

void weapon_magazine_reload_tick_predicted(datum_index item_index, int16_t magazine_index)
{
    halo::items::weapon_ref(item_index).magazine_reload_tick_predicted(magazine_index);
}

uint32_t weapon_must_be_readied(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).must_be_readied();
}

uint8_t weapon_new(uint32_t object_index)
{
    return halo::items::weapon_ref(object_index).create();
}

datum_index weapon_new_from_placement(datum_index weapon_object_index, ScenarioWeapon *placement)
{
    return halo::items::weapon_ref(weapon_object_index).new_from_placement(placement);
}

uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a, real scale_b)
{
    return halo::items::weapon_ref(item_index).play_trigger_tag_effect(tag_id, scale_a, scale_b);
}

uint32_t weapon_prevents_grenade_throwing(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).prevents_grenade_throwing();
}

uint32_t weapon_prevents_melee_attack(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).prevents_melee_attack();
}

void weapon_ready(datum_index item_index)
{
    halo::items::weapon_ref(item_index).ready();
}

void weapon_reload_recovery_finish(datum_index item_index)
{
    halo::items::weapon_ref(item_index).reload_recovery_finish();
}

void weapon_reset_triggers(datum_index item_index)
{
    halo::items::weapon_ref(item_index).reset_triggers();
}

void weapon_set_ammo_counts(datum_index item_index, int16_t *reserve_counts)
{
    halo::items::weapon_ref(item_index).set_ammo_counts(reserve_counts);
}

void weapon_set_control_flags(datum_index item_index, uint16_t control_flags, real primary_trigger)
{
    halo::items::weapon_ref(item_index).set_control_flags(control_flags, primary_trigger);
}

void weapon_set_loaded_ammo_fraction(datum_index item_index, real fraction)
{
    halo::items::weapon_ref(item_index).set_loaded_ammo_fraction(fraction);
}

void weapon_set_ready_timer(datum_index item_index, real value)
{
    halo::items::weapon_ref(item_index).set_ready_timer(value);
}

int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force)
{
    return halo::items::weapon_ref(item_index).set_state(new_state, force);
}

void weapon_set_state_indicator_flags(datum_index item_index)
{
    halo::items::weapon_ref(item_index).set_state_indicator_flags();
}

uint32_t weapon_stop_object_effect(datum_index item_index, datum_index tag_id)
{
    return halo::items::weapon_ref(item_index).stop_object_effect(tag_id);
}

uint32_t weapon_transfer_ammunition(datum_index target_item_index, datum_index source_item_index, int16_t requesting_player_index, int16_t *out_transferred)
{
    return halo::items::weapon_ref(target_item_index).transfer_ammunition(source_item_index, requesting_player_index, out_transferred);
}

int32_t weapon_triggers_idle(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).triggers_idle();
}

}
