#include "halo/ai/actor_props.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

namespace c_actor_allocate_paired_prop {
extern "C" {
extern data_array *prop_data;
extern void actor_init_prop_from_object(datum_index object_index, datum_index actor_index,
                                        datum_index prop_index);
extern void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop);
}
}

extern "C" datum_index actor_allocate_paired_prop(datum_index actor_index, datum_index existing_prop);

/**
 * actor_allocate_paired_prop: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_allocate_paired_prop.c.txt.
 *
 * @address 0x43e910
 */
datum_index halo::ai::prop_ops::allocate_paired_prop(datum_index existing_prop)
{
    using namespace c_actor_allocate_paired_prop;
    datum_index actor_index = datum;
    datum_index new_prop = halo::memory::datum_new(prop_data);

    actor_init_prop_from_object(k_datum_index_none, actor_index, new_prop);
    if (new_prop != k_datum_index_none) {
        prop *existing = (prop *)((uint8_t *)prop_data->data + (existing_prop & 0xffff) * sizeof(prop));
        prop *created = (prop *)((uint8_t *)prop_data->data + (new_prop & 0xffff) * sizeof(prop));

        actor_copy_prop_and_reset(new_prop, existing_prop);
        existing->pair_index = new_prop;
        created->pair_index = existing_prop;
    }
    return new_prop;
}

extern "C" datum_index actor_allocate_paired_prop(datum_index actor_index, datum_index existing_prop)
{
    return halo::ai::prop_ops(actor_index).allocate_paired_prop(existing_prop);
}

namespace c_actor_allocate_paired_prop_with_kind {
extern "C" {
extern data_array *prop_data;
extern void actor_init_prop_from_object(datum_index object_index, datum_index actor_index,
                                        datum_index prop_index);
extern void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop);
}
}

extern "C" datum_index actor_allocate_paired_prop_with_kind(datum_index actor_index, datum_index existing_prop, datum_index reference_prop);

/**
 * actor_allocate_paired_prop_with_kind: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_allocate_paired_prop_with_kind.c.txt.
 *
 * @address 0x43e980
 */
datum_index halo::ai::prop_ops::allocate_paired_prop_with_kind(datum_index existing_prop, datum_index reference_prop)
{
    using namespace c_actor_allocate_paired_prop_with_kind;
    datum_index actor_index = datum;
    datum_index new_prop = halo::memory::datum_new(prop_data);

    actor_init_prop_from_object(k_datum_index_none, actor_index, new_prop);
    if (new_prop == k_datum_index_none) {
        return k_datum_index_none;
    }
    {
        prop *existing = (prop *)((uint8_t *)prop_data->data + (existing_prop & 0xffff) * sizeof(prop));
        prop *created = (prop *)((uint8_t *)prop_data->data + (new_prop & 0xffff) * sizeof(prop));
        prop *reference = (prop *)((uint8_t *)prop_data->data + (reference_prop & 0xffff) * sizeof(prop));
        int16_t kind;

        actor_copy_prop_and_reset(new_prop, reference_prop);
        existing->pair_index = new_prop;
        created->pair_index = existing_prop;
        kind = reference->state;
        if (kind >= 4 && kind <= 5) {
            created->state = kind;
        }
    }
    return new_prop;
}

extern "C" datum_index actor_allocate_paired_prop_with_kind(datum_index actor_index, datum_index existing_prop, datum_index reference_prop)
{
    return halo::ai::prop_ops(actor_index).allocate_paired_prop_with_kind(existing_prop, reference_prop);
}

namespace c_actor_apply_unit_definition_properties {
extern "C" {
extern data_array *object_data;
extern int16_t network_game_mode;
extern object_type_definition *object_type_definitions[12];

extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);

extern void object_initialize_shield_stun_thresholds(uint32_t object_index, float *override_max_body_vitality,
    float *override_max_shield_vitality);
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role);
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role);
extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index);

extern void object_delete_unparented(uint32_t object_index);
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
extern uint8_t unit_try_select_equipment(uint32_t unit_index, uint32_t new_equipment_object_index,
    int16_t release_current);
extern void object_delete(uint32_t object_index);

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

static datum_index actor_create_unit_item(datum_index definition_tag, datum_index unit_index)
{
    object_placement_data placement;
    uint32_t role = 3;

    object_placement_data_initialize(&placement, definition_tag, unit_index);
    if (network_game_mode == 2) {
        int16_t type = *(int16_t *)halo::cache::globals().tag_instances[placement.definition_tag & 0xffff].data;

        if (object_type_definitions[type]->network_delta_message_type != -1) {
            role = 0;
        }
    }
    return object_new_with_datum_role_control(&placement, role);
}
}
}

