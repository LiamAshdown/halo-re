#include "halo/objects/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/ai/actor_props.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"

namespace c_actor_allocate_paired_prop {
}


/**
 * actor_allocate_paired_prop: behaviour unchanged from the original routine.
 *
 * @address 0x43e910
 */
datum_index halo::ai::prop_ops::allocate_paired_prop(datum_index existing_prop)
{
    using namespace c_actor_allocate_paired_prop;
    datum_index actor_index = datum;
    datum_index new_prop = halo::memory::datum_new(halo::ai::globals().prop_data);

    halo::ai::actor_init_prop_from_object(k_datum_index_none, actor_index, new_prop);
    if (new_prop != k_datum_index_none) {
        prop *existing = halo::ai::prop_at(existing_prop);
        prop *created = halo::ai::prop_at(new_prop);

        halo::ai::actor_copy_prop_and_reset(new_prop, existing_prop);
        existing->pair_index = new_prop;
        created->pair_index = existing_prop;
    }
    return new_prop;
}

namespace halo::ai {
datum_index actor_allocate_paired_prop(datum_index actor_index, datum_index existing_prop)
{
    return halo::ai::prop_ops(actor_index).allocate_paired_prop(existing_prop);
}
}

namespace c_actor_allocate_paired_prop_with_kind {
}


/**
 * actor_allocate_paired_prop_with_kind: behaviour unchanged from the original routine.
 *
 * @address 0x43e980
 */
datum_index halo::ai::prop_ops::allocate_paired_prop_with_kind(datum_index existing_prop, datum_index reference_prop)
{
    using namespace c_actor_allocate_paired_prop_with_kind;
    datum_index actor_index = datum;
    datum_index new_prop = halo::memory::datum_new(halo::ai::globals().prop_data);

    halo::ai::actor_init_prop_from_object(k_datum_index_none, actor_index, new_prop);
    if (new_prop == k_datum_index_none) {
        return k_datum_index_none;
    }
    {
        prop *existing = halo::ai::prop_at(existing_prop);
        prop *created = halo::ai::prop_at(new_prop);
        prop *reference = halo::ai::prop_at(reference_prop);
        int16_t kind;

        halo::ai::actor_copy_prop_and_reset(new_prop, reference_prop);
        existing->pair_index = new_prop;
        created->pair_index = existing_prop;
        kind = reference->state;
        if (kind >= 4 && kind <= 5) {
            created->state = kind;
        }
    }
    return new_prop;
}

namespace halo::ai {
datum_index actor_allocate_paired_prop_with_kind(datum_index actor_index, datum_index existing_prop, datum_index reference_prop)
{
    return halo::ai::prop_ops(actor_index).allocate_paired_prop_with_kind(existing_prop, reference_prop);
}
}

namespace c_actor_apply_unit_definition_properties {
static auto &object_type_definitions = halo::link::ref<object_type_definition *[12]>(halo::game::vars().object_type_definitions);


static uint8_t *object_get(datum_index object_index)
{
    return reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index));
}

static datum_index actor_create_unit_item(datum_index definition_tag, datum_index unit_index)
{
    object_placement_data placement;
    uint32_t role = 3;

    halo::objects::object_placement_data_initialize(&placement, definition_tag, unit_index);
    if (halo::networking::globals().game_mode == 2) {
        int16_t type = halo::ai::tag_data<Object>(placement.definition_tag)->object_type;

        if (object_type_definitions[type]->network_delta_message_type != -1) {
            role = 0;
        }
    }
    return halo::objects::object_new_with_datum_role_control(&placement, role);
}
}


/**
 * actor_apply_unit_definition_properties: behaviour unchanged from the original routine.
 *
 * @address 0x426cf0
 */
