#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/items/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/items/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"

static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &weapon_network_update_position_tolerance = halo::link::ref<real>(halo::items::vars().weapon_network_update_position_tolerance);
static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);
static auto &network_object_index_cache = halo::link::ref<uint8_t []>(halo::units::vars().network_object_index_cache);
static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);

namespace halo::items {

/**
 * Applies a received ammo-pickup network event to one of a weapon's magazines, adding the
 * signed round count it carries to that magazine's reserve, and returns a pointer to the
 * magazine state it touched.
 *
 * @address 0x4c25a0
 */
int32_t weapon_ref::add_ammunition(void **message_record)
{
    weapon_ammo_pickup_message decoded;
    object *item_obj;
    weapon_data *wd;
    datum_index item_index;
    int16_t *rounds_unloaded;

    if (*(int32_t *)*message_record != 0) {
        return halo::networking::message_delta_decode_compound_field_staged(message_record);
    }
    if ((int8_t)halo::networking::message_delta_decode_compound_field(message_record, &decoded) == 0) {
        return 0;
    }

    item_index = k_datum_index_none;
    if (decoded.object_hash != 0) {
        item_index = object_network_id_table->handles[decoded.object_hash];
    }

    item_obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return (int32_t)(long)item_obj;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    rounds_unloaded = &wd->magazines[decoded.magazine_index].rounds_unloaded;
    *rounds_unloaded = (int16_t)(*rounds_unloaded + decoded.rounds);
    return (int32_t)(long)&wd->magazines[decoded.magazine_index];
}

/**
 * Applies a host-confirmed ammo correction to one magazine, forces it into the chamber-pending
 * state with a fresh (zero) state timer, and clears the prediction-pending flag.
 *
 * @address 0x4c3870
 */
void weapon_ref::apply_ammo_correction(void **message_record)
{
    weapon_magazine_ammo_message decoded;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_state *magazine;
    datum_index item_index;

    if (*(int32_t *)*message_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged(message_record);
        return;
    }
    if ((int8_t)halo::networking::message_delta_decode_compound_field(message_record, &decoded) == 0) {
        return;
    }

    item_index = k_datum_index_none;
    if (decoded.object_hash != 0) {
        item_index = object_network_id_table->handles[decoded.object_hash];
    }

    item_obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    magazine = &wd->magazines[decoded.magazine_index];
    magazine->rounds_unloaded = decoded.rounds_unloaded;
    magazine->rounds_loaded = decoded.rounds_loaded;
    magazine->state_ticks = 0;
    magazine->state = _weapon_magazine_chamber_pending;
    wd->flags = wd->flags & ~(uint32_t)_weapon_ammo_prediction_pending_bit;
}

/**
 * Applies a host-confirmed ammo correction to one magazine, clears the prediction-pending flag,
 * and then re-derives the weapon's trigger and magazine state from scratch.
 *
 * @address 0x4c4ac0
 */
void weapon_ref::apply_ammo_correction_and_resync(void **message_record)
{
    weapon_magazine_ammo_message decoded;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_state *magazine;
    datum_index item_index;

    if (*(int32_t *)*message_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged(message_record);
        return;
    }
    if ((int8_t)halo::networking::message_delta_decode_compound_field(message_record, &decoded) == 0) {
        return;
    }

    item_index = k_datum_index_none;
    if (decoded.object_hash != 0) {
        item_index = object_network_id_table->handles[decoded.object_hash];
    }

    item_obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    magazine = &wd->magazines[decoded.magazine_index];
    magazine->rounds_unloaded = decoded.rounds_unloaded;
    magazine->rounds_loaded = decoded.rounds_loaded;
    wd->flags = wd->flags & ~(uint32_t)_weapon_ammo_prediction_pending_bit;
    halo::items::weapon_reset_triggers(item_index);
}

/**
 * Applies an incoming network state update to a weapon object, rejecting it if it is not newer
 * than the item's own recorded baseline/sequence, then refreshes the object's position/velocity
 * (relinking into the world when it moved far enough or a flag demands it) and the ammo/age
 * fields carried in the update.
 *
 * @address 0x4c6070
 */
void weapon_ref::apply_network_update(uint32_t *update_record)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    weapon_network_update_header *header;
    weapon_network_state snapshot;
    uint8_t accept;
    real dx, dy, dz;