extern "C" void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index);

/**
 * actor_apply_unit_definition_properties: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_apply_unit_definition_properties.c.txt.
 *
 * @address 0x426cf0
 */
void halo::ai::prop_ops::apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index)
{
    using namespace c_actor_apply_unit_definition_properties;
    uint8_t *variant = (uint8_t *)halo::cache::globals().tag_instances[actor_variant_tag & 0xffff].data;
    uint8_t *unit = object_get(unit_index);
    uint8_t *unit_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((ActorVariant *)variant)->actor_definition.tag_id & 0xffff].data;
    int16_t i;

    if (((ActorVariant *)variant)->body_vitality > 0.0f || ((ActorVariant *)variant)->shield_vitality > 0.0f) {
        object_initialize_shield_stun_thresholds(unit_index, (float *)(variant + 0x200), (float *)(variant + 0x204));
    }
    if (*(int16_t *)&((ActorVariant *)variant)->forced_shader_permutation != 0) {
        *(int16_t *)&((unit_object *)unit)->base.forced_shader_permutation = *(int16_t *)&((ActorVariant *)variant)->forced_shader_permutation;
    }
    for (i = 0; i < *(int32_t *)&((ActorVariant *)variant)->change_colors.count; i++) {
        uint8_t *change_color = *(uint8_t **)&((ActorVariant *)variant)->change_colors.pointer + i * 0x20;

        if (i < 4) {
            ColorRGB *working = (ColorRGB *)(unit + 0x188 + i * 0xc);

            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
            color_interpolate((ColorRGB *)(change_color + 0xc), (ColorRGB *)change_color, working, 1,
                (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f);
            *(ColorRGB *)(unit + 0x1b8 + i * 0xc) = *working;
        }
    }
    if (*(datum_index *)&((ActorVariant *)variant)->weapon.tag_id != k_datum_index_none) {
        datum_index weapon = actor_create_unit_item(*(datum_index *)&((ActorVariant *)variant)->weapon.tag_id, unit_index);

        if (weapon != k_datum_index_none && !unit_pickup_weapon(2, weapon, unit_index)) {
            int32_t role = *(int32_t *)(object_get(weapon) + 4);

            if (role == 0) {
                object_delete_unparented(weapon);
                object_delete_recursive(weapon, 0);
            } else if (role == 3) {
                object_delete_recursive(weapon, 0);
            }
        }
    }
    if (*(int16_t *)&((ActorVariant *)variant)->grenade_type != -1) {
        int16_t type = *(int16_t *)&((ActorVariant *)variant)->grenade_type;
        int16_t minimum = *(int16_t *)(variant + 0x1d0);
        int32_t range = (int16_t)(*(int16_t *)(variant + 0x1d2) + 1) - minimum;
        uint8_t *object = object_get(unit_index);

        halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
        object[0x31e + type] = (uint8_t)(object[0x31e + type] +
            (uint8_t)(((uint32_t)range * (halo::math::globals().random_seed_global >> 0x10)) >> 0x10) + (uint8_t)minimum);
        object[0x31d] = (uint8_t)type;
        object[0x31c] = (uint8_t)type;
    }
    if (*(datum_index *)&((ActorVariant *)variant)->equipment.tag_id != k_datum_index_none) {
        int16_t equipment_kind = *(int16_t *)((uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((ActorVariant *)variant)->equipment.tag_id & 0xffff].data
            + 0x308);

        if (equipment_kind != 0 && equipment_kind != 6) {
            datum_index equipment = actor_create_unit_item(*(datum_index *)&((ActorVariant *)variant)->equipment.tag_id, unit_index);

            if (equipment != k_datum_index_none && !unit_try_select_equipment(unit_index, equipment, 1)) {
                object_delete(equipment);
            }
        }
    }
    if (*(uint32_t *)variant & 0x30) {
        if (*(uint32_t *)variant & 0x20) {
            ((unit_object *)unit)->unit.flags |= 0x20;
        }
        ((unit_object *)unit)->unit.flags |= 0x10;
        ((struct unit_object *)unit)->unit.active_camouflage_power = 1.0f;
        ((struct unit_object *)unit)->unit.super_active_camouflage_power = (unit_tag[0] & 0x20) ? 1.0f : 0.0f;
    }
}