void halo::ai::prop_ops::apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index)
{
    using namespace c_actor_apply_unit_definition_properties;
    ActorVariant *variant = halo::ai::tag_data<ActorVariant>(actor_variant_tag);
    unit_object *unit = (unit_object *)object_get(unit_index);
    Unit *unit_tag = halo::ai::tag_data<Unit>(halo::ai::tag_handle(variant->actor_definition));
    int16_t i;

    if (variant->body_vitality > 0.0f || variant->shield_vitality > 0.0f) {
        halo::objects::object_initialize_shield_stun_thresholds(unit_index, &variant->body_vitality, &variant->shield_vitality);
    }
    if (static_cast<int16_t>(variant->forced_shader_permutation) != 0) {
        ((struct unit_object *)unit)->base.forced_shader_permutation = static_cast<uint16_t>(static_cast<int16_t>(variant->forced_shader_permutation));
    }
    for (i = 0; i < static_cast<int32_t>(variant->change_colors.count); i++) {
        ActorVariantChangeColors *change_color = &halo::ai::reflexive_data<ActorVariantChangeColors>(variant->change_colors)[i];

        if (i < 4) {
            ColorRGB *working = &unit->base.base_change_colors[i];

            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            halo::bitmaps::color_interpolate((ColorRGB *)&change_color->color_upper_bound, (ColorRGB *)&change_color->color_lower_bound, working, (color_interpolation_flags)1,
                (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f);
            *(ColorRGB *)&unit->base.change_colors[i] = *working;
        }
    }
    if (halo::ai::tag_handle(variant->weapon) != k_datum_index_none) {
        datum_index weapon = actor_create_unit_item(halo::ai::tag_handle(variant->weapon), unit_index);

        if (weapon != k_datum_index_none && !halo::units::unit_pickup_weapon(2, weapon, unit_index)) {
            int32_t role = ((object *)object_get(weapon))->network_role;

            if (role == 0) {
                halo::objects::object_delete_unparented(weapon);
                halo::objects::object_delete_recursive(weapon, 0);
            } else if (role == 3) {
                halo::objects::object_delete_recursive(weapon, 0);
            }
        }
    }
    if (variant->grenade_type != -1) {
        int16_t type = variant->grenade_type;
        int16_t minimum = variant->grenade_count[0];
        int32_t range = (int16_t)(variant->grenade_count[1] + 1) - minimum;
        unit_data *unit_state = halo::units::unit_data_of(object_get(unit_index));

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        unit_state->grenade_counts[type] = (int8_t)(unit_state->grenade_counts[type] +
            (uint8_t)(((uint32_t)range * (halo::math::globals().random_seed_global >> 0x10)) >> 0x10) + (uint8_t)minimum);
        unit_state->desired_grenade_index = (int8_t)type;
        unit_state->current_grenade_index = (int8_t)type;
    }
    if (halo::ai::tag_handle(variant->equipment) != k_datum_index_none) {
        int16_t equipment_kind = halo::ai::tag_data<Equipment>(halo::ai::tag_handle(variant->equipment))->powerup_type;

        if (equipment_kind != 0 && equipment_kind != 6) {
            datum_index equipment = actor_create_unit_item(halo::ai::tag_handle(variant->equipment), unit_index);

            if (equipment != k_datum_index_none && !halo::units::unit_try_select_equipment(unit_index, equipment, 1)) {
                halo::objects::object_delete(equipment);
            }
        }
    }
    if (variant->flags & 0x30) {
        if (halo::ai::flag_set(variant->flags, halo::tags::actor_variant_tag_flag::super_active_camouflage)) {
            ((unit_object *)unit)->unit.flags |= halo::to_bits(halo::units::unit_flag::super_camouflaged);
        }
        ((unit_object *)unit)->unit.flags |= halo::to_bits(halo::units::unit_flag::active_camouflaged);
        ((struct unit_object *)unit)->unit.active_camouflage_power = 1.0f;
        ((struct unit_object *)unit)->unit.super_active_camouflage_power = (static_cast<uint8_t>(unit_tag->base.object_type) & 0x20) ? 1.0f : 0.0f;
    }
}

namespace halo::ai {
void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index)
{
    halo::ai::prop_ops::apply_unit_definition_properties(actor_variant_tag, unit_index);
}
}

namespace c_actor_clear_perceived_props {
}


/**
 * actor_clear_perceived_props: behaviour unchanged from the original routine.
 *
 * @address 0x427e00
 */
void halo::ai::prop_ops::clear_perceived_props()
{
    using namespace c_actor_clear_perceived_props;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    while (self->first_prop != (datum_index)k_datum_index_none) {
        datum_index prop_index = self->first_prop;
        prop *p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];

        (void)p;
        halo::ai::actor_replace_object_reference(actor_index, halo::k_dword_none, prop_index);
        halo::ai::actor_unlink_prop(actor_index, prop_index);
        halo::memory::datum_delete(halo::ai::globals().prop_data, prop_index);
    }
}

namespace halo::ai {
void actor_clear_perceived_props(datum_index actor_index)
{
    halo::ai::prop_ops(actor_index).clear_perceived_props();
}
}

namespace c_actor_clear_recognition_history {
}


/**
 * actor_clear_recognition_history: behaviour unchanged from the original routine.
 *
 * @address 0x414140
 */
