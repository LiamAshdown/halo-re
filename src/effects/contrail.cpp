#include "halo/effects/effects.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern data_array *contrail_data;
extern data_array *contrail_point_data;
extern tag_instance *tag_instances;
extern random_seed effect_random_seed;
extern void datum_delete(data_array *array, datum_index handle);
extern data_array *object_data;
extern ScenarioStructureBSP *global_structure_bsp;
extern datum_index datum_new(data_array *array);
extern int32_t object_get_node_local_transform(uint32_t object_index, const char *marker_name, object_marker *marker, uint32_t flags);
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out, random_seed *seed, real lo, real hi);
extern datum_index datum_next(int16_t after_index, data_array *array);
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
}

namespace halo::effects {

/**
 * Advances one contrail by `delta_time` seconds: generates any points now due, optionally
 * detaches it from its owning object, and accumulates the elapsed time for contrail_update to
 * consume.
 *
 * @address 0x44ca60
 */
void contrail_ref::advance(uint8_t detach, real delta_time)
{
    datum_index contrail_handle = datum;
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];

    if ((self->flags & _contrail_emitting_bit) != 0) {
        int16_t due = halo::effects::contrail_points_due(contrail_handle, delta_time);
        halo::effects::contrail_generate_points(contrail_handle, due < 1 ? 1 : due, 0);
    }

    if (detach != 0) {
        self->object_index = k_datum_index_none;
    }

    self->accumulated_delta_time = self->accumulated_delta_time + delta_time;
}

/**
 * Advances the age/state machine of every point on all four of a contrail's point lists,
 * submitting a render segment for each live, non-transitional point, and trims fully-expired
 * runs from the tail of each list.
 *
 * @address 0x44d470
 */
void contrail_ref::age_points(real delta_time)
{
    datum_index contrail_handle = datum;
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
    ContrailPointState *states = (ContrailPointState *)tag->point_states.pointer;
    int32_t state_count = (int32_t)tag->point_states.count;
    int list;

    for (list = 0; list < 4; list++) {
        datum_index visited[1023];
        int16_t visited_count = 0;
        datum_index current = self->first_point[list];

        while (current != k_datum_index_none) {
            contrail_point *point = &((contrail_point *)contrail_point_data->data)[(uint16_t)current];

            if ((point->flags & _contrail_point_expired_bit) == 0) {
                point->age = point->age + delta_time * point->inverse_duration;

                for (;;) {
                    for (;;) {
                        if (point->inverse_duration != 0.0f && point->age <= 1.0f) {
                            goto render;
                        }
                        if ((point->flags & _contrail_point_in_transition_bit) == 0) {
                            break;
                        }
                        {
                            ContrailPointState *next_state = &states[point->state_index + 1];
                            real duration;

                            point->state_index = point->state_index + 1;
                            point->age = 0.0f;
                            duration = next_state->duration[0];
                            if ((next_state->scale_flags & 1) != 0) {
                                duration = point->scale * next_state->duration[0];
                            }
                            {
                                real span = next_state->duration[1] - next_state->duration[0];
                                if ((next_state->scale_flags & 2) != 0) {
                                    span = span * point->scale;
                                }
                                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                                duration = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * span + duration;
                            }
                            point->inverse_duration = duration;
                            if (duration != 0.0f) {
                                point->inverse_duration = 1.0f / duration;
                            }
                            point->flags = point->flags & ~_contrail_point_in_transition_bit;
                        }
                    }

                    if (state_count <= point->state_index + 1) {
                        break;
                    }

                    {
                        ContrailPointState *current_state = &states[point->state_index];
                        real duration;

                        point->age = 0.0f;
                        duration = current_state->transition_duration[0];
                        if ((current_state->scale_flags & 4) != 0) {
                            duration = current_state->transition_duration[0] * point->scale;
                        }
                        {
                            real span = current_state->transition_duration[1] - current_state->transition_duration[0];
                            if ((current_state->scale_flags & 8) != 0) {
                                span = span * point->scale;
                            }
                            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                            duration = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * span + duration;
                        }
                        point->inverse_duration = duration;
                        if (duration != 0.0f) {
                            point->inverse_duration = 1.0f / duration;
                        }
                        point->flags = point->flags | _contrail_point_in_transition_bit;
                    }
                }

                point->flags = point->flags | _contrail_point_expired_bit;
            }

render:
            if ((point->flags & _contrail_point_skip_render_bit) != 0) {
                point->flags = point->flags & ~_contrail_point_skip_render_bit;
            } else if ((point->flags & _contrail_point_expired_bit) == 0) {
                ContrailPointState *current_state = &states[point->state_index];

                if (current_state->physics.tag_id.index != 0xffff || current_state->physics.tag_id.id != 0xffff) {
                    halo::physics::point_physics_tick(&point->velocity, 0,
                        (PointPhysics *)tag_instances[current_state->physics.tag_id.index].data,
                        &point->location, 0xffffffff, &point->position, 0, 0, 0,
                        current_state->width * 0.5f, delta_time);
                }
            }

            visited[visited_count] = current;
            visited_count = visited_count + 1;
            current = point->next_point;
        }

        while (1 < visited_count) {
            contrail_point *tail = &((contrail_point *)contrail_point_data->data)[(uint16_t)visited[visited_count - 1]];
            contrail_point *before_tail = &((contrail_point *)contrail_point_data->data)[(uint16_t)visited[visited_count - 2]];

            if ((tail->flags & _contrail_point_expired_bit) == 0 ||
                (before_tail->flags & _contrail_point_expired_bit) == 0 ||
                tail->next_point != k_datum_index_none) {
                break;
            }

            before_tail->next_point = k_datum_index_none;
            self->point_count[list] = self->point_count[list] - 1;
            datum_delete(contrail_point_data, visited[visited_count - 1]);
            visited_count = visited_count - 1;
        }

        if (self->point_count[list] == 1 &&
            (((contrail_point *)contrail_point_data->data)[(uint16_t)self->first_point[list]].flags &
                _contrail_point_expired_bit) != 0) {
            datum_delete(contrail_point_data, self->first_point[list]);
            self->first_point[list] = k_datum_index_none;
            self->point_count[list] = 0;
        }
    }
}

