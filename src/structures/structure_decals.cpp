/**
 * @file src/structures/structure_decals.cpp
 * Runtime decals attached to structure bsp clusters.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"
#include "halo/math/api.hpp"

extern "C" {
extern ScenarioStructureBSP *global_structure_bsp;
extern uint8_t *runtime_decals_suppressed;
extern Scenario *global_scenario;
extern uint8_t decals_for_all_responses;
extern uint8_t decals_enabled;
extern tag_instance *tag_instances;
extern double cos(double x);
extern double sin(double x);
extern void decal_evict_object_decals(int32_t cluster_slot);
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
extern void decal_place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction,
    real radius_scale, uint8_t object_attached, int16_t sequence_index);
}

namespace halo::structures {

void structure_decals::update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b, int16_t cluster_count)
{
    int32_t cluster_offset = 0;
    int32_t bit_index = 0;
    int16_t slot;

    if (global_structure_bsp->runtime_decals.count == 0) {
        *runtime_decals_suppressed = 0;
        return;
    }
    if (cluster_count < 1) {
        *runtime_decals_suppressed = 0;
        return;
    }

    for (slot = 0; ; slot = slot + 1) {
        uint32_t saved_seed = halo::math::globals().effect_random_seed;
        ScenarioStructureBSPCluster *cluster =
            (ScenarioStructureBSPCluster *)((uint8_t *)global_structure_bsp->clusters.pointer + cluster_offset);
        int cluster_has_decals = cluster->first_decal_index != (uint16_t)-1 && cluster->decal_count != 0;
        int entering;
        int leaving;

        if (!cluster_has_decals) {
            entering = 0;
            leaving = 0;
        } else {
            uint32_t bit = 1u << (bit_index & 0x1f);
            uint32_t word = (uint32_t)((bit_index >> 5) * 4);
            int suppressed = *runtime_decals_suppressed != 0;

            entering = !suppressed &&
                (*(uint32_t *)((uint8_t *)switch_group_a + word) & bit) != 0 &&
                (*(uint32_t *)((uint8_t *)switch_group_b + word) & bit) == 0;

            leaving = ((*(uint32_t *)((uint8_t *)switch_group_a + word) & bit) == 0 || suppressed) &&
                (*(uint32_t *)((uint8_t *)switch_group_b + word) & bit) != 0;
        }

        if (entering) {
            decal_evict_object_decals(slot);
        } else {
            halo::math::globals().effect_random_seed = saved_seed;
            if (leaving && cluster->decal_count != 0) {
                int32_t i;
                for (i = 0; i < cluster->decal_count; i = i + 1) {
                    ScenarioStructureBSPRuntimeDecal *decal =
                        (ScenarioStructureBSPRuntimeDecal *)global_structure_bsp->runtime_decals.pointer +
                        cluster->first_decal_index + i;
                    ScenarioDecalPalette *decal_palette =
                        (ScenarioDecalPalette *)global_scenario->decal_palette.pointer;
                    TagID shader_tag_id = decal_palette[decal->decal_type].reference.tag_id;
                    int spawn_ok = 1;
                    real_vector3d orientation;
                    float yaw = (float)decal->yaw * 0.02473695f;
                    float pitch = (float)decal->pitch * 0.012368475f;
                    float cos_pitch = (float)cos(pitch);
                    float cos_yaw = (float)cos(yaw);

                    orientation.i = cos_yaw * cos_pitch;
                    orientation.j = (float)sin(yaw) * cos_pitch;
                    orientation.k = (float)sin(pitch);

                    if (decals_for_all_responses == 0) {
                        Decal *shader_decal = (Decal *)tag_instances[shader_tag_id.index].data;
                        if (shader_decal->layer != decallayer_alpha_tested) {
                            spawn_ok = 0;
                        }
                    }

                    if (decals_enabled != 0 && spawn_ok) {
                        collision_result placement;

                        halo::math::globals().effect_random_seed = *(uint32_t *)&decal->position.z ^
                            *(uint32_t *)&decal->position.y ^ *(uint32_t *)&decal->position.x ^ 0xdeadc0de;
                        if (collision_test_movement_segment(0x100061,
                                (real_point3d *)&decal->position, &orientation, 0xffffffff,
                                &placement) != 0 &&
                            placement.type == _collision_result_type_structure &&
                            (*(uint8_t *)tag_instances[shader_tag_id.index].data & 0x10) == 0) {
                            decal_place(*(datum_index *)&shader_tag_id, &placement, &orientation, 1.0f, 1, -1);
                        }
                    }
                    halo::math::globals().effect_random_seed = saved_seed;
                }
            }
        }

        cluster_offset = cluster_offset + sizeof(ScenarioStructureBSPCluster);
        bit_index = bit_index + 1;
        if (cluster_count <= (int16_t)(slot + 1)) {
            *runtime_decals_suppressed = 0;
            return;
        }
    }
}

void structure_decals::runtime_decals_evict(void)
{
    int16_t cluster_count;
    int16_t cluster_index;

    if (global_structure_bsp->runtime_decals.count == 0) {
        return;
    }
    cluster_count = *(int16_t *)&global_structure_bsp->clusters.count;
    for (cluster_index = 0; cluster_index < cluster_count; cluster_index++) {
        uint8_t *cluster = (uint8_t *)global_structure_bsp->clusters.pointer + cluster_index * 0x68;

        if (*(uint16_t *)(cluster + 0xc) != 0xffff && *(int16_t *)(cluster + 0xe) != 0) {
            decal_evict_object_decals(cluster_index);
        }
    }
}

void structure_decals::runtime_decals_mark_dirty(void)
{
    *runtime_decals_suppressed = 1;
}

}  // namespace halo::structures