void halo::ai::prop_ops::clear_recognition_history(uint8_t keep_when_typed)
{
    using namespace c_actor_clear_recognition_history;
    datum_index actor_index = datum;
    actor *self;
    int i;

    self = halo::ai::actor_at(actor_index);

    self->recognition_cursor = 0;
    for (i = 0; i < 4; i++) {
        self->recognition[i].firing_position_index = -1;
    }

    if (self->recognition_valid != 0 && (keep_when_typed == 0 || self->recognition_type != 0)) {
        self->recognition_valid = 0;
    }
}

namespace halo::ai {
void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed)
{
    halo::ai::prop_ops(actor_index).clear_recognition_history(keep_when_typed);
}
}

namespace c_actor_copy_prop_and_reset {
static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
}


/**
 * actor_copy_prop_and_reset: behaviour unchanged from the original routine.
 *
 * @address 0x43e840
 */
void halo::ai::prop_ops::copy_prop_and_reset(datum_index dest_prop, datum_index src_prop)
{
    using namespace c_actor_copy_prop_and_reset;
    prop *dest = halo::ai::prop_at(dest_prop);
    prop *src = halo::ai::prop_at(src_prop);

    int16_t identifier = dest->identifier;
    datum_index actor_index = dest->actor_index;
    datum_index next_in_actor = dest->next_in_actor;
    datum_index pair_index = dest->pair_index;

    *dest = *src;

    dest->identifier = identifier;
    dest->pair_index = pair_index;
    dest->actor_index = actor_index;
    dest->next_in_actor = next_in_actor;

    dest->state = 4;
    dest->orphan_timer = 900;
    dest->inspection_ticks = 0;
    dest->noticed_a = 0;
    dest->noticed_b = 0;
    dest->noticed_c = 0;
    {
        float dx = dest->last_known_position.x - dest->last_perceived_position.x;
        float dy = dest->last_known_position.y - dest->last_perceived_position.y;
        float dz = dest->last_known_position.z - dest->last_perceived_position.z;
        dest->perceived_to_known_delta.i = dx;
        dest->perceived_to_known_delta.j = dy;
        dest->perceived_to_known_delta.k = dz;
    }
    dest->velocity = *global_origin3d_pointer;
    dest->speed_class = 0;
}

namespace halo::ai {
void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop)
{
    halo::ai::prop_ops::copy_prop_and_reset(dest_prop, src_prop);
}
}

namespace c_actor_danger_register_point {
}


/**
 * actor_danger_register_point: behaviour unchanged from the original routine.
 *
 * @address 0x41ec90
 */
uint8_t halo::ai::prop_ops::danger_register_point(datum_index source_object_index, float radius, float distance, char accept_flag, uint8_t unknown_byte)
{
    using namespace c_actor_danger_register_point;
    datum_index actor_index = datum;
    actor *self;
    object *source_obj;
    float threshold;
    int16_t existing_type;
    uint8_t should_register;
    uint32_t *clear;
    int32_t i;

    self = halo::ai::actor_at(actor_index);
    threshold = radius + 10.0f;

    if (threshold < distance || threshold == distance) {
        return 0;
    }

    existing_type = self->danger_type;
    should_register = 0;
    if (existing_type <= 0) {
        should_register = 1;
    } else if (existing_type == 1 && self->danger_object_index != source_object_index &&
               distance < self->danger_distance) {
        should_register = 1;
    }

    if (should_register == 0) {
        return 0;
    }

    source_obj = halo::ai::object_at(source_object_index);

    memset(&self->danger_type, 0, 0x1b * sizeof(uint32_t));

    self->danger_object_radius = radius;
    self->danger_type = 1;
    self->danger_object_index = source_object_index;

    halo::objects::object_get_position(&self->danger_center, source_object_index);

    self->danger_object_velocity = *(real_vector3d *)&source_obj->velocity.i;
    self->danger_reaction_ticks = 6;
    self->danger_reaction_delayed = unknown_byte;
    self->danger_owner_relation = (int16_t)(accept_flag == 0);
    return 1;
}

namespace halo::ai {
uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index, float radius, float distance, char accept_flag, uint8_t unknown_byte)
{
    return halo::ai::prop_ops(actor_index).danger_register_point(source_object_index, radius, distance, accept_flag, unknown_byte);
}
}

namespace c_actor_danger_register_stationary_object {
static float sqrt_f(float x) { return (float)halo::libm::sqrt((double)x); }


}


/**
 * actor_danger_register_stationary_object: behaviour unchanged from the original routine.
 *
 * @address 0x41ea60
 */