extern "C" void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index)
{
    halo::ai::prop_ops::apply_unit_definition_properties(actor_variant_tag, unit_index);
}

namespace c_actor_clear_perceived_props {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference);
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove);
}
}

extern "C" void actor_clear_perceived_props(datum_index actor_index);

/**
 * actor_clear_perceived_props: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_clear_perceived_props.c.txt.
 *
 * @address 0x427e00
 */
void halo::ai::prop_ops::clear_perceived_props()
{
    using namespace c_actor_clear_perceived_props;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    while (self->first_prop != (datum_index)k_datum_index_none) {
        datum_index prop_index = self->first_prop;
        prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];

        (void)p;
        actor_replace_object_reference(actor_index, 0xffffffff, prop_index);
        actor_unlink_prop(actor_index, prop_index);
        halo::memory::datum_delete(prop_data, prop_index);
    }
}

extern "C" void actor_clear_perceived_props(datum_index actor_index)
{
    halo::ai::prop_ops(actor_index).clear_perceived_props();
}

namespace c_actor_clear_recognition_history {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed);

/**
 * actor_clear_recognition_history: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_clear_recognition_history.c.txt.
 *
 * @address 0x414140
 */
void halo::ai::prop_ops::clear_recognition_history(uint8_t keep_when_typed)
{
    using namespace c_actor_clear_recognition_history;
    datum_index actor_index = datum;
    actor *self;
    int i;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    self->recognition_cursor = 0;
    for (i = 0; i < 4; i++) {
        self->recognition[i].firing_position_index = -1;
    }

    if (self->recognition_valid != 0 && (keep_when_typed == 0 || self->recognition_type != 0)) {
        self->recognition_valid = 0;
    }
}

extern "C" void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed)
{
    halo::ai::prop_ops(actor_index).clear_recognition_history(keep_when_typed);
}

namespace c_actor_copy_prop_and_reset {
extern "C" {
extern data_array *prop_data;
extern real_point3d *global_origin3d_pointer;
}
}

extern "C" void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop);

/**
 * actor_copy_prop_and_reset: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_copy_prop_and_reset.c.txt.
 *
 * @address 0x43e840
 */
void halo::ai::prop_ops::copy_prop_and_reset(datum_index dest_prop, datum_index src_prop)
{
    using namespace c_actor_copy_prop_and_reset;
    prop *dest = (prop *)((uint8_t *)prop_data->data + (dest_prop & 0xffff) * sizeof(prop));
    prop *src = (prop *)((uint8_t *)prop_data->data + (src_prop & 0xffff) * sizeof(prop));

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

extern "C" void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop)
{
    halo::ai::prop_ops::copy_prop_and_reset(dest_prop, src_prop);
}

namespace c_actor_danger_register_point {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;

extern void object_get_position(real_point3d *out_position, datum_index object_index);
}
}

extern "C" uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index, float radius, float distance, char accept_flag, uint8_t unknown_byte);

/**
 * actor_danger_register_point: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_danger_register_point.c.txt.
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
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

    source_obj = ((object_header *)object_data->data)[source_object_index & 0xffff].data;

    clear = (uint32_t *)&self->danger_type;
    for (i = 0x1b; i != 0; i--) {
        *clear = 0;
        clear++;
    }

    self->danger_object_radius = radius;
    self->danger_type = 1;
    self->danger_object_index = source_object_index;

    object_get_position(&self->danger_center, source_object_index);

    self->danger_object_velocity = *(real_vector3d *)&source_obj->velocity.i;
    self->danger_reaction_ticks = 6;
    self->danger_reaction_delayed = unknown_byte;
    self->danger_owner_relation = (int16_t)(accept_flag == 0);
    return 1;
}

extern "C" uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index, float radius, float distance, char accept_flag, uint8_t unknown_byte)
{
    return halo::ai::prop_ops(actor_index).danger_register_point(source_object_index, radius, distance, accept_flag, unknown_byte);
}

namespace c_actor_danger_register_stationary_object {
extern "C" {
extern double sqrt(double x);
static float sqrt_f(float x) { return (float)sqrt((double)x); }

extern data_array *actor_data;
extern data_array *object_data;

extern void object_get_position(real_point3d *out_position, datum_index object_index);
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point);
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
}
}

extern "C" uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index, datum_index object_index, uint8_t unknown_byte);

/**
 * actor_danger_register_stationary_object: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_danger_register_stationary_object.c.txt.
 *
 * @address 0x41ea60
 */
