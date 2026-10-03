/**
 * @file src/scenario/scenario_location.cpp
 * Locations (structure bsp leaf plus cluster) and the water, fog and weather questions asked of them.
 * The original author notes and decompiles are in docs/original/scenario/.
 */

#include "halo/scenario/scenario.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern ScenarioStructureBSP *global_structure_bsp;
extern Scenario *global_scenario;
extern float sqrtf(float x);
extern void value_step_toward_target(float *value, float target, float max_step);
extern void render_lighting_step_vector3_toward(float *current, float *target, float max_delta);
extern scenario_game_globals *global_scenario_game_globals;
}

namespace halo::scenario {

void location_view::from_point(real_point3d *point)
{
    ScenarioStructureBSPLeaf *leaves;

    self->leaf_index = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, point);
    if (self->leaf_index == -1) {
        self->cluster_index = -1;
        return;
    }

    leaves = (ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer;
    self->cluster_index = (int16_t)leaves[self->leaf_index & 0x7fffffff].cluster;
}

int16_t location_view::fog_region(real_point3d *point)
{
    ScenarioStructureBSPCluster *cluster;
    int16_t fog;
    ScenarioStructureBSPFogPlane *plane;
    int16_t region;
    uint32_t fog_tag;
    float water_bias;

    if (self->cluster_index == -1) {
        return -1;
    }

    cluster = &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)
                  [self->cluster_index];
    fog = (int16_t)cluster->fog;
    if (fog == -1) {
        return -1;
    }
    if (fog >= 0) {
        return fog & k_cluster_fog_index_mask;
    }

    plane = &((ScenarioStructureBSPFogPlane *)global_structure_bsp->fog_planes.pointer)
                [fog & k_cluster_fog_index_mask];
    region = (int16_t)plane->front_region;

    water_bias = 0.0f;
    fog_tag = scenario_query::fog_region_resolve_tag(region);
    if (fog_tag != 0xffffffff) {
        Fog *fog_data = (Fog *)halo::cache::globals().tag_instances[fog_tag & 0xffff].data;
        if (fog_data->flags & k_fog_flag_is_water) {
            water_bias = fog_data->distance_to_water_plane;
        }
    }

    if (point != 0) {
        float distance = plane->plane.vector.k * point->z + plane->plane.vector.j * point->y +
                          plane->plane.vector.i * point->x - plane->plane.w + water_bias;
        if (!(distance < 0.0f)) {
            return -1;
        }
    }
    return region;
}

float location_view::water_surface_distance(real_point3d *point)
{
    ScenarioStructureBSPCluster *cluster;
    int16_t fog;
    ScenarioStructureBSPFogPlane *plane;
    int16_t region;
    uint32_t fog_tag;
    Fog *fog_data;

    if (self->cluster_index == -1) {
        return -3.4028235e+38f;
    }

    cluster = &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)
                  [self->cluster_index];
    fog = (int16_t)cluster->fog;
    if (fog == -1) {
        return -3.4028235e+38f;
    }

    plane = 0;
    if (fog < 0) {
        plane = &((ScenarioStructureBSPFogPlane *)global_structure_bsp->fog_planes.pointer)
                    [fog & k_cluster_fog_index_mask];
        region = (int16_t)plane->front_region;
    } else {
        region = fog & k_cluster_fog_index_mask;
    }

    fog_tag = scenario_query::fog_region_resolve_tag(region);
    if (fog_tag == 0xffffffff) {
        return -3.4028235e+38f;
    }
    fog_data = (Fog *)halo::cache::globals().tag_instances[fog_tag & 0xffff].data;
    if (!(fog_data->flags & k_fog_flag_is_water)) {
        return -3.4028235e+38f;
    }

    if (plane == 0) {
        return 3.4028235e+38f;
    }

    return -((plane->plane.vector.k * point->z + plane->plane.vector.j * point->y +
              plane->plane.vector.i * point->x - plane->plane.w) +
             fog_data->distance_to_water_plane);
}