uint8_t halo::ai::prop_ops::danger_register_stationary_object(const actor_firing_positions *reference, datum_index actor_index, datum_index object_index, uint8_t unknown_byte)
{
    using namespace c_actor_danger_register_stationary_object;
    actor *self;
    object *obj;
    Vehicle *tag_data;
    float bounding_radius;
    float velocity_sq;
    real_point3d fetched_position;
    actor_firing_positions local_positions;
    const actor_firing_positions *block;
    float px, py, pz;
    float dx, dy, dz;
    float distance;
    float threshold_distance;
    int16_t existing_type;
    uint32_t *clear;
    int32_t i;
    int32_t driver_field;

    self = halo::ai::actor_at(actor_index);
    if (self->active_unit_index != k_datum_index_none) {
        return 0;
    }

    obj = halo::ai::object_at(object_index);
    tag_data = halo::ai::tag_data<Vehicle>(obj->definition_tag);

    if ((int8_t)(uint8_t)tag_data->vehicle_flags < 0) {
        velocity_sq = obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                      obj->velocity.i * obj->velocity.i;

        if (velocity_sq > 0.0011111111f) {
            halo::objects::object_get_position(&fetched_position, object_index);
            px = fetched_position.x;
            py = fetched_position.y;
            pz = fetched_position.z;

            block = reference;
            if (block == (const actor_firing_positions *)0) {
                halo::ai::actor_get_firing_positions(actor_index, &local_positions, &fetched_position);
                block = &local_positions;
            }

            dx = px - block->body_position.x;
            dy = py - block->body_position.y;
            dz = pz - block->body_position.z;
            distance = sqrt_f(dx * dx + dy * dy + dz * dz);

            bounding_radius = tag_data->base.base.bounding_radius;
            threshold_distance = bounding_radius + 10.0f;

            if (threshold_distance <= distance) {
                return 0;
            }

            existing_type = self->danger_type;
            if (existing_type < 3 ||
                (existing_type == 3 && self->danger_object_index != object_index &&
                 distance < self->danger_distance)) {
                memset(&self->danger_type, 0, 0x1b * sizeof(uint32_t));

                self->danger_type = 3;
                self->danger_object_index = object_index;
                driver_field = (int32_t)halo::units::unit_data_of(obj)->driver_unit_index;
                self->danger_owner_unit = driver_field;
                self->danger_object_radius = bounding_radius;

                self->danger_object_position.x = px;
                self->danger_object_position.y = py;
                self->danger_object_position.z = pz;
                self->danger_object_velocity = *(real_vector3d *)&obj->velocity.i;
                self->danger_reaction_delayed = unknown_byte;

                self->danger_reaction_ticks = 0x14;
                self->danger_owner_relation = 0;

                if (driver_field != -1) {

                    if (halo::game::teams_are_enemies(halo::ai::object_at(driver_field)->owner_team,
                                          self->team) == 0) {
                        self->danger_owner_relation = 1;
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_danger_register_stationary_object(const actor_firing_positions *reference, datum_index actor_index, datum_index object_index, uint8_t unknown_byte)
{
    return halo::ai::prop_ops::danger_register_stationary_object(reference, actor_index, object_index, unknown_byte);
}
}

namespace c_actor_find_danger_escape {
static auto &global_forward2d_pointer = halo::link::ref<const real_vector2d *>(halo::ai::vars().global_forward2d_pointer);


}


/**
 * actor_find_danger_escape: behaviour unchanged from the original routine.
 *
 * @address 0x40bc40
 */
uint8_t halo::ai::prop_ops::find_danger_escape(int16_t *out_kind, float *out_step, real_vector3d *path_delta, uint8_t *in_danger)
{
    using namespace c_actor_find_danger_escape;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    Unit *unit_tag = halo::ai::tag_data<Unit>(*(datum_index *)halo::ai::object_bytes(act->unit_index));
    float step = unit_tag->distance_of_dive_anim;
    int16_t kind = -1;
    uint8_t blocked = 0;
    uint8_t escapes = 0;
    real_vector2d axis = {0.0f, 0.0f};

    if (step > 0.0f) {
        Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
        float sideways = halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::dive_off_ledges) ? 8.0f : 0.0f;
        float length;
        real_vector3d path;
        real_vector3d left;
        real_vector3d right;
        real_point3d left_point;
        real_point3d right_point;
        uint8_t left_blocked = 0;
        uint8_t right_blocked = 0;
        uint8_t left_hit;
        uint8_t right_hit;
        uint8_t left_out;
        uint8_t right_out;
        float left_distance;
        float right_distance;
        path_find_boundary_crossing extra;
        uint8_t have_axis = 0;

        axis.i = -act->danger_velocity.i;
        axis.j = -act->danger_velocity.j;
        length = (float)halo::libm::sqrt(axis.j * axis.j + axis.i * axis.i);
        if (halo::libm::fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            axis.i *= inverse;
            axis.j *= inverse;
            if (length > 0.033333335f) {
                have_axis = 1;
            }
        }
        if (!have_axis) {
            axis.i = act->flee_from_point.x - act->body_position.x;
            axis.j = act->flee_from_point.y - act->body_position.y;
            if (halo::math::vector2d_normalize_with_length(axis) == 0.0f) {
                axis.i = act->facing.i;
                axis.j = act->facing.j;
                if (halo::math::vector2d_normalize_with_length(axis) == 0.0f) {
                    axis = *global_forward2d_pointer;
                }
            }
        }
        path.i = act->danger_segment_end.x - act->flee_from_point.x;
        path.j = act->danger_segment_end.y - act->flee_from_point.y;
        path.k = act->danger_segment_end.z - act->flee_from_point.z;
        left.i = -axis.j;
        left.j = axis.i;
        left.k = 0.0f;
        right.i = axis.j;
        right.j = -axis.i;
        right.k = 0.0f;
        left_point.x = left.i * step + act->body_position.x;
        left_point.y = axis.i * step + act->body_position.y;
        left_point.z = step * 0.0f + act->body_position.z;
        right_point.x = axis.j * step + act->body_position.x;
        right_point.y = right.j * step + act->body_position.y;
        right_point.z = step * 0.0f + act->body_position.z;

        left_hit = halo::ai::actor_check_step_obstruction(actor_index, (real_vector2d *)&left, step, sideways, &left_blocked, &extra);
        left_distance = (float)halo::libm::sqrt(halo::math::point3d_distance_squared_to_segment(act->flee_from_point, path, left_point));
        left_out = (uint8_t)(left_hit && left_distance > act->danger_object_radius);
        right_hit = halo::ai::actor_check_step_obstruction(actor_index, (real_vector2d *)&right, step, sideways, &right_blocked, &extra);
        right_distance = (float)halo::libm::sqrt(halo::math::point3d_distance_squared_to_segment(act->flee_from_point, path, right_point));
        right_out = (uint8_t)(right_hit && right_distance > act->danger_object_radius);

        if (left_hit) {
            if (right_hit) {
                float difference = left_distance - right_distance;

                if (left_blocked > right_blocked || left_out > right_out || difference > 0.3f) {
                    kind = 0;
                    escapes = 1;
                    blocked = left_blocked;
                } else if (right_blocked > left_blocked || right_out > left_out || difference < -0.3f) {
                    kind = 1;
                    escapes = 1;
                    blocked = right_blocked;
                } else {
                    kind = 4;
                    escapes = left_out;
                    blocked = left_blocked;
                }
            } else {
                kind = 0;
                blocked = left_blocked;
                escapes = left_out;
            }
        } else if (right_hit) {
            kind = 1;
            escapes = right_out;
            blocked = right_blocked;
        }
    }
    *out_kind = kind;
    *out_step = step;
    *in_danger = blocked;
    path_delta->i = axis.i;
    path_delta->j = axis.j;
    return escapes;
}

namespace halo::ai {
uint8_t actor_find_danger_escape(datum_index actor_index, int16_t *out_kind, float *out_step, real_vector3d *path_delta, uint8_t *in_danger)
{
    return halo::ai::prop_ops(actor_index).find_danger_escape(out_kind, out_step, path_delta, in_danger);
}
}


namespace c_actor_find_or_allocate_prop {
enum {
    k_prop_admit_drop,
    k_prop_admit_keep,
};

static int actor_prop_still_admitted(datum_index actor_index, actor *self, prop *p, float distance_squared,
    uint8_t *far_out)
{
    datum_index owner_index = p->owner_actor_index;
    float radius = p->danger_radius;
    int16_t pinned_ticks = p->retain_timer;
    int16_t since_fired = p->dead_ticks;
    actor *owner = 0;

    *far_out = 0;
    if (p->is_parented != 0) {
        return k_prop_admit_keep;
    }
    if (owner_index != k_datum_index_none) {
        owner = halo::ai::actor_at(owner_index);
    }
    if (owner != 0 && (owner->active == 0 || owner->keep_unit_alive != 0)) {
        return k_prop_admit_drop;
    }
    if (p->in_use != 0 || pinned_ticks > 0) {
        return k_prop_admit_keep;
    }
    if (distance_squared > 1600.0f) {
        return k_prop_admit_drop;
    }
    if (p->dead != 0) {
        datum_index encounter_index = self->encounter_index;

        if (encounter_index != k_datum_index_none) {
            struct encounter *encounter = halo::ai::encounter_at(encounter_index);
            unit_object *unit = (unit_object *)halo::ai::object_at(p->object_index);
            int32_t reference = encounter->last_idle_time;
            uint8_t counts = 1;
            uint8_t calm;

            if (!(reference > static_cast<int32_t>(self->found_body_time))) {
                reference = static_cast<int32_t>(self->found_body_time);
            }
            if (reference != -1) {
                int32_t fired = ((struct unit_object *)unit)->unit.death_time;

                if (fired == -1 || fired < reference) {
                    counts = 0;
                }
            }
            calm = encounter->engaged == 0 && encounter->has_live_target == 0 && encounter->stood_down == 0;
            if (!counts) {
                return k_prop_admit_drop;
            }
            if (calm) {
                return distance_squared < 225.0f ? k_prop_admit_keep : k_prop_admit_drop;
            }
        }

        if (radius > 0.0f) {
            return k_prop_admit_keep;
        }
        {
            uint8_t enemy = p->enemy;
            float limit;

            if (enemy && since_fired > 0x96) {
                return k_prop_admit_drop;
            }
            if (halo::ai::actor_get_current_mode_combat_grade(actor_index) > 1) {
                return k_prop_admit_drop;
            }
            limit = 16.0f;
            if (!enemy && self->awareness_level < 3) {
                limit = 64.0f;
            }
            return distance_squared < limit ? k_prop_admit_keep : k_prop_admit_drop;
        }
    }

    if (p->enemy != 0) {
        *far_out = distance_squared > 36.0f;
        return k_prop_admit_keep;
    }
    if (self->combat_status >= 4) {
        *far_out = 1;
    } else if (self->grenade_ally_phase_flag == 0) {
        *far_out = distance_squared > 16.0f;
    } else {
        *far_out = 0;
    }
    return distance_squared < 225.0f ? k_prop_admit_keep : k_prop_admit_drop;
}
}


/**
 * actor_find_or_allocate_prop: behaviour unchanged from the original routine.
 *
 * @address 0x43e270
 */
datum_index halo::ai::prop_ops::find_or_allocate_prop(uint32_t object_index, char kind)
{
    using namespace c_actor_find_or_allocate_prop;
    datum_index actor_index = datum;
    actor *self = halo::ai::actor_at(actor_index);
    datum_index cursor = self->first_prop;
    datum_index drop_choice = k_datum_index_none;
    datum_index far_choice = k_datum_index_none;
    float drop_distance = 3.4028234663852886e+38f;
    float far_distance = 3.4028234663852886e+38f;
    int16_t same_kind_count = 0;
    datum_index result;

    while (cursor != k_datum_index_none) {
        datum_index current = cursor;
        prop *p = halo::ai::prop_at(cursor);
        int16_t prop_kind = p->state;
        float distance = p->distance;
        uint8_t far_flag;

        cursor = p->next_in_actor;
        if ((prop_kind >= 4 && prop_kind <= 5) || p->pair_index != k_datum_index_none) {
            continue;
        }
        if (actor_prop_still_admitted(actor_index, self, p, distance * distance, &far_flag) == k_prop_admit_keep) {
            if ((char)p->enemy != kind) {
                continue;
            }
            same_kind_count++;
            if (far_flag && far_distance > distance) {
                far_choice = current;
                far_distance = distance;
            }
        } else if (distance < drop_distance) {
            drop_choice = current;
            drop_distance = distance;
        }
    }

    result = drop_choice;
    if (result == k_datum_index_none) {
        result = far_choice;
        if (result != k_datum_index_none && same_kind_count < (kind != 0 ? 6 : 4)) {
            result = k_datum_index_none;
        }
    }
    if (result == k_datum_index_none) {
        result = halo::memory::datum_new(halo::ai::globals().prop_data);
    } else {
        prop *p = halo::ai::prop_at(result);
        int16_t salt = p->identifier;

        halo::ai::actor_replace_object_reference(actor_index, halo::k_dword_none, result);
        halo::ai::actor_unlink_prop(actor_index, result);
        memset(p, 0, 0x138);
        p->identifier = salt;
    }
    halo::ai::actor_init_prop_from_object(object_index, actor_index, result);
    return result;
}

namespace halo::ai {
datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind)
{
    return halo::ai::prop_ops(actor_index).find_or_allocate_prop(object_index, kind);
}
}

namespace c_actor_find_or_create_shared_prop {
}


/**
 * actor_find_or_create_shared_prop: behaviour unchanged from the original routine.
 *
 * @address 0x43eb30
 */
datum_index halo::ai::prop_ops::find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag)
{
    using namespace c_actor_find_or_create_shared_prop;
    datum_index result = (datum_index)halo::k_dword_none;
    actor *self;
    unit_object *object;
    int32_t cluster_ref;

    if (object_index == (datum_index)halo::k_dword_none) {
        return result;
    }

    self = halo::ai::actor_at(actor_index);
    object = (unit_object *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index)));
    cluster_ref = static_cast<int32_t>(object->unit.swarm_actor_index);
    if (cluster_ref == -1) {
        cluster_ref = static_cast<int32_t>(object->unit.actor_index);
    }

    if ((((struct object *)object)->type == 0) && ((datum_index)cluster_ref != actor_index)) {
        datum_index cur = self->first_prop;

        for (;;) {
            prop *p;
            if (cur == (datum_index)halo::k_dword_none) {
                break;
            }
            p = halo::ai::prop_at(cur);
            if ((p->object_index == object_index) ||
                ((p->swarm_owned != 0) && (p->owner_actor_index != (datum_index)halo::k_dword_none) &&
                 ((int32_t)p->owner_actor_index == cluster_ref))) {
                break;
            }
            cur = p->next_in_actor;
        }

        if (cur != (datum_index)halo::k_dword_none) {
            prop *p = halo::ai::prop_at(cur);
            result = cur;
            if (p->pair_index != (datum_index)halo::k_dword_none) {
                result = p->pair_index;
            }
        }

        if (result == (datum_index)halo::k_dword_none) {
            if ((create_if_missing != 0) && (self->active != 0)) {
                actor_firing_positions scratch;

                result = halo::ai::actor_find_or_allocate_prop(actor_index, object_index,
                    (char)halo::game::teams_are_enemies(((struct object *)object)->owner_team, self->team));
                if (result != (datum_index)halo::k_dword_none) {
                    prop *p = halo::ai::prop_at(result);

                    halo::ai::actor_target_data_refresh(actor_index, result, &scratch, 0, flag);
                    p->retain_timer = 0x1e;
                    p->just_created = 1;

                    if ((uint8_t)flag != 0 && (halo::ai::actor_target_update_tracking_speed(actor_index, result, &scratch), 1 < p->perception_level)) {
                        uint8_t seen_flag = halo::ai::actor_target_has_conflicting_neighbor(actor_index, result);
                        p->state = 3;
                        halo::ai::actor_target_reset_combat_flags(result, actor_index, 0, seen_flag);
                    }
                }
            }
        }
    }

    return result;
}

