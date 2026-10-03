#include "halo/effects/effects.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern data_array *effect_data;
extern void effect_start_event(datum_index effect_handle, int16_t event_index);
extern const ColorRGB *global_white_color;
extern int32_t local_player_index_for_object(datum_index object_index);
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
extern uint8_t first_person_effects_enabled;
extern data_array *object_data;
extern void *effect_marker_callback_context;
extern const real_point3d *global_origin3d_pointer;
}

namespace halo::effects {

/**
 * Allocates a new effect for `definition_index`, refusing effects flagged
 * must_be_deterministic_pc unless `force_create` is set or, once the pool is exhausted,
 * evicting the first non-deterministic effect found (via a full-table scan) to make room.
 * Requires the tag to have at least one event.
 *
 * @address 0x451500
 */
datum_index effect_ref::create(datum_index definition_index, datum_index creator_object_index, uint8_t force_create)
{
    datum_index handle = k_datum_index_none;

    if (definition_index != k_datum_index_none) {
        Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)definition_index].data;

        if ((force_create != 0 || (tag->flags & 4) == 0) && tag->events.count > 0) {
            handle = halo::memory::datum_new(effect_data);

            if (handle == k_datum_index_none) {
                if ((tag->flags & 4) != 0) {
                    handle = halo::memory::datum_next(-1, effect_data);
                    if (handle == k_datum_index_none) {
                        return k_datum_index_none;
                    }
                    for (;;) {
                        effect *candidate = &((effect *)effect_data->data)[(uint16_t)handle];
                        Effect *candidate_tag =
                            (Effect *)halo::cache::globals().tag_instances[(uint16_t)candidate->definition_index].data;

                        if ((candidate_tag->flags & 4) == 0) {
                            break;
                        }
                        handle = halo::memory::datum_next((int16_t)handle, effect_data);
                        if (handle == k_datum_index_none) {
                            return k_datum_index_none;
                        }
                    }
                    halo::memory::datum_delete(effect_data, handle);
                    handle = halo::memory::datum_new(effect_data);
                }
                if (handle == k_datum_index_none) {
                    return k_datum_index_none;
                }
            }

            {
                effect *self = &((effect *)effect_data->data)[(uint16_t)handle];

                self->definition_index = definition_index;
                self->creator_object_index = creator_object_index;
                self->first_person_weapon_index = -1;
                self->flags = 0;
                halo::effects::effect_start_event(handle, 0);
            }
        }
    }

    return handle;
}

/**
 * Creates an effect attached to `object_index` at an explicit 2D marker/texture coordinate,
 * binding it to every object marker the effect's locations name.
 *
 * @address 0x4506d0
 */
datum_index effect_ref::new_at_texture_coordinate(datum_index definition_index, datum_index object_index, int16_t change_color_index, int16_t u, int16_t v)
{
    datum_index handle = halo::effects::effect_new(definition_index, object_index, 1);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        int i;

        self->object_index = object_index;
        self->first_person_weapon_index = (int16_t)local_player_index_for_object(object_index);
        self->a_scale_function_index = u;
        self->b_scale_function_index = v;
        self->change_color_index = change_color_index;
        self->tint_source.proc = 0;
        self->tint_source.unknown_08 = 0;

        if (change_color_index == -1) {
            self->color = *global_white_color;
        }
        self->flags = self->flags | _effect_looping_bit;

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        halo::effects::effect_rebuild_markers(self, (effect_marker_resolver)(halo::objects::object_get_node_local_transform));
        if (self->first_person_weapon_index != -1) {
            halo::effects::effect_rebuild_markers(self, first_person_weapon_get_marker_data);
        }

        halo::effects::effect_update(handle, 0.0f);
    }

    return handle;
}

/**
 * Creates an effect on `object_index` with an explicit A/B scale range, marking it first-person
 * when first_person_effects_enabled and the local player's screen timer is active, and binding
 * it to every matching object (and, if applicable, first-person weapon) marker.
 *
 * @address 0x4507a0
 */
datum_index effect_ref::new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source)
{
    datum_index handle = halo::effects::effect_new(definition_index, creator_object_index, 1);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        int i;

        halo::effects::effect_set_placement(self, color, tint_source, a_scale, b_scale);
        self->object_index = object_index;
        self->first_person_weapon_index = (int16_t)local_player_index_for_object(object_index);

        if (first_person_effects_enabled != 0 && halo::effects::effect_first_person_screen_timer_active(object_index)) {
            self->flags = self->flags | _effect_first_person_bit;
        }

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        halo::effects::effect_rebuild_markers(self, (effect_marker_resolver)(halo::objects::object_get_node_local_transform));
        if (self->first_person_weapon_index != -1) {
            halo::effects::effect_rebuild_markers(self, first_person_weapon_get_marker_data);
        }

        if (first_person_weapon_override != -1) {
            self->first_person_weapon_index = first_person_weapon_override;
        }

        halo::effects::effect_update(handle, 0.0f);
    }

    return handle;
}

