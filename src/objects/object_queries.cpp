#include "halo/objects/object_queries.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/objects/vars.hpp"
#include "halo/physics/vars.hpp"

static auto &collideable_cluster_first = halo::link::ref<datum_index *>(halo::objects::vars().collideable_cluster_first);
static auto &collideable_object_references = halo::link::ref<data_array *>(halo::physics::vars().collideable_object_references);
static auto &noncollideable_cluster_first = halo::link::ref<datum_index *>(halo::objects::vars().noncollideable_cluster_first);
static auto &noncollideable_object_references = halo::link::ref<data_array *>(halo::objects::vars().noncollideable_object_references);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &object_globals_pointer = halo::link::ref<object_globals *>(halo::objects::vars().object_globals_pointer);

/**
 * Starts a cursor over the non-collideable object references of a cluster.
 *
 * Original register convention: stack -> cursor, cluster_index (cdecl).
 *
 * @address 0x004f5e90
 */
datum_index halo::objects::ObjectQueries::noncollideable_iterate_begin(datum_index *cursor, int16_t cluster_index)
{
    datum_index reference = noncollideable_cluster_first[cluster_index];
    uint8_t *element;

    *cursor = reference;
    if (reference == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)noncollideable_object_references->data + halo::datum_slot(reference) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}

/**
 * Advances a cursor over the non-collideable object references of a cluster.
 *
 * Original register convention: stack -> cursor (cdecl).
 *
 * @address 0x004f5ed0
 */
datum_index halo::objects::ObjectQueries::noncollideable_iterate_next(datum_index *cursor)
{
    uint8_t *element;

    if (*cursor == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)noncollideable_object_references->data + halo::datum_slot(*cursor) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}

/**
 * Resolves the next collideable reference of a cluster to an object handle.
 *
 * Original register convention: stack -> next_reference, cluster_index.
 *
 * @address 0x004f5f00
 */
datum_index halo::objects::ObjectQueries::resolve_collideable_reference(datum_index *next_reference,
    int16_t cluster_index)
{
    datum_index head = halo::physics::globals().collideable_cluster_first[cluster_index];
    object_cluster_reference *ref;

    *next_reference = head;
    if (head != k_datum_index_none) {
        ref = (object_cluster_reference *)collideable_object_references->data + halo::datum_slot(head);
        *next_reference = ref->next_reference;
        return ref->object_index;
    }
    return k_datum_index_none;
}

/**
 * Advances a cursor over the collideable object references of a cluster.
 *
 * Original register convention: stack -> cursor (cdecl).
 *
 * @address 0x004f5f40
 */
datum_index halo::objects::ObjectQueries::collideable_iterate_next(datum_index *cursor)
{
    uint8_t *element;

    if (*cursor == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)collideable_object_references->data + halo::datum_slot(*cursor) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}

/**
 * Returns the object for a handle when it is valid and its type matches the mask, otherwise null.
 *
 * @address 0x004f6ec0
 */
object * halo::objects::ObjectQueries::try_and_get(datum_index object_index, uint32_t type_mask)
{
    object_header *found = (object_header *)0;

    if ((object_index != k_datum_index_none) && ((int16_t)object_index >= 0) &&
        ((int16_t)object_index < object_data->maximum_count)) {
        object_header *header = (object_header *)object_data->data + (int16_t)object_index;
        if ((header->identifier != 0) &&
            (((int16_t)(object_index >> 16) == 0) || (header->identifier == (int16_t)(object_index >> 16)))) {
            found = header;
        }
    }

    if ((found != (object_header *)0) && ((type_mask & (1 << (found->type & 0x1f))) != 0)) {
        return found->data;
    }
    return (object *)0;
}

/**
 * Advances the iterator to the next object matching its type mask and returns it, or null at the end.
 *
 * @address 0x004f6f20
 */
object * halo::objects::ObjectIteratorView::next()
{
    object_iterator *iterator = self;
    int16_t index = iterator->index;