namespace halo::ai {
datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag)
{
    return halo::ai::prop_ops::find_or_create_shared_prop(object_index, actor_index, create_if_missing, flag);
}
}

namespace c_actor_find_prop_for_object {
}


/**
 * actor_find_prop_for_object: behaviour unchanged from the original routine.
 *
 * @address 0x43ea80
 */
datum_index halo::ai::prop_ops::find_prop_for_object(datum_index object_index, datum_index actor_index)
{
    using namespace c_actor_find_prop_for_object;
    unit_object *object = (unit_object *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index)));
    int32_t cluster_ref = static_cast<int32_t>(object->unit.swarm_actor_index);
    actor *self;
    datum_index cur;

    if (cluster_ref == -1) {
        cluster_ref = static_cast<int32_t>(object->unit.actor_index);
    }

    self = halo::ai::actor_at(actor_index);
    cur = self->first_prop;

    for (;;) {
        prop *p;
        if (cur == (datum_index)halo::k_dword_none) {
            return (datum_index)halo::k_dword_none;
        }
        p = halo::ai::prop_at(cur);

        if (!(((-1 < p->state) && (p->state < 2)) ||
              ((p->object_index != object_index) &&
               ((p->swarm_owned == 0) || (p->owner_actor_index == (datum_index)halo::k_dword_none) ||
                ((int32_t)p->owner_actor_index != cluster_ref))))) {
            return cur;
        }
        cur = p->next_in_actor;
    }
}