uint8_t halo::ai::prop_ops::danger_register_stationary_object(const float *reference, datum_index actor_index, datum_index object_index, uint8_t unknown_byte)
{
    using namespace c_actor_danger_register_stationary_object;
    actor *self;
    object *obj;
    uint8_t *tag_data;
    float bounding_radius;
    float velocity_sq;
    real_point3d fetched_position;
    uint32_t local_positions[14];
    const float *block;
    float px, py, pz;
    float dx, dy, dz;
    float distance;
    float threshold_distance;
    int16_t existing_type;
    uint32_t *clear;
    int32_t i;
    int32_t driver_field;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->active_unit_index != k_datum_index_none) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    tag_data = (uint8_t *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if ((int8_t)tag_data[0x2f0] < 0) {
        velocity_sq = obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                      obj->velocity.i * obj->velocity.i;

        if (velocity_sq > 0.0011111111f) {
            object_get_position(&fetched_position, object_index);
            px = fetched_position.x;
            py = fetched_position.y;
            pz = fetched_position.z;

            block = reference;
            if (block == (const float *)0) {
                actor_get_firing_positions(actor_index, local_positions, &fetched_position);
                block = (const float *)local_positions;
            }

            dx = px - *(float *)((const uint8_t *)block + 0xc);
            dy = py - *(float *)((const uint8_t *)block + 0x10);
            dz = pz - *(float *)((const uint8_t *)block + 0x14);
            distance = sqrt_f(dx * dx + dy * dy + dz * dz);

            bounding_radius = *(float *)(tag_data + 4);
            threshold_distance = bounding_radius + 10.0f;

            if (threshold_distance <= distance) {
                return 0;
            }

            existing_type = self->danger_type;
            if (existing_type < 3 ||
                (existing_type == 3 && self->danger_object_index != object_index &&
                 distance < self->danger_distance)) {
                clear = (uint32_t *)&self->danger_type;
                for (i = 0x1b; i != 0; i--) {
                    *clear = 0;
                    clear++;
                }

                self->danger_type = 3;
                self->danger_object_index = object_index;
                driver_field = *(int32_t *)((uint8_t *)obj + 0x324);
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

                    if (teams_are_enemies(*(int16_t *)((uint8_t *)((object_header *)object_data->data)[driver_field & 0xffff].data + 0xb8),
                                          ((struct actor *)self)->team) == 0) {
                        self->danger_owner_relation = 1;
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

extern "C" uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index, datum_index object_index, uint8_t unknown_byte)
{
    return halo::ai::prop_ops::danger_register_stationary_object(reference, actor_index, object_index, unknown_byte);
}

namespace c_actor_find_danger_escape {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern const real_vector2d *global_forward2d_pointer;

extern double sqrt(double x);
extern double fabs(double x);
extern uint8_t actor_check_step_obstruction(datum_index actor_index, real_vector2d *direction, float step_distance,
                                            float step_up, uint8_t *out_flag, void *extra_param);

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
}
}

extern "C" uint8_t actor_find_danger_escape(datum_index actor_index, uint32_t *out_word, uint8_t *out_position, real_vector3d *path_delta, uint8_t *in_danger);

/**
 * actor_find_danger_escape: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_danger_escape.c.txt.
 *
 * @address 0x40bc40
 */
uint8_t halo::ai::prop_ops::find_danger_escape(uint32_t *out_word, uint8_t *out_position, real_vector3d *path_delta, uint8_t *in_danger)
{
    using namespace c_actor_find_danger_escape;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(((actor *)act)->unit_index));
    float step = *(float *)(unit_tag + 0x238);
    int16_t kind = -1;
    uint8_t blocked = 0;
    uint8_t escapes = 0;
    real_vector2d axis = {0.0f, 0.0f};

    if (step > 0.0f) {
        uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
        float sideways = (*(uint32_t *)actor_tag & 0x2000000) ? 8.0f : 0.0f;
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
        uint8_t extra[0x30];
        uint8_t have_axis = 0;

        axis.i = -*(float *)(act + 0x2bc);
        axis.j = -*(float *)(act + 0x2c0);
        length = (float)sqrt(axis.j * axis.j + axis.i * axis.i);
        if (fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            axis.i *= inverse;
            axis.j *= inverse;
            if (length > 0.033333335f) {
                have_axis = 1;
            }
        }
        if (!have_axis) {
            axis.i = ((actor *)act)->flee_from_point.x - ((actor *)act)->body_position.x;
            axis.j = ((actor *)act)->flee_from_point.y - ((actor *)act)->body_position.y;
            if (halo::math::vector2d_normalize_with_length(axis) == 0.0f) {
                axis.i = ((actor *)act)->facing.i;
                axis.j = ((actor *)act)->facing.j;
                if (halo::math::vector2d_normalize_with_length(axis) == 0.0f) {
                    axis = *global_forward2d_pointer;
                }
            }
        }
        path.i = ((actor *)act)->danger_segment_end.x - ((actor *)act)->flee_from_point.x;
        path.j = ((actor *)act)->danger_segment_end.y - ((actor *)act)->flee_from_point.y;
        path.k = ((actor *)act)->danger_segment_end.z - ((actor *)act)->flee_from_point.z;
        left.i = -axis.j;
        left.j = axis.i;
        left.k = 0.0f;
        right.i = axis.j;
        right.j = -axis.i;
        right.k = 0.0f;
        left_point.x = left.i * step + ((actor *)act)->body_position.x;
        left_point.y = axis.i * step + ((actor *)act)->body_position.y;
        left_point.z = step * 0.0f + ((actor *)act)->body_position.z;
        right_point.x = axis.j * step + ((actor *)act)->body_position.x;
        right_point.y = right.j * step + ((actor *)act)->body_position.y;
        right_point.z = step * 0.0f + ((actor *)act)->body_position.z;

        left_hit = actor_check_step_obstruction(actor_index, (real_vector2d *)&left, step, sideways, &left_blocked, extra);
        left_distance = (float)sqrt(halo::math::point3d_distance_squared_to_segment(*(real_point3d *)(act + 0x2b0), path, left_point));
        left_out = (uint8_t)(left_hit && left_distance > ((actor *)act)->danger_unknown_294);
        right_hit = actor_check_step_obstruction(actor_index, (real_vector2d *)&right, step, sideways, &right_blocked, extra);
        right_distance = (float)sqrt(halo::math::point3d_distance_squared_to_segment(*(real_point3d *)(act + 0x2b0), path, right_point));
        right_out = (uint8_t)(right_hit && right_distance > ((actor *)act)->danger_unknown_294);

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
    *(int16_t *)out_word = kind;
    *(float *)out_position = step;
    *in_danger = blocked;
    path_delta->i = axis.i;
    path_delta->j = axis.j;
    return escapes;
}

extern "C" uint8_t actor_find_danger_escape(datum_index actor_index, uint32_t *out_word, uint8_t *out_position, real_vector3d *path_delta, uint8_t *in_danger)
{
    return halo::ai::prop_ops(actor_index).find_danger_escape(out_word, out_position, path_delta, in_danger);
}

#undef ACTOR
#undef OBJECT_DATA
#undef TAG_DATA

namespace c_actor_find_or_allocate_prop {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;
extern data_array *encounter_data;

extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference);
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove);
extern void actor_init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index);
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index);