    while (index < object_data->last_index) {
        object_header *header = (object_header *)object_data->data + index;
        int16_t this_index = index;
        index++;

        if ((header->identifier != 0) &&
            ((header->flags & iterator->flags_mask) == iterator->flags_mask) &&
            ((iterator->type_mask & (1u << (header->type & 0x1f))) != 0)) {
            iterator->handle = ((uint32_t)(uint16_t)header->identifier << 16) | (uint16_t)this_index;
            iterator->index = index;
            return header->data;
        }
    }

    iterator->index = index;
    return (object *)0;
}

/**
 * Finds objects of the given types in a sphere and writes their handles to out_objects, returning the count.
 *
 * @address 0x004f6fe0
 */
int16_t halo::objects::ObjectQueries::find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output)
{
    int16_t clusters[512];
    datum_index candidates[0x800];
    int16_t cluster_count;
    int16_t candidate_count;
    int16_t result_count = 0;
    int16_t i;
    int16_t start_cluster;

    if (type_mask == 0) {
        type_mask = halo::to_bits(halo::objects::object_mask::all);
    }

    start_cluster = *(int16_t *)((uint8_t *)location + 4);
    cluster_count = 0;
    if (start_cluster != -1) {
        if (radius <= 0.0f) {
            cluster_count = 1;
            clusters[0] = start_cluster;
        } else {
            halo::structures::globals().cluster_flood_stamp++;
            halo::structures::globals().cluster_flood_in_progress = 1;
            cluster_count = halo::structures::cluster_flood_fill_within_radius(start_cluster, center, radius, 0x200, clusters);
            halo::structures::globals().cluster_flood_in_progress = 0;
        }
    }

    candidate_count = halo::objects::object_collect_in_clusters(search_mask, cluster_count, clusters, 0x800, candidates);

    for (i = 0; i < candidate_count; i++) {
        object *obj;
        if (max_output <= result_count) {
            return result_count;
        }
        obj = ((object_header *)object_data->data)[candidates[i] & 0xffff].data;
        if ((type_mask & (1u << (obj->type & 0x1f))) != 0) {
            float sum_radius = radius + obj->bounding_radius;
            float dx = obj->bounding_center.x - center->x;
            float dy = obj->bounding_center.y - center->y;
            float dz = obj->bounding_center.z - center->z;
            if (dy * dy + dz * dz + dx * dx <= sum_radius * sum_radius) {
                out_objects[result_count] = candidates[i];
                result_count++;
            }
        }
    }

    return result_count;
}

/**
 * Collects objects matching a type mask from a list of clusters into out_objects, visiting each object once.
 *
 * Original register convention: all five parameters are plain STACK arguments (Ghidra's own signature). Confirmed
 * against objdump -d -M intel bin/halo.exe: 0x4f7180 mov ecx,[esp+0x4] reads the first stack slot before any register
 * is otherwise touched.
 *
 * @address 0x004f7180
 */