    item_obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)update_record);
        return;
    }
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    header = (weapon_network_update_header *)update_record[0x11];

    if ((item_obj->flags & _object_took_network_update_bit) != 0 && *(int32_t *)update_record[0] == 1 &&
        (header->baseline_index != wd->network_baseline_index ||
         (header->sequence <= wd->network_sequence &&
          (int)((uint32_t)(header->sequence - wd->network_sequence) + 0xff) > 0x1d))) {
        halo::networking::message_delta_decode_compound_field_staged((void **)update_record);
        return;
    }

    snapshot = wd->network_state;

    if (*(int32_t *)update_record[0] == 1) {
        accept = halo::networking::message_delta_decode_compound_field_forced((void **)update_record, &snapshot, (int32_t)&wd->network_state, 0);
    } else {
        accept = halo::networking::message_delta_decode_compound_field((void **)update_record, &snapshot);
    }

    if (accept != 0) {
        wd->network_sequence = header->sequence;
        item_obj->flags = item_obj->flags | _object_took_network_update_bit;

        if (header->force_baseline != 0) {
            wd->network_baseline_index = header->baseline_index;
            wd->network_state = snapshot;
        }

        item_obj->velocity = snapshot.velocity;
        item_obj->network_position = snapshot.position;
        item_obj->network_velocity = snapshot.velocity;
        item_obj->network_position_valid = 1;
        item_obj->network_velocity_valid = 1;

        if (wd->magazines[0].state != 1) {
            wd->magazines[0].rounds_unloaded = snapshot.rounds_unloaded[0];
        }
        if (wd->magazines[1].state != 1) {
            wd->magazines[1].rounds_unloaded = snapshot.rounds_unloaded[1];
        }
        if (*(int32_t *)update_record[0] == 0) {
            wd->age = snapshot.age;
        }

        dx = snapshot.position.x - item_obj->position.x;
        dy = snapshot.position.y - item_obj->position.y;
        dz = snapshot.position.z - item_obj->position.z;
        if ((item_obj->flags & _object_needs_cluster_update_bit) != 0 &&
            (weapon_network_update_position_tolerance < (real)halo::libm::sqrt((double)(dy * dy + dx * dx + dz * dz)) ||
             (item_obj->flags & _object_at_rest_bit) != 0 || header->force_baseline != 0)) {
            halo::objects::object_set_position_and_recalculate(&snapshot.position, item_index);
        }

        wd->last_update_valid = 1;
        wd->last_update_state = snapshot;
    }
}

/**
 * Builds and encodes a weapon creation network message (type 0x20).
 *
 * @address 0x4c5a50
 */
void weapon_ref::build_creation_message(uint32_t unused_param_2, uint32_t unused_param_3, uint32_t object_flags)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    weapon_creation_message message;
    void *items[2];
    int32_t object_hash = 0;
    int32_t owner_hash = 0;
    int32_t parent_hash = 0;

    (void)unused_param_2;
    (void)unused_param_3;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    if (item_index != k_datum_index_none) {
        object_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, item_index);
    }
    if (item_obj->creator_object != k_datum_index_none) {
        parent_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, item_obj->creator_object);
        if (parent_hash == -1) parent_hash = 0;
    }
    if (item_obj->owner_linkage != k_datum_index_none) {
        owner_hash = halo::objects::hash_table_get(&machine_table->id_to_index, item_obj->owner_linkage);
        if (owner_hash == -1) owner_hash = 0;
    }
    if (object_hash == -1) {
        object_hash = halo::networking::network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)item_index);
    }

    message.definition_tag = item_obj->definition_tag;
    message.object_hash = object_hash;
    message.name_index = item_obj->owner_team;
    message.owner_hash = owner_hash;
    message.parent_hash = parent_hash;
    message.object_flags = object_flags;
    message.position = wd->network_state.position;
    message.forward = item_obj->forward;
    message.up = item_obj->up;
    message.velocity = wd->network_state.velocity;
    message.baseline_index = wd->network_baseline_index;
    message.rounds_unloaded[0] = wd->network_state.rounds_unloaded[0];
    message.rounds_unloaded[1] = wd->network_state.rounds_unloaded[1];
    message.age = wd->network_state.age;
    message.rounds_loaded[0] = wd->magazines[0].rounds_loaded;
    message.rounds_loaded[1] = wd->magazines[1].rounds_loaded;

    items[0] = &message;
    items[1] = 0;
    halo::networking::message_delta_encode_message((int32_t)unused_param_2, (int32_t)unused_param_3, 0, k_message_weapon_creation, 0, items, 0, 1, 0);
}

/**
 * Builds and sends one network delta update for a weapon item: update_type == 1 sends a full
 * baseline (live position/velocity plus both magazines' reserve and the network age), anything
 * else sends a delta against the stored weapon_network_state. Advances the per-object sequence
 * byte, wrapping 0xff back to 0, whenever the send reports a positive result.
 *
 * @address 0x4c5f10
 */