enum {
    k_prop_admit_drop,
    k_prop_admit_keep,
};

static int actor_prop_still_admitted(datum_index actor_index, uint8_t *self, uint8_t *p, float distance_squared,
    uint8_t *far_out)
{
    datum_index owner_index = ((prop *)p)->owner_actor_index;
    float radius = ((struct prop *)p)->danger_radius;
    int16_t pinned_ticks = ((struct prop *)p)->retain_timer;
    int16_t since_fired = ((struct prop *)p)->dead_ticks;
    uint8_t *owner = 0;

    *far_out = 0;
    if (p[0x12e] != 0) {
        return k_prop_admit_keep;
    }
    if (owner_index != k_datum_index_none) {
        owner = (uint8_t *)actor_data->data + (owner_index & 0xffff) * 0x724;
    }
    if (owner != 0 && (owner[8] == 0 || owner[0x13] != 0)) {
        return k_prop_admit_drop;
    }
    if (p[0x63] != 0 || pinned_ticks > 0) {
        return k_prop_admit_keep;
    }
    if (distance_squared > 1600.0f) {
        return k_prop_admit_drop;
    }
    if (p[0x127] != 0) {
        datum_index encounter_index = ((actor *)self)->encounter_index;

        if (encounter_index != k_datum_index_none) {
            uint8_t *encounter = (uint8_t *)encounter_data->data + (encounter_index & 0xffff) * 0x6c;
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[((prop *)p)->object_index & 0xffff].data;
            int32_t reference = ((struct encounter *)encounter)->last_idle_time;
            uint8_t counts = 1;
            uint8_t calm;

            if (!(reference > *(int32_t *)&((struct actor *)self)->found_body_time)) {
                reference = *(int32_t *)&((struct actor *)self)->found_body_time;
            }
            if (reference != -1) {
                int32_t fired = ((struct unit_object *)unit)->unit.death_time;

                if (fired == -1 || fired < reference) {
                    counts = 0;
                }
            }
            calm = encounter[0x45] == 0 && encounter[0x44] == 0 && encounter[0x42] == 0;
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
            uint8_t enemy = p[0x60];
            float limit;

            if (enemy && since_fired > 0x96) {
                return k_prop_admit_drop;
            }
            if (actor_get_current_mode_combat_grade(actor_index) > 1) {
                return k_prop_admit_drop;
            }
            limit = 16.0f;
            if (!enemy && ((actor *)self)->awareness_level < 3) {
                limit = 64.0f;
            }
            return distance_squared < limit ? k_prop_admit_keep : k_prop_admit_drop;
        }
    }

    if (p[0x60] != 0) {
        *far_out = distance_squared > 36.0f;
        return k_prop_admit_keep;
    }
    if (((struct actor *)self)->combat_status >= 4) {
        *far_out = 1;
    } else if (self[0x1cc] == 0) {
        *far_out = distance_squared > 16.0f;
    } else {
        *far_out = 0;
    }
    return distance_squared < 225.0f ? k_prop_admit_keep : k_prop_admit_drop;
}
}
}