uint8_t location_view::background_sound_is_deafening_to_ais()
{
    ScenarioStructureBSPCluster *clusters;
    ScenarioStructureBSPBackgroundSoundPalette *palette;
    int16_t background_sound_index;
    uint32_t sound_tag;
    SoundLooping *sound;

    clusters = (ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer;
    background_sound_index = (int16_t)clusters[self->cluster_index].background_sound;

    if (background_sound_index != -1 &&
        (int32_t)background_sound_index < (int32_t)global_structure_bsp->background_sound_palette.count) {
        palette = (ScenarioStructureBSPBackgroundSoundPalette *)
            global_structure_bsp->background_sound_palette.pointer;
        sound_tag = *(uint32_t *)&palette[background_sound_index].background_sound.tag_id;
        if (sound_tag != 0xffffffff) {
            sound = (SoundLooping *)halo::cache::globals().tag_instances[sound_tag & 0xffff].data;
            return (sound->flags & k_sound_looping_flag_deafening_to_ais) != 0;
        }
    }
    return 0;
}

uint8_t scenario_query::location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out)
{
    int16_t fog_region;
    int16_t weather;
    uint8_t is_water;

    weather = -1;
    fog_region = location_view(leaf).fog_region(point);
    if (fog_region == -1) {
        is_water = 0;
    } else {
        ScenarioStructureBSPFogRegion *region =
            &((ScenarioStructureBSPFogRegion *)global_structure_bsp->fog_regions.pointer)
                [fog_region];
        uint32_t fog_tag = scenario_query::fog_region_resolve_tag(fog_region);

        if (fog_tag == 0xffffffff) {
            is_water = 0;
        } else {
            Fog *fog_data = (Fog *)halo::cache::globals().tag_instances[fog_tag & 0xffff].data;
            is_water = (uint8_t)(fog_data->flags & k_fog_flag_is_water);
        }

        weather = (int16_t)region->weather_palette;
        if (weather != -1) {
            goto write_output;
        }
    }

    if (leaf->cluster_index != -1) {
        ScenarioStructureBSPCluster *cluster =
            &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)
                [leaf->cluster_index];
        weather = (int16_t)cluster->weather;
    }

write_output:
    if (weather_index_out != 0) {
        *weather_index_out = weather;
    }
    return is_water;
}

uint32_t scenario_query::fog_region_resolve_tag(int16_t fog_region)
{
    ScenarioStructureBSPFogRegion *region;
    int16_t palette_index;
    ScenarioStructureBSPFogPalette *palette;

    if (fog_region == -1) {
        return 0xffffffff;
    }
    region = &((ScenarioStructureBSPFogRegion *)global_structure_bsp->fog_regions.pointer)
                 [fog_region];
    palette_index = (int16_t)region->fog;
    if (palette_index == -1) {
        return 0xffffffff;
    }
    palette = &((ScenarioStructureBSPFogPalette *)global_structure_bsp->fog_palette.pointer)
                  [palette_index];
    if (*(uint32_t *)&palette->fog.tag_id == 0xffffffff) {
        return 0xffffffff;
    }
    return *(uint32_t *)&palette->fog.tag_id;
}

uint8_t scenario_query::cluster_visibility_test(int16_t row_cluster, int16_t column_cluster)
{
    int32_t row_words = ((int32_t)global_structure_bsp->clusters.count + 0x1f) >> 5;
    uint32_t *pvs = (uint32_t *)global_structure_bsp->cluster_data.pointer;
    int32_t word_index = row_words * (int32_t)row_cluster + (column_cluster >> 5);

    return (pvs[word_index] & (1u << (column_cluster & 0x1f))) != 0;
}

uint8_t scenario_query::trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point)
{
    ScenarioTriggerVolume *volume;
    scenario_trigger_volume_box *box;
    float local_z;

    volume = &((ScenarioTriggerVolume *)global_scenario->trigger_volumes.pointer)
                 [trigger_volume_index];
    box = (scenario_trigger_volume_box *)&volume->starting_corner;

    if (volume->type == scenariotriggervolumetype_fixed) {
        if (!(box->fixed.x_bounds[0] < point->x)) return 0;
        if (!(box->fixed.y_bounds[0] < point->y)) return 0;
        if (!(box->fixed.z_bounds[0] < point->z)) return 0;
        if (!(point->x < box->fixed.x_bounds[1])) return 0;
        if (!(point->y < box->fixed.y_bounds[1])) return 0;
        local_z = point->z;
    } else if (volume->type == scenariotriggervolumetype_rotational) {
        real_matrix4x3 matrix;
        real_point3d local;

        halo::math::matrix4x3_from_forward_up(*((real_vector3d *)&volume->rotation_vector_up),
            *((real_vector3d *)&volume->rotation_vector_forward), matrix);

        matrix.position.x = volume->starting_corner.x;
        matrix.position.y = volume->starting_corner.y;
        matrix.position.z = volume->starting_corner.z;

        halo::math::matrix4x3_inverse_transform_point(matrix, local, *point);

        if (!(0.0f < local.x)) return 0;
        if (!(0.0f < local.y)) return 0;
        if (!(0.0f < local.z)) return 0;
        if (!(local.x < box->rotational.extents.i)) return 0;
        if (!(local.y < box->rotational.extents.j)) return 0;
        local_z = local.z;
    } else {
        return 0;
    }

    if (!(local_z < box->fixed.z_bounds[1])) {
        return 0;
    }
    return 1;
}

