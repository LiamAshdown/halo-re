/**
 * @file include/halo/objects/object_queries.hpp
 * Object system API: object queries.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Spatial and cluster queries over the object data array: sphere searches, cluster collection, iteration and checked
 * lookup.
 */
class ObjectQueries {
public:
    /**
     * Starts a cursor over the non-collideable object references of a cluster.
     *
     * Original register convention: stack -> cursor, cluster_index (cdecl).
     *
     * @address 0x004f5e90
     */
    static datum_index noncollideable_iterate_begin(datum_index *cursor, int16_t cluster_index);

    /**
     * Advances a cursor over the non-collideable object references of a cluster.
     *
     * Original register convention: stack -> cursor (cdecl).
     *
     * @address 0x004f5ed0
     */
    static datum_index noncollideable_iterate_next(datum_index *cursor);

    /**
     * Resolves the next collideable reference of a cluster to an object handle.
     *
     * Original register convention: stack -> next_reference, cluster_index.
     *
     * @address 0x004f5f00
     */
    static datum_index resolve_collideable_reference(datum_index *next_reference, int16_t cluster_index);

    /**
     * Advances a cursor over the collideable object references of a cluster.
     *
     * Original register convention: stack -> cursor (cdecl).
     *
     * @address 0x004f5f40
     */
    static datum_index collideable_iterate_next(datum_index *cursor);

    /**
     * Returns the object for a handle when it is valid and its type matches the mask, otherwise null.
     *
     * @address 0x004f6ec0
     */
    static object * try_and_get(datum_index object_index, uint32_t type_mask);

    /**
     * Finds objects of the given types in a sphere and writes their handles to out_objects, returning the count.
     *
     * @address 0x004f6fe0
     */
    static int16_t find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location, real_point3d *center,
    float radius, datum_index *out_objects, int16_t max_output);

    /**
     * Collects objects matching a type mask from a list of clusters into out_objects, visiting each object once.
     *
     * Original register convention: all five parameters are plain STACK arguments (Ghidra's own signature). Confirmed
     * against objdump -d -M intel bin/halo.exe: 0x4f7180 mov ecx,[esp+0x4] reads the first stack slot before any
     * register is otherwise touched.
     *
     * @address 0x004f7180
     */
    static int16_t collect_in_clusters(uint32_t search_mask, int16_t cluster_count, int16_t *cluster_indices,
    int16_t max_output, datum_index *out_objects);

    /**
     * Stamps the object with the current cluster stamp; returns whether it had already been visited this pass.
     *
     * Original register convention: stack -> object_index (cdecl); returns AL.
     *
     * @address 0x004f9720
     */
    static uint8_t cluster_stamp_mark_visited(datum_index object_index);

    /**
     * Collects the object and its children that the caller's filter accepts.
     *
     * Original register convention: all six parameters are plain stack arguments (Ghidra's own fully recovered
     * signature, no in_REG markers anywhere).
     *
     * @address 0x004fa0f0
     */
    static int32_t tree_collect_matching(uint32_t object_index, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t count, int32_t max_count, datum_index *out);

    /**
     * Collects objects relevant to the local player near a point, accepted by a caller filter, up to max_count.
     *
     * Original register convention: EDX -> point, stack -> filter, filter_context, max_count, out.
     *
     * @address 0x004fa1a0
     */
    static int32_t collect_local_player_relevant_objects(real_point3d *point, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t max_count, datum_index *out);

    /**
     * Collects objects from a cluster bit array, filtering by the caller's callback.
     *
     * Original register convention: UNRESOLVED, see file header.
     *
     * @address 0x004fa280
     */
    static int32_t collect_by_flag_bits(int32_t bit_index, int32_t remaining_bits, int16_t range_index,
    int16_t range_count, int32_t *bit_array, int32_t cluster_stamp_snapshot, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t count, int32_t max_count, datum_index *out);
};

/**
 * Cursor over the object data array filtered by a type mask.
 */
class ObjectIteratorView {
public:
    explicit ObjectIteratorView(object_iterator *self) : self(self) {}

    /**
     * Advances the iterator to the next object matching its type mask and returns it, or null at the end.
     *
     * @address 0x004f6f20
     */
    object * next();

private:
    object_iterator *self;
};

}  // namespace halo::objects
