#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern data_array *player_data;
extern int16_t light_count_enabled;
extern const real_vector3d *global_origin3d_pointer;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag);
extern void damage_apply_area_effect(damage_data *dd);
extern datum_index light_new_positioned(datum_index light_tag, int32_t marker_index, int16_t marker_sub_index, real_point3d *position, uint32_t param_5, real_vector3d *direction);
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role);
extern datum_index object_new(object_placement_data *placement);
extern void effect_random_velocity_vector(effect *self, random_seed *seed, real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset);
extern void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min, real max, effect *self, uint32_t a_bitset, uint32_t b_bitset);
extern datum_index particle_system_new_at_point(uint32_t definition_index, real_point3d *position, real_vector3d *velocity, ColorARGB *color, float scale);
extern void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin, real_vector3d *direction, real radius, int32_t marker_index);
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint);
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
extern const ColorRGB *global_white_color;
extern data_array *effect_location_data;
extern data_array *object_data;
extern const real_vector3d *global_down3d_pointer;
extern effect_location_marker *effect_marker_next(effect *self, datum_index *marker, int32_t mode);
extern real_matrix4x3 *effect_resolve_marker_transform(effect *self, int16_t marker);
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out);
extern void effect_event_apply(effect *self, EffectPart *part, effect_location_marker *marker, real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale);
real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset, uint32_t b_bitset, random_seed *seed, real base_min, real base_max);
void effect_set_placement(effect *self, const ColorRGB *color, const effect_tint_source *tint_source, real a_scale, real b_scale);
void object_change_color_evaluate(effect *self);
}

