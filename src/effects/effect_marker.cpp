#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern const real_vector3d *global_down3d_pointer;
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out);
extern void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset);
extern data_array *effect_location_data;
extern player_globals *local_player_globals;
extern uint8_t *effect_marker_callback_context;
extern void effect_marker_from_node_table(int16_t entry_index, uint8_t *context, object_marker *out);
extern data_array *effect_data;
extern void effect_rebuild_markers(effect *self, int32_t (*resolve_marker)(uint32_t, const char *, object_marker *, uint32_t));
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
extern data_array *object_data;
extern uint8_t *first_person_weapon_interfaces;
void effect_marker_environment_probe(uint32_t definition_index, int16_t location_index, real_point3d *marker_position, uint32_t sound_param);
datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker, uint8_t first_person);
effect_location_marker * effect_marker_next(effect *self, datum_index *marker, int32_t mode);
int32_t effect_marker_node_table_resolver(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
void effect_reattach_markers_for_object(int16_t first_person_weapon_index, datum_index object_index);
void effect_release_first_person_markers(int16_t first_person_weapon_index);
real_matrix4x3 * effect_resolve_marker_transform(effect *self, int16_t marker);
}

namespace halo::effects {

/**
 * Member form of the original effect_marker_environment_probe: environment probe.
 *
 * @address 0x4533b0
 */
void effect_view::environment_probe(uint32_t definition_index, int16_t location_index, real_point3d *marker_position, uint32_t sound_param)
{
    TagReflexive *reflexive = (TagReflexive *)halo::cache::globals().tag_instances[definition_index & 0xffff].data;

    if (location_index < (int32_t)reflexive->count) {
        real_point3d origin;
        real_vector3d delta;
        collision_result result;
        uint8_t hit;

        origin.x = marker_position->x;
        origin.y = marker_position->y;
        origin.z = marker_position->z + 0.15f;
        delta.i = global_down3d_pointer->i * 0.3f;
        delta.j = global_down3d_pointer->j * 0.3f;
        delta.k = global_down3d_pointer->k * 0.3f;

        hit = collision_test_movement_segment(0xc2a0, &origin, &delta, 0xffffffff, &result);
        if (hit) {
            uint8_t in_sky = scenario_location_get_water_and_weather(&result.point, &result.leaf, 0);
            int16_t material_type = in_sky ? 0x1c : result.material_type;

            material_effects_play_at_marker(definition_index, location_index, material_type,
                                             (uint32_t *)&result.leaf, sound_param, &result.point,
                                             (real_vector3d *)&result.plane);
        }
    }
}

/**
 * Member form of the original effect_marker_from_node_table: from node table.
 *
 * @address 0x451930
 */
void effect_view::from_node_table(int16_t entry_index, uint8_t *context, object_marker *out)
{
    real_matrix4x3 *node = *(real_matrix4x3 **)(context + 4);
    real_point3d *point = (real_point3d *)(*(uint8_t **)(context + 0x10) + entry_index * 0xc);
    real_vector3d *normal = (real_vector3d *)(*(uint8_t **)(context + 0x14) + entry_index * 0xc);
    real_point3d position;
    real_vector3d forward;
    real_vector3d up;

    *(uint16_t *)out = *(uint16_t *)context;
    if (node != 0) {
        halo::math::matrix4x3_inverse_transform_point(*node, position, *point);
        forward.i = normal->k * node->forward.k + normal->j * node->forward.j + normal->i * node->forward.i;
        forward.j = normal->k * node->left.k + normal->j * node->left.j + normal->i * node->left.i;
        forward.k = normal->k * node->up.k + normal->j * node->up.j + normal->i * node->up.i;
    } else {
        position = *point;
        forward = *normal;
    }
    halo::math::vector3d_build_perpendicular(up, forward);
    halo::math::vector3d_normalize_with_length(up);
    halo::math::matrix4x3_from_forward_up(up, forward, *(real_matrix4x3 *)((uint8_t *)out + 4));
    ((real_matrix4x3 *)((uint8_t *)out + 4))->position = position;
}

/**
 * Allocates an effect_location_marker for a marker resolved by effect_rebuild_markers, copies
 * its transform, and links it at the head of effect->location_markers[location_index].
 *
 * @address 0x4517d0
 */
datum_index effect_view::create(int16_t location_index, object_marker *resolved_marker, uint8_t first_person)
{
    effect * self = record;
    datum_index handle = halo::memory::datum_new(effect_location_data);

    if (handle != k_datum_index_none) {
        effect_location_marker *marker =
            &((effect_location_marker *)effect_location_data->data)[(uint16_t)handle];
        uint16_t node_index = resolved_marker->node_index;

        if (node_index != 0xffff) {
            node_index = first_person ? (node_index | 0x8000) : (node_index & 0x7fff);
        }
        marker->marker_index = node_index;
        marker->transform = resolved_marker->transform;

        marker->next_marker = self->location_markers[location_index];
        self->location_markers[location_index] = handle;
    }
    return handle;
}

/**
 * Walks the linked list of effect_location_marker records starting at *marker (advancing *marker
 * to the next entry as it goes) and returns the first entry whose first-person-ness matches the
 * requested evaluation mode: mode 1, or mode 3 while self is a first-person-weapon effect and
 * the local player is in first person view, keeps only markers carrying the first-person bit
 * (0x8000) or the "object origin" sentinel 0xffff; any other mode keeps only plain markers.
 * Returns NULL once the list is exhausted without a match.
 *
 * @address 0x453180
 */
effect_location_marker * effect_view::next(datum_index *marker, int32_t mode)
{
    effect * self = record;
    effect_location_marker *entry;

    if (*marker == (datum_index)0xffffffff) {
        return (effect_location_marker *)0;
    }

    entry = &((effect_location_marker *)effect_location_data->data)[*marker & 0xffff];
    *marker = entry->next_marker;

    if (mode == 1 ||
        (mode == 3 && self->first_person_weapon_index != -1 &&
         local_player_globals->local_player_count == 1)) {
        if (entry->marker_index == 0xffff || (entry->marker_index & 0x8000) == 0) {
            return effect_marker_next(self, marker, mode);
        }
    } else if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0) {
        return effect_marker_next(self, marker, mode);
    }