int16_t halo::objects::ObjectQueries::collect_in_clusters(uint32_t search_mask, int16_t cluster_count,
    int16_t *cluster_indices, int16_t max_output, datum_index *out_objects)
{
    int16_t count = 0;
    int32_t stamp;
    int16_t i;

    if (search_mask == 0) {
        search_mask = halo::to_bits(halo::objects::object_mask::all);
    }
    object_globals_pointer->collecting_in_clusters = 1;
    halo::physics::globals().object_cluster_stamp = halo::physics::globals().object_cluster_stamp + 1;
    stamp = halo::physics::globals().object_cluster_stamp;

    for (i = 0; i < cluster_count; i++) {
        int16_t cluster_index = cluster_indices[i];

        if ((search_mask & 1) != 0) {
            datum_index ref = halo::physics::globals().collideable_cluster_first[cluster_index];
            while (ref != k_datum_index_none) {
                object_cluster_reference *node = (object_cluster_reference *)
                    collideable_object_references->data + halo::datum_slot(ref);
                datum_index object_index = node->object_index;
                object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
                if (obj->cluster_stamp != stamp) {
                    obj->cluster_stamp = stamp;
                    if (max_output <= count) {
                        object_globals_pointer->collecting_in_clusters = 0;
                        return count;
                    }
                    out_objects[count] = object_index;
                    count++;
                }
                ref = node->next_reference;
            }
        }

        if ((search_mask & 2) != 0) {
            datum_index ref = noncollideable_cluster_first[cluster_index];
            while (ref != k_datum_index_none) {
                object_cluster_reference *node = (object_cluster_reference *)
                    noncollideable_object_references->data + halo::datum_slot(ref);
                datum_index object_index = node->object_index;
                object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
                if (obj->cluster_stamp != stamp) {
                    obj->cluster_stamp = stamp;
                    if (max_output <= count) {
                        object_globals_pointer->collecting_in_clusters = 0;
                        return count;
                    }
                    out_objects[count] = object_index;
                    count++;
                }
                ref = node->next_reference;
            }
        }
    }

    object_globals_pointer->collecting_in_clusters = 0;
    return count;
}

namespace {
static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(object_index) * 0xc + 8);
}
}

/**
 * Stamps the object with the current cluster stamp; returns whether it had already been visited this pass.
 *
 * Original register convention: stack -> object_index (cdecl); returns AL.
 *
 * @address 0x004f9720
 */
uint8_t halo::objects::ObjectQueries::cluster_stamp_mark_visited(datum_index object_index)
{
    uint8_t *object = object_get(object_index);

    if (((struct object *)object)->cluster_stamp == halo::physics::globals().object_cluster_stamp) {
        return 0;
    }
    ((struct object *)object)->cluster_stamp = halo::physics::globals().object_cluster_stamp;
    return 1;
}

/**
 * Collects the object and its children that the caller's filter accepts.
 *
 * Original register convention: all six parameters are plain stack arguments (Ghidra's own fully recovered signature,
 * no in_REG markers anywhere).
 *
 * @address 0x004fa0f0
 */
int32_t halo::objects::ObjectQueries::tree_collect_matching(uint32_t object_index,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t count, int32_t max_count, datum_index *out)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (max_count <= count) {
        return count;
    }

    if ((filter == 0) || (filter(object_index, filter_context) != 0)) {
        out[count] = object_index;
        count++;
    }

    if (obj->first_child_object != k_datum_index_none) {
        count = halo::objects::object_tree_collect_matching(obj->first_child_object, filter, filter_context, count, max_count, out);
    }

    if (obj->next_object == k_datum_index_none) {
        return count;
    }

    return halo::objects::object_tree_collect_matching(obj->next_object, filter, filter_context, count, max_count, out);
}

/**
 * Collects objects relevant to the local player near a point, accepted by a caller filter, up to max_count.
 *
 * Original register convention: EDX -> point, stack -> filter, filter_context, max_count, out.
 *
 * @address 0x004fa1a0
 */