namespace halo::ai {
datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index)
{
    return halo::ai::prop_ops::find_prop_for_object(object_index, actor_index);
}
}

namespace c_actor_get_target_prop_object_index {
}


/**
 * actor_get_target_prop_object_index: behaviour unchanged from the original routine.
 *
 * @address 0x4283d0
 */
datum_index halo::ai::prop_ops::get_target_prop_object_index()
{
    using namespace c_actor_get_target_prop_object_index;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)halo::ai::globals().prop_data->data)[self->target_unit_index & halo::k_slot_mask];
        return p->object_index;
    }
    return (datum_index)k_datum_index_none;
}

namespace halo::ai {
datum_index actor_get_target_prop_object_index(datum_index actor_index)
{
    return halo::ai::prop_ops(actor_index).get_target_prop_object_index();
}
}

namespace c_actor_init_prop_from_object {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_init_prop_from_object: behaviour unchanged from the original routine.
 *
 * @address 0x43e640
 */
void halo::ai::prop_ops::init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index)
{
    using namespace c_actor_init_prop_from_object;
    actor *self;
    prop *p;

    if (prop_index == (datum_index)halo::k_dword_none) {
        return;
    }

    self = halo::ai::actor_at(actor_index);
    p = halo::ai::prop_at(prop_index);

    p->actor_index = actor_index;
    p->stimulus_type = -1;
    p->seen_state = -1;
    p->object_index = object_index;
    p->seen = 0;
    p->damage_inflicted_on_me = 0.0f;
    p->information_age = -1;
    p->has_current_information = 0;
    p->information_source_actor = -1;
    p->last_perceived_time = -1;
    p->last_seen_time = -1;
    p->dead_confirmed = 0;
    p->owner_actor_index = (datum_index)halo::k_dword_none;
    p->pair_index = (datum_index)halo::k_dword_none;
    p->retain_timer = 0;
    p->last_engaged_time = -1;

    if (object_index != (datum_index)halo::k_dword_none) {
        unit_object *object = (unit_object *)halo::ai::object_at(object_index);
        Unit *object_type = halo::ai::tag_data<Unit>(object->base.definition_tag);
        uint8_t is_vault;

        p->team = object->base.owner_team;

        p->enemy = halo::game::teams_are_enemies(p->team, self->team);
        p->allegiance = halo::game::team_pair_flag_test(self->team, p->team);
        p->team_pair_status = halo::game::team_pair_override_get_flag(self->team, p->team);

        is_vault = (object->base.vitality_flags >> 2) & 1;
        p->dead = is_vault;
        p->danger_radius = object_type->ai_danger_radius;
        p->dead_not_feigning = (is_vault != 0) && (object->unit.feign_death_ticks == 0);
        p->dead_ticks = (is_vault != 0) ? 1000 : 0;
        p->is_parented = static_cast<int32_t>(object->base.owner_linkage) != -1;

        if ((int32_t)object->unit.swarm_actor_index == -1) {
            p->owner_actor_index = object->unit.actor_index;
        } else {
            p->swarm_owned = 1;
            p->owner_actor_index = object->unit.swarm_actor_index;
            p->swarm_reassign_time = halo::game::globals().game_time->game_time;
        }

        if (p->is_parented != 0) {
            p->actor_type = 6;
            p->next_in_actor = self->first_prop;
            self->first_prop = prop_index;
            return;
        }
        if (p->owner_actor_index != (datum_index)halo::k_dword_none) {
            p->actor_type = (halo::ai::actor_at(p->owner_actor_index))->type;
            p->next_in_actor = self->first_prop;
            self->first_prop = prop_index;
            return;
        }
        p->actor_type = -1;
    }

    p->next_in_actor = self->first_prop;
    self->first_prop = prop_index;
}