/**
 * Frees every contrail_point on all four of a contrail's point lists, then deletes the contrail
 * itself.
 *
 * @address 0x44cad0
 */
void contrail_ref::destroy()
{
    datum_index contrail_index = datum;
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_index];
    int list;

    for (list = 0; list < 4; list++) {
        datum_index point = self->first_point[list];

        while (point != k_datum_index_none) {
            datum_index next = ((contrail_point *)contrail_point_data->data)[(uint16_t)point].next_point;

            datum_delete(contrail_point_data, point);
            point = next;
        }
    }

    datum_delete(contrail_data, contrail_index);
}

/**
 * Subdivides a contrail's marker attachment(s): for a list that has no points yet, adds a single
 * fresh point at the marker; for a list whose head point has fallen behind the marker's current
 * position (or when `force` is set), adds `point_count` new points interpolated between the old
 * head and the marker.
 *
 * @address 0x44d020
 */
void contrail_ref::generate_points(int16_t point_count, uint8_t force)
{
    datum_index contrail_handle = datum;
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;

    if (point_count == 0) {
        return;
    }

    {
        object *owner = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
        Object *owner_tag = (Object *)tag_instances[(uint16_t)owner->definition_tag].data;
        ObjectAttachment *attachment = (ObjectAttachment *)owner_tag->attachments.pointer +
            self->attachment_index;
        object_marker markers[4];
        int16_t marker_count = (int16_t)object_get_node_local_transform(self->object_index,
            attachment->marker.string, markers, 4);

        if (marker_count > 0) {
            real velocity_magnitude = halo::effects::effect_random_scaled_range(tag->scale_flags, self->scale,
                tag->point_velocity[0], tag->point_velocity[1], 1);
            real cone_angle = tag->point_velocity_cone_angle;
            real inherited_fraction = tag->inherited_velocity_fraction;
            int list;

            if ((tag->scale_flags & (1u << 3)) != 0) {
                cone_angle = cone_angle * self->scale;
            }
            if ((tag->scale_flags & (1u << 4)) != 0) {
                inherited_fraction = inherited_fraction * self->scale;
            }

            for (list = 0; list < marker_count; list++) {
                object_marker *marker = &markers[list];
                datum_index *head = &self->first_point[list];
                contrail_point *previous = 0;
                int16_t count;

                if (*head == k_datum_index_none) {
                    count = 1;
                } else {
                    contrail_point *current = &((contrail_point *)contrail_point_data->data)[(uint16_t)*head];
                    int unchanged = current->position.x == marker->node_transform.position.x &&
                                    current->position.y == marker->node_transform.position.y &&
                                    current->position.z == marker->node_transform.position.z;

                    if (unchanged && force == 0) {
                        continue;
                    }
                    previous = current;
                    count = point_count;
                }

                {
                    int i;
                    for (i = 1; i <= count; i++) {
                        datum_index new_handle = datum_new(contrail_point_data);

                        if (new_handle != k_datum_index_none) {
                            contrail_point *point =
                                &((contrail_point *)contrail_point_data->data)[(uint16_t)new_handle];
                            real_vector3d direction;

                            point->age = 0.0f;
                            point->inverse_duration = 0.0f;
                            point->flags = _contrail_point_skip_render_bit | _contrail_point_in_transition_bit;
                            point->state_index = -1;
                            point->scale = self->scale;

                            vector3d_randomize_direction((real_point3d *)&marker->node_transform.up,
                                &direction, &effect_random_seed, 0.0f, cone_angle);

                            point->position = marker->node_transform.position;

                            {
                                int32_t leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, &point->position);
                                point->location.leaf_index = leaf;
                                point->location.cluster_index = (leaf == -1) ? -1 :
                                    *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                                        (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
                            }

                            {
                                object *root = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                                while (root->parent_object != k_datum_index_none) {
                                    root = ((object_header *)object_data->data)[(uint16_t)root->parent_object].data;
                                }
                                point->velocity.i = direction.i * velocity_magnitude + inherited_fraction * root->velocity.i;
                                point->velocity.j = direction.j * velocity_magnitude + inherited_fraction * root->velocity.j;
                                point->velocity.k = direction.k * velocity_magnitude + inherited_fraction * root->velocity.k;
                            }

                            if (i < count) {
                                real fraction = (real)i / (real)count;
                                real inverse_fraction = 1.0f - fraction;
                                real_point3d sampled_position = point->position;
                                real_vector3d sampled_velocity = point->velocity;
                                int32_t leaf;

                                point->scale = fraction * point->scale + inverse_fraction * previous->scale;

                                leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, &point->position);
                                point->location.leaf_index = leaf;
                                point->location.cluster_index = (leaf == -1) ? -1 :
                                    *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                                        (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

                                point->position.x = fraction * sampled_position.x + inverse_fraction * previous->position.x;
                                point->position.y = fraction * sampled_position.y + inverse_fraction * previous->position.y;
                                point->position.z = fraction * sampled_position.z + inverse_fraction * previous->position.z;
                                point->velocity.i = fraction * sampled_velocity.i + inverse_fraction * previous->velocity.i;
                                point->velocity.j = fraction * sampled_velocity.j + inverse_fraction * previous->velocity.j;
                                point->velocity.k = fraction * sampled_velocity.k + inverse_fraction * previous->velocity.k;
                            }

                            point->next_point = *head;
                            self->point_count[list] = self->point_count[list] + 1;
                            *head = new_handle;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Creates a new contrail attached to `object_index` at attachment `attachment_index`, and
 * immediately generates its first point.
 *
 * @address 0x44c910
 */
datum_index contrail_ref::create(int16_t attachment_index, datum_index object_index, datum_index definition_index)
{
    datum_index handle = k_datum_index_none;

    if (definition_index != k_datum_index_none) {
        handle = datum_new(contrail_data);
        if (handle != k_datum_index_none) {
            contrail *self = &((contrail *)contrail_data->data)[(uint16_t)handle];
            object *owner = ((object_header *)object_data->data)[(uint16_t)object_index].data;
            Object *owner_tag = (Object *)tag_instances[(uint16_t)owner->definition_tag].data;
            ObjectAttachment *attachment = (ObjectAttachment *)owner_tag->attachments.pointer +
                attachment_index;

            self->flags = 0;
            self->unknown_03 = 0;
            self->definition_index = definition_index;
            self->object_index = object_index;
            self->attachment_index = attachment_index;
            self->scale_function_index = attachment->primary_scale - 1;
            self->sequence_index = (int16_t)0xffff;
            halo::effects::contrail_next_sequence(self);

            self->texture_offset_u = 0.0f;
            self->texture_offset_v = 0.0f;
            {
                int i;
                for (i = 0; i < 4; i++) {
                    self->point_count[i] = 0;
                    self->first_point[i] = k_datum_index_none;
                }
            }

            {
                int16_t scale_function_index = self->scale_function_index;
                object *root = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;

                if (scale_function_index == -1) {
                    self->scale = 1.0f;
                } else {
                    self->scale = root->function_out_values[scale_function_index];
                    if ((root->function_valid_flags & (1u << (scale_function_index & 0x1f))) == 0) {
                        return handle;
                    }
                }
            }

            self->flags |= _contrail_emitting_bit;
            halo::effects::contrail_generate_points(handle, 1, 1);
        }
    }

    return handle;
}

/**
 * VERIFIED against disassembly 0x44ced0..0x44cf76 (2026-09-30); fixed: the sequence range test is a SIGNED compare (a
 * huge/negative count rerolls instead of indexing the table) and the random scale ends in a logical shift.
 * Advances to the next frame of the current bitmap sequence, and once the sequence or frame runs
 * past the end of the Bitmap's sequence table, rerolls a new random sequence within
 * Contrail.first_sequence_index/sequence_count and resets frame_index to 0.
 *
 * @address 0x44ced0
 */
void contrail_ref::next_sequence(contrail *self)
{
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
    Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmap.tag_id.index].data;
    BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
    int16_t sequence_index;

    self->frame_index = self->frame_index + 1;
    sequence_index = self->sequence_index;
    self->animation_timer = 0.0f;

    if (sequence_index >= 0 && (int32_t)sequence_index < (int32_t)bitmap->bitmap_group_sequence.count &&
        self->frame_index >= 0 &&
        self->frame_index < (int16_t)sequences[sequence_index].bitmap_count) {
        return;
    }

    {
        int16_t first = tag->first_sequence_index;

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        self->sequence_index = (int16_t)((((uint32_t)(int32_t)(((int16_t)(tag->sequence_count + first) - first) *
            (int32_t)(effect_random_seed >> k_random_value_shift))) >> 16) + first);
        self->frame_index = 0;
    }
}

/**
 * Advances contrail->generation_timer by `elapsed_time` and returns how many new points are due,
 * reloading the timer with 1 / point_generation_rate (optionally scaled by contrail->scale) for
 * each one consumed.
 *
 * @address 0x44cf80
 */
int16_t contrail_ref::points_due(real elapsed_time)
{
    datum_index contrail_handle = datum;
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
    real rate = tag->point_generation_rate;
    int16_t count = 0;

    if ((tag->scale_flags & 1) != 0) {
        rate = rate * self->scale;
    }

    while (self->generation_timer <= elapsed_time) {
        real timer = self->generation_timer;
        self->generation_timer = 1.0f / rate;
        count = count + 1;
        elapsed_time = elapsed_time - timer;
    }
    self->generation_timer = self->generation_timer - elapsed_time;

    return count;
}

/**
 * Per-tick driver: advances every live contrail's emitting state, sequence/frame animation and
 * texture scroll, ages its points, and deletes it once it has no points left and no owning
 * object.
 *
 * @address 0x44cb50
 */
void contrail_ref::update(real delta_time)
{
    datum_index contrail_index = datum_next(-1, contrail_data);

    while (contrail_index != k_datum_index_none) {
        contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_index];
        Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
        real remaining = delta_time - self->accumulated_delta_time;

        self->accumulated_delta_time = 0.0f;

        if (self->object_index != k_datum_index_none) {
            int16_t scale_function_index = self->scale_function_index;
            uint8_t want_emitting;

            if (scale_function_index == -1) {
                self->scale = 1.0f;
                want_emitting = 1;
            } else {
                object *owner = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                self->scale = owner->function_out_values[scale_function_index];
                want_emitting = (owner->function_valid_flags & (1u << (scale_function_index & 0x1f))) != 0;
            }

            if (want_emitting != (uint8_t)(self->flags & _contrail_emitting_bit)) {
                real saved_scale = self->scale;
                self->scale = 0.0f;
                halo::effects::contrail_generate_points(contrail_index, 1, 1);
                self->scale = saved_scale;
            }

            if (!want_emitting) {
                self->flags = self->flags & ~_contrail_emitting_bit;
            } else {
                self->flags = self->flags | _contrail_emitting_bit;
                {
                    int16_t due = halo::effects::contrail_points_due(contrail_index, delta_time);
                    halo::effects::contrail_generate_points(contrail_index, due, 0);
                }
            }
        }

        {
            real animation_rate = tag->animation_rate;
            if ((tag->scale_flags & (1u << 5)) != 0) {
                animation_rate = animation_rate * self->scale;
            }

            while (remaining > 0.0f) {
                real time_to_next_frame = (1.0f / animation_rate) - self->animation_timer;
                if (time_to_next_frame > remaining) {
                    self->animation_timer = self->animation_timer + remaining;
                    break;
                }
                halo::effects::contrail_next_sequence(self);
                remaining = remaining - time_to_next_frame;
            }
        }

        {
            real u_rate = tag->texture_animation_u;
            if ((tag->scale_flags & (1u << 8)) != 0) {
                u_rate = u_rate * self->scale;
            }
            self->texture_offset_u = self->texture_offset_u - u_rate * remaining;
        }
        {
            real v_rate = tag->texture_animation_v;
            if ((tag->scale_flags & (1u << 9)) != 0) {
                v_rate = v_rate * self->scale;
            }
            self->texture_offset_v = self->texture_offset_v + v_rate * remaining;
        }

        halo::effects::contrail_age_points(contrail_index, delta_time);

        {
            int list;
            for (list = 0; list < 4 && self->first_point[list] == k_datum_index_none; list++) {
            }
            if (list == 4 && self->object_index == k_datum_index_none) {
                halo::effects::contrail_delete(contrail_index);
            }
        }

        contrail_index = datum_next((int16_t)contrail_index, contrail_data);
    }
}

/**
 * Registers the contrail and contrail-point datum tables. If only one of the two allocations
 * succeeds, that handle is cleared too, so the module is left fully enabled or fully disabled.
 *
 * @address 0x44c8b0
 */
void contrail_ref::initialize()
{
    contrail_data = (data_array *)game_state_new((char *)"contrail", k_maximum_contrails, sizeof(contrail));
    contrail_point_data = (data_array *)game_state_new((char *)"contrail point", k_maximum_contrail_points, sizeof(contrail_point));

    if (contrail_data == 0) {
        if (contrail_point_data != 0) {
            contrail_point_data = 0;
        }
    } else if (contrail_point_data == 0) {
        contrail_data = 0;
    }
}

}

namespace halo::effects {

void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time)
{
    halo::effects::contrail_ref(contrail_handle).advance(detach, delta_time);
}

void contrail_age_points(datum_index contrail_handle, real delta_time)
{
    halo::effects::contrail_ref(contrail_handle).age_points(delta_time);
}

void contrail_delete(datum_index contrail_index)
{
    halo::effects::contrail_ref(contrail_index).destroy();
}

void contrail_generate_points(datum_index contrail_handle, int16_t point_count, uint8_t force)
{
    halo::effects::contrail_ref(contrail_handle).generate_points(point_count, force);
}

datum_index contrail_new(int16_t attachment_index, datum_index object_index, datum_index definition_index)
{
    return halo::effects::contrail_ref::create(attachment_index, object_index, definition_index);
}

void contrail_next_sequence(contrail *self)
{
    halo::effects::contrail_ref::next_sequence(self);
}

int16_t contrail_points_due(datum_index contrail_handle, real elapsed_time)
{
    return halo::effects::contrail_ref(contrail_handle).points_due(elapsed_time);
}

void contrail_update(real delta_time)
{
    halo::effects::contrail_ref::update(delta_time);
}

void contrails_initialize()
{
    halo::effects::contrail_ref::initialize();
}

}
