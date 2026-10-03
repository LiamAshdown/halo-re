#include "halo/hs/hs1_effects.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern void *memset(void *dst, int32_t value, uint32_t size);
extern double fcos(double x);
extern double fsin(double x);
extern void *global_origin3d_pointer;
}

namespace halo::hs {

/**
 * , scenario.h (formerly global_matg_multiplayer) hs_damage_request: defined in types/hs.h (foreign-module
 * slice; was a local TYPES-GAP copy) Builds and applies a damage request at
 * Scenario::cutscene_flags[location_index]'s position (used for both position and direction). Resolves a
 * sound-impulse table entry via bsp3d_node_find_leaf and global_matg_multiplayer+0xe4 (stride 0x10, uint16
 * at +8) when available.
 *
 * @address 0x488960
 */
void ScriptEffects::damage_apply_at_location(int16_t location_index, uint32_t damage_effect)
{
    ScenarioCutsceneFlag *location;
    hs_damage_request request;
    int32_t impulse;

    location = (ScenarioCutsceneFlag *)((uint8_t *)halo::scenario::globals().scenario->cutscene_flags.pointer +
        location_index * 0x5c);

    memset(&request, 0, sizeof(request));
    request.damage_effect = damage_effect;
    request.team_index = 0xffff;
    request.causer = 0xffffffff;
    request.attacker = 0xffffffff;
    request.sound_index = 0xffff;
    request.scale_a = 1.0f;
    request.scale_b = 1.0f;
    request.material_type = 0xffff;
    *(Point3D *)&request.position = location->position;
    *(Point3D *)&request.direction = location->position;

    impulse = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, (real_point3d *)&location->position);
    request.sound_impulse = impulse;
    if (impulse == -1) {
        request.sound_index = 0xffff;
        halo::objects::damage_apply_area_effect((damage_data *)&request);
        return;
    }
    request.sound_index = ((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)[impulse & 0x7fffffff].cluster;
    halo::objects::damage_apply_area_effect((damage_data *)&request);
}

/**
 * Applies `damage_effect` to `object_index` at the object's current position, resolving a sound-impulse
 * table entry the same way damage_apply_at_location does.
 *
 * @address 0x488a40
 */
void ScriptEffects::damage_apply_with_sound(datum_index object_index, uint32_t damage_effect)
{
    hs_damage_request request;
    int32_t impulse;

    if (object_index != k_datum_index_none) {
        memset(&request, 0, sizeof(request));
        request.damage_effect = damage_effect;
        request.team_index = 0xffff;
        request.causer = 0xffffffff;
        request.attacker = 0xffffffff;
        request.sound_index = 0xffff;
        request.scale_a = 1.0f;
        request.scale_b = 1.0f;
        request.material_type = 0xffff;

        halo::objects::object_get_position((real_point3d *)&request.position, object_index);
        *(Point3D *)&request.direction = *(Point3D *)&request.position;

        impulse = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, (real_point3d *)&request.position);
        request.sound_impulse = impulse;
        if (impulse == -1) {
            request.sound_index = 0xffff;
        } else {
            request.sound_index = ((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)[impulse & 0x7fffffff].cluster;
        }
        halo::objects::object_apply_damage((damage_data *)&request, object_index, -1, -1, -1, 0);
    }
}

/**
 * Spawns `effect` at the position of the scenario cutscene flag `location_index`.
 *
 * @address 0x488870
 */
void ScriptEffects::effect_spawn_at_location(int16_t location_index, uint32_t effect)
{
    ScenarioCutsceneFlag *location;
    real_vector3d forward;

    location = (ScenarioCutsceneFlag *)((uint8_t *)halo::scenario::globals().scenario->cutscene_flags.pointer +
        location_index * 0x5c);
    forward.i = (float)(fcos((double)location->facing.yaw) *
        fcos((double)location->facing.pitch));
    forward.j = (float)(fsin((double)location->facing.yaw) *
        fcos((double)location->facing.pitch));
    forward.k = (float)fsin((double)location->facing.pitch);

    halo::effects::effect_new_with_color(effect, 0xffffffff, (const real_vector3d *)global_origin3d_pointer, 1, 0, (real_point3d *)&location->position, (uint32_t)&forward,
        1.0f, 1.0f, 0, 0, 1);
}

/**
 * Spawns `effect` on the named marker of `object_index`.
 *
 * @address 0x4888f0
 */
void ScriptEffects::effect_spawn_on_marker(datum_index object_index, datum_index effect, char *marker_name)
{
    object_marker marker;

    if (effect == k_datum_index_none || object_index == k_datum_index_none) {
        return;
    }
    if ((int16_t)halo::objects::object_get_node_local_transform(object_index, marker_name, &marker, 1) == 0) {
        return;
    }
    halo::effects::effect_new_on_object_with_node_table(k_datum_index_none, effect, object_index, *(uint16_t *)&marker,
        1, (uint32_t)&marker_name, (uint32_t)((uint8_t *)&marker + 0x60), (uint32_t)((uint8_t *)&marker + 0x3c),
        1.0f, 1.0f, 0, 0);
}

}

extern "C" {

void hs_damage_apply_at_location(int16_t location_index, uint32_t damage_effect)
{
    halo::hs::ScriptEffects::damage_apply_at_location(location_index, damage_effect);
}

void hs_damage_apply_with_sound(datum_index object_index, uint32_t damage_effect)
{
    halo::hs::ScriptEffects::damage_apply_with_sound(object_index, damage_effect);
}

void hs_effect_spawn_at_location(int16_t location_index, uint32_t effect)
{
    halo::hs::ScriptEffects::effect_spawn_at_location(location_index, effect);
}

void hs_effect_spawn_on_marker(datum_index object_index, datum_index effect, char *marker_name)
{
    halo::hs::ScriptEffects::effect_spawn_on_marker(object_index, effect, marker_name);
}

}