    return entry;
}

/**
 * Member form of the original effect_marker_node_table_resolver: node table resolver.
 *
 * @address 0x451850
 */
int32_t effect_view::node_table_resolver(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count)
{
    uint8_t *context = effect_marker_callback_context;
    int16_t count = 0;

    (void)object_index;
    if (*(char ***)(context + 0xc) != 0 && strlen(location) != 0 && (int16_t)max_count > 0) {
        int16_t i;

        for (i = 0; count < (int16_t)max_count; i++) {
            if (i >= *(int16_t *)(context + 8)) {
                break;
            }
            if (strcmp(location, (*(char ***)(context + 0xc))[i]) == 0) {
                effect_marker_from_node_table(i, context, &out[count]);
                count++;
            }
        }
        if (count != 0) {
            return count;
        }
    }
    effect_marker_from_node_table(0, context, out);
    return 1;
}

/**
 * For every live effect attached to `object_index`, updates its first-person weapon index and
 * re-resolves its first-person markers.
 *
 * @address 0x450cb0
 */
void effect_view::reattach_markers_for_object(int16_t first_person_weapon_index, datum_index object_index)
{
    datum_index effect_index = halo::memory::datum_next(-1, effect_data);

    while (effect_index != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)effect_index];

        if (self->object_index == object_index) {
            self->first_person_weapon_index = first_person_weapon_index;
            effect_rebuild_markers(self, first_person_weapon_get_marker_data);
        }

        effect_index = halo::memory::datum_next((int16_t)effect_index, effect_data);
    }
}

/**
 * Invalidates and removes every first-person marker reference (a marker_index with the
 * k_effect_marker_first_person_bit set) from every live effect using `first_person_weapon_index`,
 * and clears that effect's first_person_weapon_index -- used when a first-person weapon's marker
 * table is being freed.
 *
 * @address 0x450d50
 */
void effect_view::release_first_person_markers(int16_t first_person_weapon_index)
{
    datum_index effect_index = halo::memory::datum_next(-1, effect_data);

    while (effect_index != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)effect_index];
        Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;

        if (self->first_person_weapon_index == first_person_weapon_index) {
            int32_t location_index;

            for (location_index = 0; location_index < (int32_t)tag->locations.count; location_index++) {
                datum_index *link = &self->location_markers[location_index];

                while (*link != k_datum_index_none) {
                    effect_location_marker *marker =
                        &((effect_location_marker *)effect_location_data->data)[(uint16_t)*link];

                    if (marker->marker_index == 0xffff || (int16_t)marker->marker_index >= 0) {
                        link = &marker->next_marker;
                    } else {
                        datum_index next = marker->next_marker;
                        halo::memory::datum_delete(effect_location_data, *link);
                        *link = next;
                    }
                }
            }

            self->first_person_weapon_index = -1;
        }

        effect_index = halo::memory::datum_next((int16_t)effect_index, effect_data);
    }
}

/**
 * Member form of the original effect_resolve_marker_transform: resolve marker transform.
 *
 * @address 0x453220
 */
real_matrix4x3 * effect_view::resolve_marker_transform(int16_t marker)
{
    effect * self = record;
    int16_t node_index = marker;

    if (node_index != (int16_t)0xffff) {
        if ((marker & 0x8000) != 0) {
            uint16_t weapon_node = (uint16_t)marker & 0x7fff;
            return (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
                                       self->first_person_weapon_index * 0x1ea0 +
                                       weapon_node * 0x34);
        }
        node_index = (int16_t)((uint16_t)marker & 0x7fff);
    }

    {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        return (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + node_index * 0x34);
    }
}

}

extern "C" {

void effect_marker_environment_probe(uint32_t definition_index, int16_t location_index, real_point3d *marker_position, uint32_t sound_param)
{
    halo::effects::effect_view::environment_probe(definition_index, location_index, marker_position, sound_param);
}

void effect_marker_from_node_table(int16_t entry_index, uint8_t *context, object_marker *out)
{
    halo::effects::effect_view::from_node_table(entry_index, context, out);
}

datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker, uint8_t first_person)
{
    return halo::effects::effect_view(self).create(location_index, resolved_marker, first_person);
}

effect_location_marker * effect_marker_next(effect *self, datum_index *marker, int32_t mode)
{
    return halo::effects::effect_view(self).next(marker, mode);
}

int32_t effect_marker_node_table_resolver(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count)
{
    return halo::effects::effect_view::node_table_resolver(object_index, location, out, max_count);
}

void effect_reattach_markers_for_object(int16_t first_person_weapon_index, datum_index object_index)
{
    halo::effects::effect_view::reattach_markers_for_object(first_person_weapon_index, object_index);
}

void effect_release_first_person_markers(int16_t first_person_weapon_index)
{
    halo::effects::effect_view::release_first_person_markers(first_person_weapon_index);
}

real_matrix4x3 * effect_resolve_marker_transform(effect *self, int16_t marker)
{
    return halo::effects::effect_view(self).resolve_marker_transform(marker);
}

}