extern "C" datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind);

/**
 * actor_find_or_allocate_prop: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_or_allocate_prop.c.txt.
 *
 * @address 0x43e270
 */
datum_index halo::ai::prop_ops::find_or_allocate_prop(uint32_t object_index, char kind)
{
    using namespace c_actor_find_or_allocate_prop;
    datum_index actor_index = datum;
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    datum_index cursor = ((actor *)self)->first_prop;
    datum_index drop_choice = k_datum_index_none;
    datum_index far_choice = k_datum_index_none;
    float drop_distance = 3.4028234663852886e+38f;
    float far_distance = 3.4028234663852886e+38f;
    int16_t same_kind_count = 0;
    datum_index result;

    while (cursor != k_datum_index_none) {
        datum_index current = cursor;
        uint8_t *p = (uint8_t *)prop_data->data + (cursor & 0xffff) * 0x138;
        int16_t prop_kind = ((prop *)p)->state;
        float distance = ((prop *)p)->distance;
        uint8_t far_flag;

        cursor = ((prop *)p)->next_in_actor;
        if ((prop_kind >= 4 && prop_kind <= 5) || ((prop *)p)->pair_index != k_datum_index_none) {
            continue;
        }
        if (actor_prop_still_admitted(actor_index, self, p, distance * distance, &far_flag) == k_prop_admit_keep) {
            if ((char)p[0x60] != kind) {
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
        result = halo::memory::datum_new(prop_data);
    } else {
        uint8_t *p = (uint8_t *)prop_data->data + (result & 0xffff) * 0x138;
        int16_t salt = *(int16_t *)p;

        actor_replace_object_reference(actor_index, 0xffffffff, result);
        actor_unlink_prop(actor_index, result);
        memset(p, 0, 0x138);
        *(int16_t *)p = salt;
    }
    actor_init_prop_from_object(object_index, actor_index, result);
    return result;
}

extern "C" datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind)
{
    return halo::ai::prop_ops(actor_index).find_or_allocate_prop(object_index, kind);
}

namespace c_actor_find_or_create_shared_prop {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *prop_data;

extern void actor_target_reset_combat_flags(datum_index target_prop_index, datum_index actor_index, uint32_t unused,
    uint8_t already_noticed);
extern datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind);
extern void actor_target_data_refresh(datum_index actor_index, datum_index prop_index, void *scratch, uint32_t param4,
                         uint32_t flag);
extern void actor_target_update_tracking_speed(datum_index actor_index, datum_index prop_index, void *scratch);
extern uint8_t actor_target_has_conflicting_neighbor(datum_index actor_index, datum_index target_prop_index);
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
}
}

extern "C" datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag);

/**
 * actor_find_or_create_shared_prop: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_or_create_shared_prop.c.txt.
 *
 * @address 0x43eb30
 */
