#pragma once

#include "halo/hs/hs3_types.hpp"

namespace halo::hs::part3 {

/**
 * Script-side object helpers: named object slots, object lists built for scripts, angle and volume predicates.
 */
class ScriptObjects {
public:
    uint8_t object_angle_predicate_helper(datum_index object_index, datum_index viewer_unit, float angle_degrees) const;
    void object_create_name_index_if_absent(int32_t name_index) const;
    void object_detach_and_place_at_location(int16_t location_index, datum_index object_index, char detach_from_parent, char reorient) const;
    char object_hierarchy_test(datum_index object_index) const;
    uint32_t object_list_any_angle_match(datum_index header_index, datum_index target_object, float angle_degrees) const;
    uint32_t object_list_any_angle_match_gated(datum_index header_index, int16_t gate, float angle_degrees) const;
    datum_index object_list_collect_player_units() const;
    void object_list_for_each(datum_index header_index) const;
    datum_index object_list_new_singleton(datum_index object_index) const;
    char object_list_test_trigger_volume(int32_t trigger_volume_index, datum_index header_index, char all_mode) const;
    void object_name_cache_validate(int16_t object_name_index) const;
    void object_name_destroy(int32_t object_name_index) const;
    void object_names_for_each(void (*callback)(int32_t index), uint32_t predicate_arg) const;
    int32_t object_orient(float x) const;
    void object_runtime_cleanup() const;
    void object_set_health_fraction(datum_index object_index, float fraction) const;
    void hs_object_set_permutation_by_name(datum_index object_index, void *permutation_name, char *name) const;
    void objects_delete_by_type(uint32_t tag_id) const;
};

/**
 * The object-list reference chains used by scripts: iteration, insertion and disposal.
 */
class ObjectLists {
public:
    int32_t get_first(datum_index header_index, object_list_iterator *iterator_out) const;
    int32_t nth_reference(datum_index header_index, int16_t n) const;
    void reference_add(datum_index header_index, datum_index object_index) const;
    void reference_chain_delete(data_array *reference_array, datum_index chain_head) const;
    void dispose_empty() const;
    void initialize() const;
};

}