namespace halo::ai {
void actor_init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index)
{
    halo::ai::prop_ops::init_prop_from_object(object_index, actor_index, prop_index);
}
}

namespace c_actor_mark_prop_seen_with_delta {
}


/**
 * actor_mark_prop_seen_with_delta: behaviour unchanged from the original routine.
 *
 * @address 0x428840
 */
void halo::ai::prop_ops::mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta, const real_vector3d *direction)
{
    using namespace c_actor_mark_prop_seen_with_delta;
    datum_index prop_index;

    if (object_index == k_datum_index_none) {
        return;
    }
    prop_index = halo::ai::actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
    if (prop_index != k_datum_index_none) {
        prop *p = halo::ai::prop_at(prop_index);
        datum_index pair = p->pair_index;
        int16_t kind;

        p->seen_state = 0;
        p->seen = 1;
        p->damage_inflicted_on_me = delta + p->damage_inflicted_on_me;
        if (pair != k_datum_index_none) {
            prop *q = halo::ai::prop_at(pair);

            q->seen_state = 0;
            q->seen = 1;
            q->damage_inflicted_on_me = delta + q->damage_inflicted_on_me;
        }
        kind = p->state;
        if (kind < 2 || kind > 3) {
            prop_index = k_datum_index_none;
        }
    }
    halo::ai::actor_queue_directional_reaction_event(direction, prop_index, actor_index);
}

namespace halo::ai {
void actor_mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta, const real_vector3d *direction)
{
    halo::ai::prop_ops::mark_prop_seen_with_delta(object_index, actor_index, delta, direction);
}
}