datum_index halo::ai::prop_ops::find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag)
{
    using namespace c_actor_find_or_create_shared_prop;
    datum_index result = (datum_index)0xffffffff;
    actor *self;
    uint8_t *object;
    int32_t cluster_ref;

    if (object_index == (datum_index)0xffffffff) {
        return result;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    object = *(uint8_t **)((uint8_t *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
    cluster_ref = *(int32_t *)(object + 0x1f8);
    if (cluster_ref == -1) {
        cluster_ref = *(int32_t *)(object + 500);
    }

    if ((((struct object *)object)->type == 0) && ((datum_index)cluster_ref != actor_index)) {
        datum_index cur = self->first_prop;

        for (;;) {
            prop *p;
            if (cur == (datum_index)0xffffffff) {
                goto not_found;
            }
            p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
            if ((p->object_index == object_index) ||
                ((p->swarm_owned != 0) && (p->owner_actor_index != (datum_index)0xffffffff) &&
                 ((int32_t)p->owner_actor_index == cluster_ref))) {
                break;
            }
            cur = p->next_in_actor;
        }

        {
            prop *p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
            result = cur;
            if (p->pair_index != (datum_index)0xffffffff) {
                result = p->pair_index;
            }
        }

        if (result == (datum_index)0xffffffff) {
        not_found:
            if ((create_if_missing != 0) && (self->active != 0)) {
                uint8_t scratch[56];

                result = actor_find_or_allocate_prop(actor_index, object_index,
                    (char)teams_are_enemies(((struct object *)object)->owner_team, ((struct actor *)self)->team));
                if (result != (datum_index)0xffffffff) {
                    prop *p = (prop *)((uint8_t *)prop_data->data + (result & 0xffff) * sizeof(prop));

                    actor_target_data_refresh(actor_index, result, scratch, 0, flag);
                    p->retain_timer = 0x1e;
                    p->just_created = 1;

                    if ((uint8_t)flag != 0 && (actor_target_update_tracking_speed(actor_index, result, scratch), 1 < p->perception_level)) {
                        uint8_t seen_flag = actor_target_has_conflicting_neighbor(actor_index, result);
                        p->state = 3;
                        actor_target_reset_combat_flags(result, actor_index, 0, seen_flag);
                    }
                }
            }
        }
    }

    return result;
}

extern "C" datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag)
{
    return halo::ai::prop_ops::find_or_create_shared_prop(object_index, actor_index, create_if_missing, flag);
}

namespace c_actor_find_prop_for_object {
extern "C" {
extern data_array *object_data;
extern data_array *actor_data;
extern data_array *prop_data;
}
}

extern "C" datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);

/**
 * actor_find_prop_for_object: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_prop_for_object.c.txt.
 *
 * @address 0x43ea80
 */
datum_index halo::ai::prop_ops::find_prop_for_object(datum_index object_index, datum_index actor_index)
{
    using namespace c_actor_find_prop_for_object;
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
    int32_t cluster_ref = *(int32_t *)(object + 0x1f8);
    actor *self;
    datum_index cur;

    if (cluster_ref == -1) {
        cluster_ref = *(int32_t *)(object + 500);
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    cur = self->first_prop;

    for (;;) {
        prop *p;
        if (cur == (datum_index)0xffffffff) {
            return (datum_index)0xffffffff;
        }
        p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));

        if (!(((-1 < p->state) && (p->state < 2)) ||
              ((p->object_index != object_index) &&
               ((p->swarm_owned == 0) || (p->owner_actor_index == (datum_index)0xffffffff) ||
                ((int32_t)p->owner_actor_index != cluster_ref))))) {
            return cur;
        }
        cur = p->next_in_actor;
    }
}

extern "C" datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index)
{
    return halo::ai::prop_ops::find_prop_for_object(object_index, actor_index);
}

namespace c_actor_get_target_prop_object_index {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

extern "C" datum_index actor_get_target_prop_object_index(datum_index actor_index);

/**
 * actor_get_target_prop_object_index: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_target_prop_object_index.c.txt.
 *
 * @address 0x4283d0
 */
datum_index halo::ai::prop_ops::get_target_prop_object_index()
{
    using namespace c_actor_get_target_prop_object_index;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
        return p->object_index;
    }
    return (datum_index)k_datum_index_none;
}

extern "C" datum_index actor_get_target_prop_object_index(datum_index actor_index)
{
    return halo::ai::prop_ops(actor_index).get_target_prop_object_index();
}

namespace c_actor_init_prop_from_object {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;
extern game_time_globals *game_time;

extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b);
extern uint8_t team_pair_override_get_flag(int16_t index_a, int16_t index_b);
}
}

extern "C" void actor_init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index);

/**
 * actor_init_prop_from_object: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_init_prop_from_object.c.txt.
 *
 * @address 0x43e640
 */
