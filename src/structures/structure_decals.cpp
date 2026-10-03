/**
 * @file src/structures/structure_decals.cpp
 * Runtime decals attached to structure bsp clusters.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/structures/api.hpp"

extern "C" {
extern ScenarioStructureBSP *global_structure_bsp;
extern Scenario *global_scenario;
extern uint8_t decals_enabled;
extern double cos(double x);
extern double sin(double x);
}

namespace halo::structures {

void structure_decals::update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b, int16_t cluster_count)
{
    int32_t cluster_offset = 0;
    int32_t bit_index = 0;
    int16_t slot;

    if (global_structure_bsp->runtime_decals.count == 0) {
        *globals().runtime_decals_suppressed = 0;
        return;
    }
    if (cluster_count < 1) {
        *globals().runtime_decals_suppressed = 0;
        return;
    }

    for (slot = 0; ; slot = slot + 1) {
        uint32_t saved_seed = halo::math::globals().effect_random_seed;
        ScenarioStructureBSPCluster *cluster =
            (ScenarioStructureBSPCluster *)((uint8_t *)global_structure_bsp->clusters.pointer + cluster_offset);
        int cluster_has_decals = cluster->first_decal_index != k_word_none && cluster->decal_count != 0;
        int entering;
        int leaving;

        if (!cluster_has_decals) {
            entering = 0;
            leaving = 0;
        } else {
            uint32_t bit = bit_array_mask(bit_index);
            uint32_t word = (uint32_t)(bit_array_word(bit_index) * 4);
            int suppressed = *halo::structures::globals().runtime_decals_suppressed != 0;

            entering = !suppressed &&
                (*(uint32_t *)((uint8_t *)switch_group_a + word) & bit) != 0 &&
                (*(uint32_t *)((uint8_t *)switch_group_b + word) & bit) == 0;

            leaving = ((*(uint32_t *)((uint8_t *)switch_group_a + word) & bit) == 0 || suppressed) &&
                (*(uint32_t *)((uint8_t *)switch_group_b + word) & bit) != 0;
        }

        if (entering) {
            halo::effects::decal_evict_object_decals(slot);
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

                    if (halo::effects::globals().decals_for_all_responses == 0) {
                        Decal *shader_decal = (Decal *)halo::cache::globals().tag_instances[shader_tag_id.index].data;
                        if (shader_decal->layer != decallayer_alpha_tested) {
                            spawn_ok = 0;
                        }
                    }

                    if (decals_enabled != 0 && spawn_ok) {
                        collision_result placement;

                        halo::math::globals().effect_random_seed = *(uint32_t *)&decal->position.z ^
                            *(uint32_t *)&decal->position.y ^ *(uint32_t *)&decal->position.x ^ k_decal_placement_seed_xor;
                        if (halo::physics::collision_test_movement_segment(to_bits(k_decal_placement_query),
                                (real_point3d *)&decal->position, &orientation, k_dword_none,
                                &placement) != 0 &&
                            placement.type == _collision_result_type_structure &&
                            (*(uint8_t *)halo::cache::globals().tag_instances[shader_tag_id.index].data & k_decal_shader_skip_structure_flag) == 0) {
                            halo::effects::decal_place(*(datum_index *)&shader_tag_id, &placement, &orientation, 1.0f, 1, -1);
                        }
                    }
                    halo::math::globals().effect_random_seed = saved_seed;
                }
            }
        }

        cluster_offset = cluster_offset + sizeof(ScenarioStructureBSPCluster);
        bit_index = bit_index + 1;
        if (cluster_count <= (int16_t)(slot + 1)) {
            *globals().runtime_decals_suppressed = 0;
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
        ScenarioStructureBSPCluster *cluster = (ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer + cluster_index;

        if (cluster->first_decal_index != k_word_none && cluster->decal_count != 0) {
            halo::effects::decal_evict_object_decals(cluster_index);
        }
    }
}

void structure_decals::runtime_decals_mark_dirty(void)
{
    *globals().runtime_decals_suppressed = 1;
}

}  // namespace halo::structures