void scenario_query::sky_fog_state_update(int16_t sky_index, int16_t local_player_index, real_point3d *camera_position, render_fog *out)
{
    uint32_t sky_tag;
    Sky *sky_data;
    scenario_sky_fog_state *state;
    scenario_sky_fog_state local_scratch;
    sky_fog_block *fog;
    float distance;
    float fog_screen_blend_target;

    sky_tag = 0xffffffff;
    if (sky_index == -1) {
        if (0 < (int32_t)global_scenario->skies.count) {
            sky_tag = *(uint32_t *)&((ScenarioSky *)global_scenario->skies.pointer)[0].sky.tag_id;
        }
    } else if (0 <= sky_index && (int32_t)sky_index < (int32_t)global_scenario->skies.count) {
        sky_tag = *(uint32_t *)&((ScenarioSky *)global_scenario->skies.pointer)[sky_index].sky.tag_id;
    }

    sky_data = (Sky *)0;
    if (sky_tag != 0xffffffff) {
        sky_data = (Sky *)halo::cache::globals().tag_instances[sky_tag & 0xffff].data;
    }

    if (local_player_index == -1) {
        state = &local_scratch;
    } else {
        state = &global_scenario_game_globals->sky_fog[local_player_index];
    }

    if (sky_data != (Sky *)0) {
        if (sky_index == -1) {
            fog = (sky_fog_block *)&sky_data->indoor_fog_color;

            sky_tag = 0xffffffff;
            if (0 < (int32_t)global_scenario->skies.count) {
                sky_tag = *(uint32_t *)&((ScenarioSky *)global_scenario->skies.pointer)[0].sky.tag_id;
            }
            sky_data = (Sky *)0;
            if (sky_tag != 0xffffffff) {
                sky_data = (Sky *)halo::cache::globals().tag_instances[sky_tag & 0xffff].data;
            }

            fog_screen_blend_target = 1.0f;
            if (*(uint32_t *)&sky_data->indoor_fog_screen.tag_id != 0xffffffff) {
                goto sky_fog_resolved;
            }
        } else {
            fog = (sky_fog_block *)&sky_data->outdoor_fog_color;
        }
        fog_screen_blend_target = 0.0f;
sky_fog_resolved:

        distance = sqrtf(
            (camera_position->x - state->camera_position.x) * (camera_position->x - state->camera_position.x) +
            (camera_position->z - state->camera_position.z) * (camera_position->z - state->camera_position.z) +
            (camera_position->y - state->camera_position.y) * (camera_position->y - state->camera_position.y));

        if (local_player_index == -1 || !(distance < 15.0f) || state->valid == 0 ||
            fog->opaque_distance == 0.0f || state->opaque_distance == 0.0f) {
            state->start_distance   = fog->start_distance;
            state->opaque_distance  = fog->opaque_distance;
            state->maximum_density  = fog->maximum_density;
            state->color            = fog->color;
            state->fog_screen_blend = fog_screen_blend_target;
            state->valid = 1;
        } else {
            value_step_toward_target(&state->start_distance, fog->start_distance, distance);
            value_step_toward_target(&state->opaque_distance, fog->opaque_distance, distance);
            distance = distance * 0.05f;
            value_step_toward_target(&state->maximum_density, fog->maximum_density, distance);
            render_lighting_step_vector3_toward((float *)&state->color, (float *)&fog->color, distance);
            value_step_toward_target(&state->fog_screen_blend, fog_screen_blend_target, distance);
        }

        state->camera_position.x = camera_position->x;
        state->camera_position.y = camera_position->y;
        state->camera_position.z = camera_position->z;
    }

    out->atmospheric_color = state->color;
    out->atmospheric_maximum_density = state->maximum_density;
    out->atmospheric_minimum_distance = state->start_distance;

    if (state->opaque_distance == 0.0f) {
        out->atmospheric_maximum_distance = 0.0f;
    } else {
        out->atmospheric_maximum_distance = state->start_distance + 0.0001f;
        if (out->atmospheric_maximum_distance < state->opaque_distance) {
            out->atmospheric_maximum_distance = state->opaque_distance;
        }
    }

    if (state->fog_screen_blend < 0.0f) {
        out->sky_fog_screen_blend = 0.0f;
    } else if (1.0f < state->fog_screen_blend) {
        out->sky_fog_screen_blend = 1.0f;
    } else {
        out->sky_fog_screen_blend = state->fog_screen_blend;
    }
}

}  // namespace halo::scenario