void halo::ai::prop_ops::init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index)
{
    using namespace c_actor_init_prop_from_object;
    actor *self;
    prop *p;

    if (prop_index == (datum_index)0xffffffff) {
        return;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    p = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));

    p->actor_index = actor_index;
    p->stimulus_type = -1;
    p->seen_state = -1;
    p->object_index = object_index;
    p->seen = 0;
    p->unknown_70 = 0.0f;
    p->information_age = -1;
    p->has_current_information = 0;
    p->information_source_actor = -1;
    p->last_perceived_time = -1;
    p->last_seen_time = -1;
    p->dead_confirmed = 0;
    p->owner_actor_index = (datum_index)0xffffffff;
    p->pair_index = (datum_index)0xffffffff;
    p->retain_timer = 0;
    p->last_engaged_time = -1;

    if (object_index != (datum_index)0xffffffff) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
        uint8_t *object_type = (uint8_t *)halo::cache::globals().tag_instances[*(uint16_t *)object & 0xffff].data;
        uint8_t is_vault;

        p->team = ((struct object *)object)->owner_team;

        p->enemy = teams_are_enemies(p->team, ((struct actor *)self)->team);
        p->allegiance = team_pair_flag_test(((struct actor *)self)->team, p->team);
        p->team_pair_status = team_pair_override_get_flag(((struct actor *)self)->team, p->team);

        is_vault = (*(uint8_t *)&((struct object *)object)->vitality_flags >> 2) & 1;
        p->dead = is_vault;
        p->danger_radius = *(float *)(object_type + 0x284);
        p->dead_not_feigning = (is_vault != 0) && (*(int16_t *)(object + 0x420) == 0);
        p->dead_ticks = (is_vault != 0) ? 1000 : 0;
        p->is_parented = *(int32_t *)&((struct object *)object)->owner_linkage != -1;

        if (*(int32_t *)(object + 0x1f8) == -1) {
            p->owner_actor_index = *(datum_index *)(object + 0x1f4);
        } else {
            p->swarm_owned = 1;
            p->owner_actor_index = *(datum_index *)(object + 0x1f8);
            p->swarm_reassign_time = game_time->game_time;
        }

        if (p->is_parented != 0) {
            p->actor_type = 6;
            p->next_in_actor = self->first_prop;
            self->first_prop = prop_index;
            return;
        }
        if (p->owner_actor_index != (datum_index)0xffffffff) {
            p->actor_type = ((actor *)((uint8_t *)actor_data->data + (p->owner_actor_index & 0xffff) * sizeof(actor)))->type;
            p->next_in_actor = self->first_prop;
            self->first_prop = prop_index;
            return;
        }
        p->actor_type = -1;
    }

    p->next_in_actor = self->first_prop;
    self->first_prop = prop_index;
}

extern "C" void actor_init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index)
{
    halo::ai::prop_ops::init_prop_from_object(object_index, actor_index, prop_index);
}

namespace c_actor_mark_prop_seen_with_delta {
extern "C" {
extern data_array *prop_data;

extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag);
extern void actor_queue_directional_reaction_event(const real_vector3d *direction, datum_index target_prop_index,
    datum_index actor_index);

#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
}
}

extern "C" void actor_mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta, const real_vector3d *direction);

/**
 * actor_mark_prop_seen_with_delta: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mark_prop_seen_with_delta.c.txt.
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
    prop_index = actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
    if (prop_index != k_datum_index_none) {
        uint8_t *p = PROP(prop_index);
        datum_index pair = *(datum_index *)(p + 0xc);
        int16_t kind;

        *(int16_t *)(p + 0x6c) = 0;
        p[0x74] = 1;
        *(float *)(p + 0x70) = delta + *(float *)(p + 0x70);
        if (pair != k_datum_index_none) {
            uint8_t *q = PROP(pair);

            *(int16_t *)(q + 0x6c) = 0;
            q[0x74] = 1;
            *(float *)(q + 0x70) = delta + *(float *)(q + 0x70);
        }
        kind = *(int16_t *)(p + 0x24);
        if (kind < 2 || kind > 3) {
            prop_index = k_datum_index_none;
        }
    }
    actor_queue_directional_reaction_event(direction, prop_index, actor_index);
}

extern "C" void actor_mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta, const real_vector3d *direction)
{
    halo::ai::prop_ops::mark_prop_seen_with_delta(object_index, actor_index, delta, direction);
}

#undef PROP