int32_t halo::objects::ObjectQueries::collect_local_player_relevant_objects(real_point3d *point,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t max_count, datum_index *out)
{
    int32_t count = 0;
    uint32_t leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, point);
    ScenarioStructureBSP *bsp;
    int16_t cluster;
    int32_t words;
    int32_t *row;
    int32_t *word;
    int16_t word_index;

    if (leaf == k_datum_index_none) {
        return 0;
    }
    bsp = halo::scenario::globals().structure_bsp;
    cluster = *(int16_t *)((uint8_t *)bsp->leaves.pointer + (leaf & halo::k_leaf_index_mask) * 0x10 + 8);
    if (cluster == -1) {
        return 0;
    }

    object_globals_pointer->collecting_in_clusters = 1;
    words = ((int32_t)bsp->clusters.count + 0x1f) >> 5;
    row = (int32_t *)((uint8_t *)bsp->cluster_data.pointer) + (int32_t)cluster * words;
    halo::physics::globals().object_cluster_stamp = halo::physics::globals().object_cluster_stamp + 1;

    word = row;
    for (word_index = 0; word_index < (int16_t)words; word_index++, word++) {
        int32_t cluster_count;
        int32_t lo;
        int32_t hi;
        int32_t bit;
        int32_t remaining;

        if (*word == 0) {
            continue;
        }
        cluster_count = *(int32_t *)&halo::scenario::globals().structure_bsp->clusters.count;
        lo = (int16_t)(word_index << 5);
        hi = lo + 0x20;
        if (hi > cluster_count) {
            hi = cluster_count;
        }
        if ((int16_t)lo >= (int16_t)hi) {
            continue;
        }
        remaining = (uint16_t)((int16_t)hi - (int16_t)lo);
        bit = lo;
        do {
            if ((row[bit >> 5] & (1u << (bit & 0x1f))) != 0) {
                datum_index ref = halo::physics::globals().collideable_cluster_first[bit];

                while (ref != k_datum_index_none) {
                    object_cluster_reference *node =
                        (object_cluster_reference *)collideable_object_references->data + halo::datum_slot(ref);
                    datum_index object_index = node->object_index;
                    object *obj;

                    ref = node->next_reference;
                    obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
                    if (obj->cluster_stamp != halo::physics::globals().object_cluster_stamp) {
                        obj->cluster_stamp = halo::physics::globals().object_cluster_stamp;
                        count = halo::objects::object_tree_collect_matching(object_index, filter, filter_context, count, max_count,
                            out);
                    }
                }
            }
            bit++;
        } while (--remaining != 0);
    }

    object_globals_pointer->collecting_in_clusters = 0;
    return count;
}

/**
 * Collects objects from a cluster bit array, filtering by the caller's callback.
 *
 * Original register convention: UNRESOLVED, see file header.
 *
 * @address 0x004fa280
 */
int32_t halo::objects::ObjectQueries::collect_by_flag_bits(int32_t bit_index, int32_t remaining_bits,
    int16_t range_index, int16_t range_count, int32_t *bit_array, int32_t cluster_stamp_snapshot,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t count, int32_t max_count, datum_index *out)
{
    int32_t result = count;

    do {
        if ((bit_array[bit_index >> 5] & (1u << (bit_index & 0x1f))) != 0) {
            datum_index ref = halo::physics::globals().collideable_cluster_first[bit_index];

            while (ref != k_datum_index_none) {
                object_cluster_reference *node = (object_cluster_reference *)
                    collideable_object_references->data + halo::datum_slot(ref);
                object *obj = ((object_header *)object_data->data)[halo::datum_slot(node->object_index)].data;

                if (obj->cluster_stamp != cluster_stamp_snapshot) {
                    obj->cluster_stamp = cluster_stamp_snapshot;
                    result = halo::objects::object_tree_collect_matching(node->object_index, filter, filter_context, result, max_count, out);
                    cluster_stamp_snapshot = halo::physics::globals().object_cluster_stamp;
                }
                ref = node->next_reference;
            }
        }
        bit_index = bit_index + 1;
        remaining_bits = remaining_bits - 1;
    } while (remaining_bits != 0);

    for (;;) {
        range_index = range_index + 1;
        bit_array = bit_array + 1;
        if (range_count <= range_index) {
            object_globals_pointer->collecting_in_clusters = 0;
            return result;
        }
        if (*bit_array != 0) {
            int32_t hi = (int16_t)(range_index * 0x20) + 0x20;
            if (hi > range_count) {
                hi = range_count;
            }
            if ((int16_t)(range_index * 0x20) < (int16_t)hi) {
                break;
            }
        }
    }

    return halo::objects::object_type_definitions_collect_by_flag_bits(bit_index, remaining_bits, range_index,
        range_count, bit_array, cluster_stamp_snapshot, filter, filter_context, result, max_count, out);
}