int32_t weapon_ref::build_network_update(uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type)
{
    uint32_t item_index = datum;
    object *obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);
    int32_t result;

    if (obj == 0) {
        return 0;
    }

    {
        struct {
            int32_t item_hash;
            uint8_t baseline_index;
            uint8_t sequence;
            uint8_t is_first_update;
        } header;
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
        int32_t message_type = object_type_definitions[obj->type]->network_delta_message_type;
        void *header_ptr = &header;
        int32_t is_full_snapshot = (update_type == 1);
        void *items_array[1];
        void *type_offset;
        struct {
            real position[3];
            real velocity[3];
            int16_t rounds_unloaded[2];
            real age;
        } snapshot;

        header.item_hash = 0;
        if (item_index != (uint32_t)k_datum_index_none) {
            header.item_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, item_index);
            if (header.item_hash == -1) {
                header.item_hash = 0;
            }
        }
        header.sequence = wd->network_sequence;
        header.baseline_index = wd->network_baseline_index;
        header.is_first_update = (uint8_t)(update_type == 0);

        if (!is_full_snapshot) {
            items_array[0] = &wd->network_state;
            type_offset = 0;
        } else {
            void *net_ptr = &wd->network_state;

            snapshot.position[0] = obj->position.x;
            snapshot.position[1] = obj->position.y;
            snapshot.position[2] = obj->position.z;
            snapshot.velocity[0] = obj->velocity.i;
            snapshot.velocity[1] = obj->velocity.j;
            snapshot.velocity[2] = obj->velocity.k;
            snapshot.rounds_unloaded[0] = wd->magazines[0].rounds_unloaded;
            snapshot.rounds_unloaded[1] = wd->magazines[1].rounds_unloaded;
            snapshot.age = wd->age;

            items_array[0] = &snapshot;
            type_offset = &net_ptr;
        }

        result = halo::networking::message_delta_encode_message((int32_t)unused_arg2, (int32_t)unused_arg3, is_full_snapshot, message_type,
            (int)&header_ptr, items_array, (int)type_offset, 1, 0);
    }

    if (0 < result) {
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
        uint8_t sequence = wd->network_sequence + 1;
        wd->network_sequence = sequence;
        if ((int8_t)sequence == -1) {
            wd->network_sequence = 0;
        }
    }
    return result;
}

/**
 * Handles an incoming network weapon-creation update: decodes the message, re-orthonormalizes
 * the replicated forward/up basis, resolves the owner and parent hashes into object handles,
 * spawns the weapon through object_new_with_datum_role_control, registers it in the networking
 * hash table, and then seeds its network block, position, velocity and per-magazine ammo.
 *
 * @address 0x4c5c10
 */
void weapon_ref::create_from_creation_message(void *incoming_record)
{
    weapon_creation_message decoded;
    real_vector3d forward, up, cross;
    object_placement_data placement;
    uint32_t role_material, owner_material;
    datum_index new_object_index;
    object *obj;
    weapon_data *wd;
    uint8_t *zero;
    int32_t i;

    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)incoming_record);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)incoming_record, &decoded) != 1) {
        return;
    }

    forward = decoded.forward;
    up = decoded.up;
    halo::math::vector3d_cross_product(cross, up, forward);
    halo::math::vector3d_cross_product(up, forward, cross);
    halo::math::vector3d_normalize_with_length(forward);
    halo::math::vector3d_normalize_with_length(up);

    role_material = halo::k_dword_none;
    if (decoded.parent_hash != 0) {
        role_material = ((uint32_t *)object_network_id_table->handles)[decoded.parent_hash];
    }
    owner_material = halo::k_dword_none;
    if (decoded.owner_hash != 0) {
        owner_material = ((uint32_t *)machine_table->handles)[decoded.owner_hash];
    }

    zero = (uint8_t *)&placement;
    for (i = 0; i < (int32_t)sizeof(placement); i++) {
        zero[i] = 0;
    }
    placement.definition_tag = decoded.definition_tag;
    placement.owner_linkage = owner_material;
    placement.role = role_material;
    placement.owner_team = decoded.name_index;
    placement.position = decoded.position;
    placement.forward = forward;
    placement.up = up;

    new_object_index = halo::objects::object_new_with_datum_role_control(&placement, 1);
    if (new_object_index == (datum_index)k_datum_index_none) {
        return;
    }

    halo::networking::network_index_cache_insert_if_free(network_object_index_cache, decoded.object_hash, (int32_t)new_object_index);

    obj = ((object_header *)halo::objects::globals().object_data->data)[new_object_index & halo::k_slot_mask].data;
    wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);

    obj->flags |= decoded.object_flags;
    wd->network_state.position = decoded.position;
    wd->network_state.velocity = decoded.velocity;
    wd->network_state.rounds_unloaded[0] = decoded.rounds_unloaded[0];
    wd->network_state.rounds_unloaded[1] = decoded.rounds_unloaded[1];
    wd->network_state.age = decoded.age;
    wd->network_baseline_index = decoded.baseline_index;
    wd->network_state_valid = 1;
    wd->network_sequence = 0;

    halo::objects::object_set_position_and_recalculate(&wd->network_state.position, new_object_index);

    obj->velocity = wd->network_state.velocity;
    wd->magazines[0].rounds_unloaded = wd->network_state.rounds_unloaded[0];
    wd->magazines[1].rounds_unloaded = wd->network_state.rounds_unloaded[1];
    wd->age = wd->network_state.age;
    wd->magazines[0].rounds_loaded = decoded.rounds_loaded[0];
    wd->magazines[1].rounds_loaded = decoded.rounds_loaded[1];
}