/**
 * Creates an effect on `object_index` bound to a specific per-object-type node table entry,
 * with an explicit A/B scale range.
 * Stack parameter order is Ghidra's own: node_index, then the four opaque marker context
 * dwords, then the two scales. creator_object_index / definition_index / object_index arrive
 * in EAX / ECX / EDX.
 *
 * @address 0x450870
 */
datum_index effect_ref::new_on_object_with_node_table(datum_index creator_object_index, datum_index definition_index, datum_index object_index, uint16_t node_index, uint16_t marker_count, uint32_t marker_names, uint32_t marker_positions, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source)
{
    datum_index handle = halo::effects::effect_new(definition_index, creator_object_index, 1);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        effect_marker_node_context context;
        object *attach_object;
        int i;

        halo::effects::effect_set_placement(self, color, tint_source, a_scale, b_scale);
        self->object_index = object_index;

        if (first_person_effects_enabled != 0 && halo::effects::effect_first_person_screen_timer_active(object_index)) {
            self->flags = self->flags | _effect_first_person_bit;
        }

        context.marker_count = marker_count;
        context.marker_names = marker_names;
        context.marker_forwards = marker_forwards;
        context.marker_positions = marker_positions;
        context.node_index = (node_index == 0xffff) ? 0 : node_index;

        attach_object = ((object_header *)object_data->data)[(uint16_t)object_index].data;
        context.node_table_entry = (int16_t)context.node_index * 0x34 +
            ((struct object *)attach_object)->nodes.offset + (int32_t)(long)attach_object;

        effect_marker_callback_context = &context;

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        halo::effects::effect_rebuild_markers(self, halo::effects::effect_marker_node_table_resolver);

        halo::effects::effect_update(handle, 0.0f);
    }

    return handle;
}

/**
 * Creates a free-standing effect (not attached to any object) at `position`, probing its BSP
 * location and defaulting its velocity to the global origin vector when `velocity` is NULL.
 * FIXED (objdump 0x450980..0x450a99): TWELVE stack arguments (every caller cleans 0x30). Arg 6 is the position:
 * it goes to bsp3d_node_find_leaf in EDX (0x4509e4 -> 0x450a18) and is also stored as marker-context +0x10;
 * args 8..11 are effect_set_placement's a/b scale (stack) and color (ECX) / tint_source (EDX) (0x4509b2..0x4509d6);
 * arg 12 is effect_new's force_create (0x450980).
 *
 * @address 0x450980
 */
datum_index effect_ref::new_with_color(datum_index definition_index, datum_index creator_object_index, const real_vector3d *velocity, uint16_t marker_count, uint32_t marker_names, real_point3d *position, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source, uint8_t force_create)
{
    datum_index handle = halo::effects::effect_new(definition_index, creator_object_index, force_create);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        effect_marker_node_context context;
        int32_t leaf;
        int i;

        halo::effects::effect_set_placement(self, color, tint_source, a_scale, b_scale);
        self->object_index = k_datum_index_none;

        context.marker_count = marker_count;
        context.marker_names = marker_names;
        context.marker_positions = (uint32_t)position;
        context.marker_forwards = marker_forwards;
        context.node_index = 0xffff;
        context.node_table_entry = 0;

        leaf = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, position);
        self->location.leaf_index = leaf;
        self->location.cluster_index = (leaf == -1) ? -1 :
            *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer + (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

        if (velocity == 0) {
            velocity = (const real_vector3d *)global_origin3d_pointer;
        }
        self->velocity = *velocity;

        effect_marker_callback_context = &context;

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        halo::effects::effect_rebuild_markers(self, halo::effects::effect_marker_node_table_resolver);

        halo::effects::effect_update(handle, 0.0f);
    }

    return handle;
}

}

namespace halo::effects {

datum_index effect_new(datum_index definition_index, datum_index creator_object_index, uint8_t force_create)
{
    return halo::effects::effect_ref::create(definition_index, creator_object_index, force_create);
}

datum_index effect_new_at_texture_coordinate(datum_index definition_index, datum_index object_index, int16_t change_color_index, int16_t u, int16_t v)
{
    return halo::effects::effect_ref::new_at_texture_coordinate(definition_index, object_index, change_color_index, u, v);
}

datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source)
{
    return halo::effects::effect_ref::new_on_object(creator_object_index, definition_index, object_index, first_person_weapon_override, a_scale, b_scale, color, tint_source);
}

datum_index effect_new_on_object_with_node_table(datum_index creator_object_index, datum_index definition_index, datum_index object_index, uint16_t node_index, uint16_t marker_count, uint32_t marker_names, uint32_t marker_positions, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source)
{
    return halo::effects::effect_ref::new_on_object_with_node_table(creator_object_index, definition_index, object_index, node_index, marker_count, marker_names, marker_positions, marker_forwards, a_scale, b_scale, color, tint_source);
}

datum_index effect_new_with_color(datum_index definition_index, datum_index creator_object_index, const real_vector3d *velocity, uint16_t marker_count, uint32_t marker_names, real_point3d *position, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source, uint8_t force_create)
{
    return halo::effects::effect_ref::new_with_color(definition_index, creator_object_index, velocity, marker_count, marker_names, position, marker_forwards, a_scale, b_scale, color, tint_source, force_create);
}

}
