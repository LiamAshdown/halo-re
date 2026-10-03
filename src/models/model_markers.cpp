/**
 * @file src/models/model_markers.cpp
 * Marker group lookup of model tags.
 * The original author notes and decompiles are in docs/original/models/.
 */

#include "halo/models/models.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"


namespace halo::models {

int16_t model_markers::group_index_from_name(datum_index model_tag_id, const char *name)
{
    GBXModel *model;
    ModelMarker *markers;
    int16_t lo, hi;

    if (model_tag_id == (datum_index)-1 || name == 0 || *name == '\0') {
        return -1;
    }

    model = (GBXModel *)halo::cache::globals().tag_instances[model_tag_id & 0xffff].data;
    markers = (ModelMarker *)model->markers.pointer;

    lo = 0;
    hi = (int16_t)(model->markers.count - 1);
    while (lo <= hi) {
        int16_t mid = (int16_t)((hi + lo) / 2);
        int32_t cmp = _stricmp(name, markers[mid].name.string);

        if (cmp == 0) {
            return mid;
        }
        if (cmp < 0) {
            hi = (int16_t)(mid - 1);
        } else {
            lo = (int16_t)(mid + 1);
        }
    }
    return -1;
}

int16_t model_markers::get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations, int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum)
{
    int16_t group_index;
    GBXModel *model;
    ModelMarker *marker;
    ModelMarkerInstance *instances;
    int16_t result_count;
    int16_t i;

    group_index = model_markers::group_index_from_name(model_tag_id, name);
    if (group_index == -1) {
        return 0;
    }

    model = (GBXModel *)halo::cache::globals().tag_instances[model_tag_id & 0xffff].data;
    marker = &((ModelMarker *)model->markers.pointer)[group_index];
    if (marker->instances.count <= 0) {
        return 0;
    }
    instances = (ModelMarkerInstance *)marker->instances.pointer;

    result_count = 0;
    for (i = 0; (int32_t)i < marker->instances.count; i++) {
        ModelMarkerInstance *instance = &instances[i];

        if (region_permutations != 0 && region_permutations[instance->region_index] != instance->permutation_index) {
            continue;
        }
        if (maximum <= result_count) {
            return result_count;
        }

        {
            object_marker *entry = &out[result_count];
            int16_t node_index;

            result_count = result_count + 1;
            node_index = (node_remap == 0) ? (int16_t)instance->node_index : node_remap[instance->node_index];
            entry->node_index = node_index;

            halo::math::matrix4x3_from_quaternion(*((real_quaternion *)&instance->rotation), entry->transform);
            entry->transform.position = *(real_point3d *)&instance->translation;

            halo::math::globals().matrix4x3_multiply_procedure(&node_matrices[node_index], &entry->transform, &entry->node_transform);

            if (mirrored != 0) {
                entry->node_transform.left.i = -entry->node_transform.left.i;
                entry->node_transform.left.j = -entry->node_transform.left.j;
                entry->node_transform.left.k = -entry->node_transform.left.k;
            }
        }
    }
    return result_count;
}

}  // namespace halo::models