/**
 * Takes a fresh network baseline for a weapon: bumps network_baseline_index, snapshots the
 * object's current position and velocity, both magazines' rounds_unloaded and the weapon's age
 * into weapon_data.network_state, marks the state valid and resets the sequence counter to 0.
 * FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x4c5e80
 */
void weapon_ref::network_baseline_take()
{
    uint32_t item_index = datum;
    object *obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);

    if (obj != 0) {
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);

        wd->network_baseline_index++;
        wd->network_state.position = obj->position;
        wd->network_state.velocity = obj->velocity;
        wd->network_state.rounds_unloaded[0] = wd->magazines[0].rounds_unloaded;
        wd->network_state_valid = 1;
        wd->network_sequence = 0;
        wd->network_state.rounds_unloaded[1] = wd->magazines[1].rounds_unloaded;
        wd->network_state.age = wd->age;
    }
}

/**
 * Applies a client-predicted ammo count for one magazine and marks the weapon as having a
 * prediction still awaiting the host's confirmation.
 *
 * @address 0x4c3530
 */
void weapon_ref::predict_ammo(void **message_record)
{
    weapon_magazine_ammo_message decoded;
    object *item_obj;
    weapon_data *wd;
    datum_index item_index;

    if (*(int32_t *)*message_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged(message_record);
        return;
    }
    if ((int8_t)halo::networking::message_delta_decode_compound_field(message_record, &decoded) == 0) {
        return;
    }

    item_index = k_datum_index_none;
    if (decoded.object_hash != 0) {
        item_index = object_network_id_table->handles[decoded.object_hash];
    }

    item_obj = halo::objects::object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->predicted_rounds_unloaded[decoded.magazine_index] = decoded.rounds_unloaded;
    wd->predicted_rounds_loaded[decoded.magazine_index] = decoded.rounds_loaded;
    wd->flags = wd->flags | _weapon_ammo_prediction_pending_bit;
}

/**
 * The weapon row's "send creation" hook (object_type_definition +0x64). Builds and sends the
 * weapon creation message, advertising _object_at_rest_bit in the message's object_flags only
 * when the item is at rest and that rest is not on a structure surface.
 *
 * @address 0x4c59f0
 */
void weapon_ref::send_creation(uint32_t arg2, uint32_t arg3)
{
    uint32_t item_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[item_index & halo::k_slot_mask].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((obj->flags & _object_at_rest_bit) != 0 && (id->flags & _item_at_rest_on_structure_bit) == 0) {
        halo::items::weapon_build_creation_message(item_index, arg2, arg3, _object_at_rest_bit);
        return;
    }
    halo::items::weapon_build_creation_message(item_index, arg2, arg3, 0);
}

}

namespace halo::items {

int32_t weapon_add_ammunition(void **message_record)
{
    return halo::items::weapon_ref::add_ammunition(message_record);
}

void weapon_apply_ammo_correction(void **message_record)
{
    halo::items::weapon_ref::apply_ammo_correction(message_record);
}

void weapon_apply_ammo_correction_and_resync(void **message_record)
{
    halo::items::weapon_ref::apply_ammo_correction_and_resync(message_record);
}

void weapon_apply_network_update(datum_index item_index, uint32_t *update_record)
{
    halo::items::weapon_ref(item_index).apply_network_update(update_record);
}

void weapon_build_creation_message(datum_index item_index, uint32_t unused_param_2, uint32_t unused_param_3, uint32_t object_flags)
{
    halo::items::weapon_ref(item_index).build_creation_message(unused_param_2, unused_param_3, object_flags);
}

int32_t weapon_build_network_update(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type)
{
    return halo::items::weapon_ref(item_index).build_network_update(unused_arg2, unused_arg3, update_type);
}

void weapon_create_from_creation_message(void *incoming_record)
{
    halo::items::weapon_ref::create_from_creation_message(incoming_record);
}

void weapon_network_baseline_take(uint32_t item_index)
{
    halo::items::weapon_ref(item_index).network_baseline_take();
}

void weapon_predict_ammo(void **message_record)
{
    halo::items::weapon_ref::predict_ammo(message_record);
}

void weapon_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3)
{
    halo::items::weapon_ref(item_index).send_creation(arg2, arg3);
}

}
