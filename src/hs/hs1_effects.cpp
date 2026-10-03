#include "halo/hs/hs1_effects.hpp"

extern "C" {
extern void *memset(void *dst, int32_t value, uint32_t size);
extern int32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern void damage_apply_area_effect(void *request, uint32_t param_2);
extern Scenario *global_scenario;
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern ScenarioStructureBSP *global_structure_bsp;
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void object_apply_damage(void *dd, uint32_t object_index, int16_t hit_node_index, int16_t hit_region_index, int16_t hit_material_index, uint32_t hit_plane);
extern double fcos(double x);
extern double fsin(double x);
extern void effect_new_with_color(uint32_t effect, uint32_t creator_object_index, void *velocity, int32_t marker_count, int32_t marker_names, real_point3d *position, real_vector3d *forward, float a_scale, float b_scale, int32_t color, int32_t tint_source, int32_t force_create);
extern void *global_origin3d_pointer;
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t maximum);
extern datum_index effect_new_on_object_with_node_table(datum_index creator_object_index, datum_index definition_index, datum_index object_index, uint16_t node_index, uint16_t marker_count, uint32_t marker_names, uint32_t marker_positions, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
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

    location = (ScenarioCutsceneFlag *)((uint8_t *)global_scenario->cutscene_flags.pointer +
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

    impulse = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&location->position);
    request.sound_impulse = impulse;
    if (impulse == -1) {
        request.sound_index = 0xffff;
        damage_apply_area_effect(&request, 0xffffffff);
        return;
    }
    request.sound_index = ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[impulse & 0x7fffffff].cluster;
    damage_apply_area_effect(&request, 0xffffffff);
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

        object_get_position((real_point3d *)&request.position, object_index);
        *(Point3D *)&request.direction = *(Point3D *)&request.position;

        impulse = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&request.position);
        request.sound_impulse = impulse;
        if (impulse == -1) {
            request.sound_index = 0xffff;
        } else {
            request.sound_index = ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[impulse & 0x7fffffff].cluster;
        }
        object_apply_damage(&request, object_index, -1, -1, -1, 0);
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

    location = (ScenarioCutsceneFlag *)((uint8_t *)global_scenario->cutscene_flags.pointer +
        location_index * 0x5c);
    forward.i = (float)(fcos((double)location->facing.yaw) *
        fcos((double)location->facing.pitch));
    forward.j = (float)(fsin((double)location->facing.yaw) *
        fcos((double)location->facing.pitch));
    forward.k = (float)fsin((double)location->facing.pitch);

    effect_new_with_color(effect, 0xffffffff, global_origin3d_pointer, 1, 0, (real_point3d *)&location->position, &forward,
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
    if ((int16_t)object_get_node_local_transform(object_index, marker_name, &marker, 1) == 0) {
        return;
    }
    effect_new_on_object_with_node_table(k_datum_index_none, effect, object_index, *(uint16_t *)&marker,
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