namespace halo::effects {

#define PART_FIELD(type, offset) (*(type *)((uint8_t *)part + (offset)))

#define SELF_FIELD(type, offset) (*(type *)((uint8_t *)self + (offset)))

/**
 * File-local helper used by effect_event_apply.
 */
static int32_t effect_event_apply_marker_index(effect_location_marker *marker)
{
    if (marker->marker_index == 0xffff) {
        return -1;
    }
    return marker->marker_index & 0x7fff;
}

/**
 * Member form of the original effect_event_apply: event apply.
 *
 * @address 0x452cf0
 */
void effect_view::event_apply(EffectPart *part, effect_location_marker *marker, real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale)
{
    effect * self = record;
    uint32_t group = part->type_class;
    datum_index tag = PART_FIELD(datum_index, 0x24);
    real_point3d *marker_position = (real_point3d *)((uint8_t *)marker + 0x30);
    real_vector3d *marker_forward = (real_vector3d *)((uint8_t *)marker + 0x0c);

    if (group == 0x6c696768u) {
        if (light_count_enabled > 0) {
            light_new_positioned(tag, (int32_t)SELF_FIELD(datum_index, 0x3c),
                (int16_t)effect_event_apply_marker_index(marker), marker_position, *(uint32_t *)&scale,
                marker_forward);
        }
    } else if (group == 0x6a707421u) {
        damage_data dd;
        uint8_t *raw = (uint8_t *)&dd;
        uint8_t *creator = (uint8_t *)object_try_and_get(SELF_FIELD(datum_index, 0x40), 0xffffffff);

        damage_data_initialize(&dd, tag);
        if (creator != 0) {
            *(uint32_t *)(raw + 0x08) = ((struct object *)creator)->owner_linkage;
            *(datum_index *)(raw + 0x0c) = SELF_FIELD(datum_index, 0x40);
            *(int16_t *)(raw + 0x10) = ((struct object *)creator)->owner_team;
        }
        *(real *)(raw + 0x40) = scale;
        *(uint32_t *)(raw + 0x14) = SELF_FIELD(uint32_t, 0x10);
        *(uint32_t *)(raw + 0x18) = SELF_FIELD(uint32_t, 0x14);
        *(real_point3d *)(raw + 0x28) = *position;
        *(real_point3d *)(raw + 0x1c) = *position;
        *(real_vector3d *)(raw + 0x34) = *forward;
        damage_apply_area_effect(&dd);
    } else if (group == 0x64656361u) {
        real_vector3d out_direction;
        real_vector3d direction;
        real radius;

        effect_random_velocity_vector(self, &halo::math::globals().effect_random_seed, forward, &out_direction, &direction,
            PART_FIELD(real, 0x40), PART_FIELD(real, 0x44), PART_FIELD(real, 0x48), PART_FIELD(uint32_t, 0x60),
            PART_FIELD(uint8_t, 0x64));
        radius = halo::math::random_range_real(PART_FIELD(real, 0x54), PART_FIELD(real, 0x58));
        decal_spawn_for_response(tag, 0, position, &direction, radius, -1);
    } else if (group == 0x6f626a65u) {
        object_placement_data placement;
        uint8_t *raw = (uint8_t *)&placement;
        real_vector3d out_direction;
        real_vector3d *velocity = (real_vector3d *)(raw + 0x28);

        object_placement_data_initialize(&placement, tag, SELF_FIELD(datum_index, 0x40));
        *(real_point3d *)(raw + 0x18) = *position;
        *(real_vector3d *)(raw + 0x34) = *forward;
        *(real_vector3d *)(raw + 0x40) = *up;
        effect_random_velocity_vector(self, &halo::math::globals().random_seed_global, forward, &out_direction, velocity,
            PART_FIELD(real, 0x40), PART_FIELD(real, 0x44), PART_FIELD(real, 0x48), PART_FIELD(uint32_t, 0x60),
            PART_FIELD(uint8_t, 0x64));
        velocity->i = velocity->i + SELF_FIELD(real, 0x24);
        velocity->j = velocity->j + SELF_FIELD(real, 0x28);
        velocity->k = velocity->k + SELF_FIELD(real, 0x2c);
        effect_random_direction_vector(&halo::math::globals().random_seed_global, (real_point3d *)(raw + 0x4c), PART_FIELD(real, 0x4c),
            PART_FIELD(real, 0x50), self, PART_FIELD(uint32_t, 0x60), PART_FIELD(uint32_t, 0x64));
        object_new(&placement);
    } else if (group == 0x7063746cu) {
        ColorARGB color;
        real_vector3d out_direction;
        real_vector3d velocity;

        color.alpha = 1.0f;
        color.red = SELF_FIELD(real, 0x18);
        color.green = SELF_FIELD(real, 0x1c);
        color.blue = SELF_FIELD(real, 0x20);
        effect_random_velocity_vector(self, &halo::math::globals().effect_random_seed, forward, &out_direction, &velocity,
            PART_FIELD(real, 0x40), PART_FIELD(real, 0x44), PART_FIELD(real, 0x48), PART_FIELD(uint32_t, 0x60),
            PART_FIELD(uint8_t, 0x64));
        velocity.i = velocity.i + SELF_FIELD(real, 0x24);
        velocity.j = velocity.j + SELF_FIELD(real, 0x28);
        velocity.k = velocity.k + SELF_FIELD(real, 0x2c);
        particle_system_new_at_point(tag, position, &velocity, &color, scale);
    } else if (group == 0x736e6421u) {
        datum_index object_index = SELF_FIELD(datum_index, 0x3c);

        if (object_index != k_datum_index_none) {
            uint8_t first_person = 0;
            uint8_t *creator = (uint8_t *)object_try_and_get(SELF_FIELD(datum_index, 0x40), 3);

            if (creator != 0) {
                uint8_t *owner = (uint8_t *)halo::memory::datum_get(*(datum_index *)(creator + 0x218), player_data);

                if (owner != 0 && ((struct player *)owner)->local_player_index != -1) {
                    first_person = 1;
                }
            }
            sound_start_at_object_marker(object_index, (Point3D *)marker_position, (Vector3D *)marker_forward, tag,
                (int16_t)effect_event_apply_marker_index(marker), scale, first_person);
        } else {
            sound_placement placement;

            placement.position = *(Point3D *)position;
            placement.forward = *(Vector3D *)forward;
            placement.velocity = *(const Vector3D *)global_origin3d_pointer;
            *(uint32_t *)&placement.leaf_index = SELF_FIELD(uint32_t, 0x10);
            *(uint32_t *)&placement.cluster_index = SELF_FIELD(uint32_t, 0x14);
            sound_start_at_location(tag, &placement, scale);
        }
    }
}

#undef PART_FIELD

#undef SELF_FIELD

/**
 * Member form of the original effect_property_random_value: property random value.
 *
 * @address 0x451290
 */
real effect_view::property_random_value(uint8_t bit_index, uint32_t a_bitset, uint32_t b_bitset, random_seed *seed, real base_min, real base_max)
{
    effect * self = record;
    real lower = base_min;
    real span;
    uint32_t mask = 1u << (bit_index & 0x1f);

    if ((a_bitset & mask) != 0) {
        lower = base_min * self->a_scale;
    }
    if ((b_bitset & mask) != 0) {
        lower = lower * self->b_scale;
    }

    span = base_max - base_min;
    mask = 1u << ((bit_index + 1) & 0x1f);
    if ((a_bitset & mask) != 0) {
        span = span * self->a_scale;
    }
    if ((b_bitset & mask) != 0) {
        span = span * self->b_scale;
    }

    *seed = *seed * k_random_multiplier + k_random_increment;
    return (real)(*seed >> k_random_value_shift) * 1.5259022e-05f * span + lower;
}

/**
 * Sets an effect's per-instance placement inputs: the A/B scale values, its tint colour
 * (defaulting to *global_white_color when `color` is NULL), and its optional
 * per-position tint source callback (cleared, except for its data pointer, when `tint_source` is
 * NULL).
 *
 * @address 0x451600
 */
void effect_view::set_placement(const ColorRGB *color, const effect_tint_source *tint_source, real a_scale, real b_scale)
{
    effect * self = record;
    self->a_scale = a_scale;
    self->b_scale = b_scale;

    if (color == 0) {
        color = global_white_color;
    }
    self->color = *color;

    if (tint_source != 0) {
        self->tint_source = *tint_source;
    } else {
        self->tint_source.proc = 0;
        self->tint_source.unknown_08 = 0;
    }
}

/**
 * For every EffectLocation whose location index resolves and whose part's environment/violence
 * gate passes, walks that location's marker list and applies each qualifying EffectPart:
 * resolving the marker's world placement (either straight from the marker transform for the
 * object-origin sentinel, or rotated through the attached node/first-person-weapon transform
 * otherwise, or a hard-coded down/forward pair when the part is flagged
 * face_down_regardless_of_location_decals), then dispatching through effect_event_apply.
 * FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; self arrive(s) on the stack (1 stack argument(s)).
 *
 * @address 0x4529d0
 */
void effect_view::change_color_evaluate()
{
    effect * self = record;
    Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    EffectEvent *event = &((EffectEvent *)tag->events.pointer)[self->event_index];
    EffectPart *parts = (EffectPart *)event->parts.pointer;
    int32_t part_index;

    for (part_index = 0; part_index < (int32_t)event->parts.count; part_index++) {
        EffectPart *part = &parts[part_index];
        int16_t location = part->location;

        if (location >= 0 && (int32_t)location < (int32_t)tag->locations.count &&
            (part->type.tag_id.index != 0xffff || part->type.tag_id.id != 0xffff)) {
            uint8_t skip_part;

            if ((self->flags & _effect_first_person_bit) == 0) {
                skip_part = (part->violence_mode == 2);
            } else {
                skip_part = (part->violence_mode == 1);
            }

            if (!skip_part) {
                datum_index marker_handle = self->location_markers[location];
                uint8_t create_ok = 1;

                while (marker_handle != k_datum_index_none) {
                    effect_location_marker *entry =
                        &((effect_location_marker *)effect_location_data->data)[(uint16_t)marker_handle];
                    marker_handle = entry->next_marker;

                    if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0) {
                        entry = effect_marker_next(self, &marker_handle, 0);
                    }
                    if (entry == (effect_location_marker *)0) {
                        break;
                    }

                    real_vector3d placement[3];

                    if (entry->marker_index == 0xffff) {
                        placement[0] = entry->transform.up;
                        placement[1] = entry->transform.forward;
                        *(real_point3d *)&placement[2] = entry->transform.position;
                    } else {
                        real_matrix4x3 *node = effect_resolve_marker_transform(self, (int16_t)entry->marker_index);
                        real forward_i = entry->transform.forward.i;
                        real forward_j = entry->transform.forward.j;
                        real forward_k = entry->transform.forward.k;
                        real up_i = entry->transform.up.i;
                        real up_j = entry->transform.up.j;
                        real up_k = entry->transform.up.k;

                        placement[1].i = forward_j * node->left.i + forward_k * node->up.i + forward_i * node->forward.i;
                        placement[1].j = forward_i * node->left.j + forward_j * node->up.j + forward_k * node->forward.j;
                        placement[1].k = forward_j * node->left.k + forward_k * node->up.k + forward_i * node->forward.k;
                        placement[0].i = up_j * node->left.i + up_k * node->up.i + up_i * node->forward.i;
                        placement[0].j = up_i * node->left.j + up_j * node->up.j + up_k * node->forward.j;
                        placement[0].k = up_j * node->left.k + up_k * node->up.k + up_i * node->forward.k;

                        halo::math::matrix4x3_transform_point(*((real_point3d *)&placement[2]),
                            entry->transform.position, *node);
                    }

                    if ((part->flags & 1) != 0) {
                        placement[1] = *global_down3d_pointer;
                        placement[0] = *halo::math::globals().global_forward3d_pointer;
                    }

                    switch (part->create_in) {
                    case effectcreatein_any_environment:
                        create_ok = 1;
                        break;
                    case effectcreatein_air_only:
                        create_ok = !scenario_location_get_water_and_weather((real_point3d *)&placement[2], &self->location, 0);
                        break;
                    case effectcreatein_water_only:
                        create_ok = scenario_location_get_water_and_weather((real_point3d *)&placement[2], &self->location, 0);
                        break;
                    case effectcreatein_space_only:
                        create_ok = 0;
                        break;
                    default:
                        create_ok = 0;
                        break;
                    }

                    if (create_ok) {
                        real scale = 1.0f;
                        if ((part->a_scales_values & 0x20) != 0) {
                            scale = self->a_scale;
                        }
                        if ((part->b_scales_values & 0x20) != 0) {
                            scale = scale * self->b_scale;
                        }
                        effect_event_apply(self, part, entry, &placement[0], &placement[1],
                            (real_point3d *)&placement[2], scale);
                    }
                }
            }
        }
    }
}

}

extern "C" {

void effect_event_apply(effect *self, EffectPart *part, effect_location_marker *marker, real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale)
{
    halo::effects::effect_view(self).event_apply(part, marker, up, forward, position, scale);
}

real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset, uint32_t b_bitset, random_seed *seed, real base_min, real base_max)
{
    return halo::effects::effect_view(self).property_random_value(bit_index, a_bitset, b_bitset, seed, base_min, base_max);
}

void effect_set_placement(effect *self, const ColorRGB *color, const effect_tint_source *tint_source, real a_scale, real b_scale)
{
    halo::effects::effect_view(self).set_placement(color, tint_source, a_scale, b_scale);
}

void object_change_color_evaluate(effect *self)
{
    halo::effects::effect_view(self).change_color_evaluate();
}

}
