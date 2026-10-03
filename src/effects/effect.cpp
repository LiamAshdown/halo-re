#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern data_array *effect_data;
extern data_array *effect_location_data;
extern data_array *object_data;
extern data_array *player_data;
extern uint8_t *first_person_weapon_interfaces;
extern effect_location_marker *effect_marker_next(effect *self, datum_index *marker, int32_t mode);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern game_time_globals *game_time;
extern void effect_delete(datum_index effect_handle);
extern void effect_start_event(datum_index effect_handle, int16_t event_index);
extern player_globals *local_player_globals;
extern uint32_t object_get_root_object_index(uint32_t object_index);
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value);
extern ScenarioStructureBSP *global_structure_bsp;
}

namespace halo::effects {

/**
 * Scans every effect that has not finished playing and whose Effect tag carries a nonzero
 * maximum_damage_radius, testing each of them against every player's controlled unit: for the
 * first (non first-person) location marker of the effect that resolves to a world point, if the
 * unit's bounding sphere is within (unit.bounding_radius + effect.maximum_damage_radius) of that
 * point the function returns true immediately. Returns false once every effect/player/marker
 * combination has been checked with no overlap.
 *
 * @address 0x450fa0
 */
uint32_t effect_ref::check_object_collisions()
{
    datum_index effect_handle;

    for (effect_handle = halo::memory::datum_next((int16_t)k_datum_index_none, effect_data);
         effect_handle != k_datum_index_none;
         effect_handle = halo::memory::datum_next((int16_t)effect_handle, effect_data)) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)effect_handle];

        if ((self->flags & _effect_finished_bit) != 0) {
            continue;
        }

        Effect *definition = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
        float damage_radius = definition->maximum_damage_radius;
        if (damage_radius == 0.0f) {
            continue;
        }

        {
            data_iterator player_iterator;
            player *p;

            player_iterator.data = player_data;
            player_iterator.next_index = 0;
            player_iterator.index = k_datum_index_none;
            player_iterator.signature = (uint32_t)(uintptr_t)player_iterator.data ^ k_data_iterator_signature;

            for (p = (player *)halo::memory::data_iterator_next(&player_iterator); p != (player *)0;
                 p = (player *)halo::memory::data_iterator_next(&player_iterator)) {
                if (p->unit == k_datum_index_none) {
                    continue;
                }

                object *unit = ((object_header *)object_data->data)[(uint16_t)p->unit].data;
                datum_index marker_handle = self->location_markers[0];

                while (marker_handle != k_datum_index_none) {
                    effect_location_marker *entry =
                        &((effect_location_marker *)effect_location_data->data)[(uint16_t)marker_handle];
                    marker_handle = entry->next_marker;

                    if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0) {
                        entry = halo::effects::effect_marker_next(self, &marker_handle, 0);
                    }
                    if (entry == (effect_location_marker *)0) {
                        break;
                    }

                    real_point3d point;
                    if (entry->marker_index == 0xffff) {
                        point = entry->transform.position;
                    } else {
                        real_matrix4x3 *node;
                        uint16_t node_index = entry->marker_index & 0x7fff;

                        if ((entry->marker_index & 0x8000) != 0) {
                            node = (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
                                self->first_person_weapon_index * 0x1ea0 + node_index * 0x34);
                        } else {
                            object *owner =
                                ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                            node = (real_matrix4x3 *)((uint8_t *)owner + owner->nodes.offset +
                                node_index * 0x34);
                        }

                        float x = entry->transform.position.x;
                        float y = entry->transform.position.y;
                        float z = entry->transform.position.z;
                        if (node->scale != 1.0f) {
                            x = x * node->scale;
                            y = y * node->scale;
                            z = z * node->scale;
                        }
                        point.x = x * node->forward.i + y * node->left.i + z * node->up.i +
                            node->position.x;
                        point.y = x * node->forward.j + y * node->left.j + z * node->up.j +
                            node->position.y;
                        point.z = x * node->forward.k + y * node->left.k + z * node->up.k +
                            node->position.z;
                    }

                    float combined_radius = unit->bounding_radius + damage_radius;
                    float dx = point.x - unit->bounding_center.x;
                    float dy = point.y - unit->bounding_center.y;
                    float dz = point.z - unit->bounding_center.z;
                    float distance_squared = dx * dx + dy * dy + dz * dz;

                    if (combined_radius * combined_radius >= distance_squared) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * Frees every effect_location_marker owned by an effect (across all of its Effect tag's
 * locations) and then deletes the effect itself.
 *
 * @address 0x450be0
 */
void effect_ref::destroy()
{
    datum_index effect_index = datum;
    effect *self = (effect *)halo::memory::datum_get(effect_index, effect_data);

    if (self != 0) {
        Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
        int32_t location_index;

        for (location_index = 0; location_index < (int32_t)tag->locations.count; location_index++) {
            datum_index marker_index = self->location_markers[location_index];

            while (marker_index != k_datum_index_none) {
                datum_index next =
                    ((effect_location_marker *)effect_location_data->data)[(uint16_t)marker_index].next_marker;

                halo::memory::datum_delete(effect_location_data, marker_index);
                marker_index = next;
            }
        }

        halo::memory::datum_delete(effect_data, effect_index);
    }
}

/**
 * UNSURE overall (see file header): whether object_index has flag bit 2 of the byte at +0x106
 * set, and if so, whether the linked index at +0x41c (offset by 0x1e ticks) is still within
 * k_game_tick globals's current tick.
 *
 * @address 0x450680
 */
uint8_t effect_ref::first_person_screen_timer_active(datum_index object_index)
{
    object *self = object_try_and_get(object_index, 1);

    if (self == 0 || (*((uint8_t *)self + 0x106) & 4) == 0) {
        return 0;
    }

    {
        int32_t linked = *(int32_t *)((uint8_t *)self + 0x41c);

        if (linked != -1) {
            linked = linked + 0x1e;
            if (linked < game_time->game_time) {
                return 1;
            }
        }
    }

    return 0;
}

/**
 * Begins event `event_index` on an effect: resets its elapsed time and started flag, and rolls
 * its duration from the tag's delay bounds using the deterministic global seed.
 *
 * @address 0x451660
 */
void effect_ref::start_event(int16_t event_index)
{
    datum_index effect_handle = datum;
    effect *self = (effect *)halo::memory::datum_get(effect_handle, effect_data);

    if (self != 0) {
        Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;

        if (event_index >= 0 && (uint32_t)event_index < tag->events.count) {
            EffectEvent *event = &((EffectEvent *)tag->events.pointer)[event_index];

            self->flags = self->flags & ~_effect_event_started_bit;
            self->event_index = event_index;
            self->event_time = 0.0f;
            self->event_duration = halo::math::random_real_range_seeded(*((tag->flags & 4) != 0 ? &halo::math::globals().random_seed_global
                : &halo::math::globals().effect_random_seed), event->delay_bounds[0], event->delay_bounds[1]);
        }
    }
}

/**
 * Stops a looping effect: deletes it outright if it was never looping, otherwise plays its
 * loop_stop_event (or, if there is none, marks it finished immediately). `stop_immediately`
 * records whether the stop event itself should skip straight to its own end.
 *
 * @address 0x450b20
 */
void effect_ref::stop(uint8_t stop_immediately)
{
    datum_index effect_handle = datum;
    effect *self = (effect *)halo::memory::datum_get(effect_handle, effect_data);

    if (self != 0) {
        Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;

        if ((self->flags & _effect_looping_bit) == 0) {
            halo::effects::effect_delete(effect_handle);
            return;
        }

        if (stop_immediately == 0) {
            self->flags = self->flags & ~_effect_stop_immediately_bit;
        } else {
            self->flags = self->flags | _effect_stop_immediately_bit;
        }

        if (tag->loop_stop_event < 0 || (int32_t)tag->events.count <= tag->loop_stop_event + 1) {
            self->flags = self->flags | _effect_finished_bit;
            return;
        }

        halo::effects::effect_start_event(effect_handle, (int16_t)(tag->loop_stop_event + 1));
        self->flags = self->flags | _effect_stopping_bit;
    }
}

/**
 * Resolves an effect handle to its record, or NULL if stale or out of range.
 *
 * @address 0x450630
 */
effect * effect_ref::try_and_get()
{
    datum_index effect_index = datum;
    return (effect *)halo::memory::datum_get(effect_index, effect_data);
}

/**
 * One roll of the effect's random stream (tag flag bit 2 selects the global one), 0..1.
 */
static real effect_update_roll_fraction(uint8_t *tag)
{
    random_seed *seed = (tag[0] & 4) ? &halo::math::globals().random_seed_global : &halo::math::globals().effect_random_seed;

    *seed = *seed * k_random_multiplier + k_random_increment;
    return (real)(*seed >> k_random_value_shift) * 1.5259022e-05f;
}

/**
 * Member form of the original effect_update: update.
 *
 * @address 0x451a30
 */
void effect_ref::update(real dt)
{
    datum_index effect_index = datum;
    effect *self = (effect *)((uint8_t *)effect_data->data + (effect_index & 0xffff) * 0xfc);
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data;
    uint8_t *events = *(uint8_t **)(tag + 0x38);
    int32_t event_count = *(int32_t *)(tag + 0x34);
    datum_index object_index = self->object_index;
    int16_t steps;

    if (object_index != k_datum_index_none) {
        uint8_t *obj = (uint8_t *)object_try_and_get(object_index, 0xffffffff);
        uint8_t *root;

        if (obj == 0) {
            halo::effects::effect_delete(effect_index);
            return;
        }
        root = *(uint8_t **)((uint8_t *)object_data->data +
            (object_get_root_object_index(object_index) & 0xffff) * 0xc + 8);
        if (*(uint32_t *)(root + 0x10) & 0x800) {
            *(uint32_t *)&((struct effect *)self)->location.leaf_index = *(uint32_t *)(root + 0x98);
            *(uint32_t *)&((struct effect *)self)->location.cluster_index = *(uint32_t *)(root + 0x9c);
            self->velocity = *(real_vector3d *)(root + 0x68);
        } else {
            ((struct effect *)self)->location.cluster_index = -1;
        }
        if (self->flags & 2) {
            if (object_function_get_value(object_index, self->a_scale_function_index, &self->a_scale)) {
                if (self->flags & 8) {
                    if (self->flags & 0x20) {
                        halo::effects::effect_delete(effect_index);
                    } else {
                        self->flags = (uint16_t)(self->flags & 0xfff7);
                        halo::effects::effect_start_event(effect_index, 0);
                    }
                }
            } else if (tag[0] & 1) {
                uint8_t *obj_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;
                int16_t i;

                for (i = 0; i < *(int32_t *)&((struct Object *)obj_tag)->attachments.count; i++) {
                    if (*(datum_index *)(obj + 0x14c + i * 4) == effect_index) {
                        *(datum_index *)(obj + 0x14c + i * 4) = k_datum_index_none;
                        break;
                    }
                }
                halo::effects::effect_delete(effect_index);
                return;
            } else if ((self->flags & 0xc) == 0) {
                halo::effects::effect_stop(effect_index, 0);
            }
            object_function_get_value(self->object_index, self->b_scale_function_index, &self->b_scale);
            if (self->change_color_index != -1) {
                self->color = *(ColorRGB *)(obj + 0x1b8 + self->change_color_index * 0xc);
            }
        }
    }

    {
        int16_t cluster = ((struct effect *)self)->location.cluster_index;
        uint8_t visible = 0;

        if (cluster != -1) {
            uint32_t *bits = (uint32_t *)((uint8_t *)local_player_globals + ((tag[0] & 4) ? 0x18 : 0x58));

            visible = (bits[cluster >> 5] & (1u << (cluster & 0x1f))) != 0;
        }
        if (visible) {
            if (self->flags & 0x10) {
                self->flags = (uint16_t)(self->flags & 0xffef);
            }
        } else if ((self->flags & 0x10) == 0) {
            if ((self->flags & 2) == 0) {
                halo::effects::effect_delete(effect_index);
                return;
            }
            self->flags = (uint16_t)(self->flags | 0x10);
        }
    }

    if (dt < 0.0f || dt != dt) {
        return;
    }
    for (steps = 0; ; steps++) {
        uint16_t flags = self->flags;
        uint8_t finished;
        real remaining;

        if ((flags & 8) || steps >= 8) {
            return;
        }
        remaining = self->event_duration - self->event_time;
        if (remaining <= dt) {
            dt = dt - remaining;
            finished = 1;
            self->event_time = self->event_duration;
        } else {
            finished = 0;
            self->event_time = self->event_time + dt;
            dt = -1.0f;
        }
        if (flags & 1) {
            if ((flags & 0x10) == 0) {
                halo::effects::effect_spawn_particles(self);
            }
            if (finished) {
                int16_t next;

                if ((self->flags & 2) && self->event_index == *(int16_t *)(tag + 6) &&
                    *(int16_t *)(tag + 4) != -1) {
                    next = *(int16_t *)(tag + 4);
                } else {
                    next = (int16_t)(self->event_index + 1);
                }
                while (next < event_count &&
                    effect_update_roll_fraction((uint8_t *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data) <
                        *(float *)(events + next * 0x44 + 4)) {
                    next++;
                }
                if (next >= event_count) {
                    if (self->flags & 2) {
                        self->flags = (uint16_t)(self->flags | 8);
                        return;
                    }
                    halo::effects::effect_delete(effect_index);
                    return;
                }
                halo::effects::effect_start_event(effect_index, next);
            }
        } else if (finished) {
            uint8_t *event = events + self->event_index * 0x44;
            int32_t particle;

            self->flags = (uint16_t)(flags | 1);
            self->event_time = 0.0f;
            self->previous_event_fraction = -1.0f;
            {
                real fraction = effect_update_roll_fraction((uint8_t *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data);

                self->event_duration = (*(float *)(event + 0x14) - *(float *)(event + 0x10)) * fraction +
                    *(float *)(event + 0x10);
            }
            for (particle = 0; particle < *(int32_t *)(event + 0x38); particle = (int16_t)(particle + 1)) {
                uint8_t *part = *(uint8_t **)(event + 0x3c) + particle * 0xe8;
                uint8_t count = (uint8_t)(int32_t)halo::effects::effect_property_random_value(5, self, *(uint32_t *)(part + 0xe0),
                    *(uint32_t *)(part + 0xe4), &halo::math::globals().effect_random_seed, (real)*(int16_t *)(part + 0x6c),
                    (real)*(int16_t *)(part + 0x6e));

                self->particle_counts[particle] = count;
                if (count > 6) {
                    self->particle_counts[particle] = (uint8_t)(int32_t)(((real)count - 6.0f) /
                        (real)*(int16_t *)((uint8_t *)local_player_globals + 0xc) + 6.0f);
                }
            }
            if ((self->flags & 0x10) == 0) {
                halo::effects::object_change_color_evaluate(self);
            }
        }
        if (!(dt >= 0.0f)) {
            return;
        }
    }
}

/**
 * Member form of the original effects_refresh_structure_locations: refresh structure locations.
 *
 * @address 0x450e80
 */
void effect_ref::refresh_structure_locations()
{
    datum_index handle;

    for (handle = halo::memory::datum_next(-1, effect_data); handle != k_datum_index_none;
         handle = halo::memory::datum_next((int16_t)handle, effect_data)) {
        effect *entry = (effect *)((uint8_t *)effect_data->data + (handle & 0xffff) * 0xfc);
        datum_index marker;
        effect_location_marker *location;
        uint32_t leaf;

        if (entry->object_index != k_datum_index_none) {
            continue;
        }
        marker = entry->location_markers[0];
        location = halo::effects::effect_marker_next(entry, &marker, 0);
        if (location == 0) {
            halo::effects::effect_delete(handle);
            continue;
        }
        leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, (real_point3d *)((uint8_t *)location + 0x30));
        entry->location.leaf_index = (int32_t)leaf;
        if (leaf == 0xffffffff) {
            entry->location.cluster_index = -1;
        } else {
            entry->location.cluster_index =
                *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
        }
    }
}

/**
 * Per-tick driver: advances every live effect by `delta_time`.
 *
 * @address 0x450aa0
 */
void effect_ref::update_all(real delta_time)
{
    datum_index effect_index = halo::memory::datum_next(-1, effect_data);

    while (effect_index != k_datum_index_none) {
        halo::effects::effect_update(effect_index, delta_time);
        effect_index = halo::memory::datum_next((int16_t)effect_index, effect_data);
    }
}

}

namespace halo::effects {

uint32_t effect_check_object_collisions()
{
    return halo::effects::effect_ref::check_object_collisions();
}

void effect_delete(datum_index effect_index)
{
    halo::effects::effect_ref(effect_index).destroy();
}

uint8_t effect_first_person_screen_timer_active(datum_index object_index)
{
    return halo::effects::effect_ref::first_person_screen_timer_active(object_index);
}

void effect_start_event(datum_index effect_handle, int16_t event_index)
{
    halo::effects::effect_ref(effect_handle).start_event(event_index);
}

void effect_stop(datum_index effect_handle, uint8_t stop_immediately)
{
    halo::effects::effect_ref(effect_handle).stop(stop_immediately);
}

effect * effect_try_and_get(datum_index effect_index)
{
    return halo::effects::effect_ref(effect_index).try_and_get();
}

void effect_update(datum_index effect_index, real dt)
{
    halo::effects::effect_ref(effect_index).update(dt);
}

void effects_refresh_structure_locations()
{
    halo::effects::effect_ref::refresh_structure_locations();
}

void effects_update_all(real delta_time)
{
    halo::effects::effect_ref::update_all(delta_time);
}

}
