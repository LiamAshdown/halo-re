#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/items/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern real equipment_network_update_position_tolerance;
extern double sqrt(double x);
extern network_id_table *object_network_id_table;
extern network_id_table *machine_table;
extern uint8_t network_object_index_cache[];
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern uint32_t sound_play_new(uint32_t sound_tag_id, void *parameters, uint32_t owner_index, int32_t extra_size, void *extra, uint32_t extra_count, uint32_t allow_deferred);
extern void *game_time;
extern int32_t k_equipment_minimum_age_ticks;
void halo::items::equipment_apply_network_update(datum_index item_index, uint32_t *update_record);
int32_t halo::items::equipment_build_network_update(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
void halo::items::equipment_create_from_creation_message(void *incoming_record);
void halo::items::equipment_definition_play_pickup_sound(uint32_t equipment_tag_id);
uint8_t halo::items::equipment_is_old_enough(uint32_t object_index);
void halo::items::equipment_network_baseline_take(uint32_t item_index);
uint8_t halo::items::equipment_new(uint32_t object_index);
void halo::items::equipment_new_from_placement(uint32_t equipment_object_index, ScenarioEquipment *placement);
void halo::items::equipment_pickup_play_sound(uint32_t object_index);
void halo::items::equipment_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3);
}

namespace halo::items {

/**
 * Member form of the original equipment_apply_network_update: apply network update.
 *
 * @address 0x4bc250
 */
void equipment_ref::apply_network_update(uint32_t *update_record)
{
    datum_index item_index = datum;
    object *obj;
    equipment_data *ed;
    weapon_network_update_header *header;
    equipment_network_state decoded;
    real dx, dy, dz;

    obj = halo::objects::object_try_and_get(item_index, _object_mask_equipment);
    if (obj == 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)update_record);
        return;
    }
    ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);
    header = (weapon_network_update_header *)update_record[0x11];

    if ((obj->flags & _object_took_network_update_bit) != 0 && *(int32_t *)update_record[0] == 1 &&
        (header->baseline_index != ed->network_baseline_index ||
         (header->sequence <= ed->network_sequence &&
          (int)((uint32_t)(header->sequence - ed->network_sequence) + 0xff) > 0x1d))) {
        halo::networking::message_delta_decode_compound_field_staged((void **)update_record);
        return;
    }

    decoded = ed->network_state;

    {
        uint8_t accept = (*(int32_t *)update_record[0] == 1)
                             ? halo::networking::message_delta_decode_compound_field_forced((void **)update_record, &decoded, (int32_t)&ed->network_state, 0)
                             : halo::networking::message_delta_decode_compound_field((void **)update_record, &decoded);
        if (accept != 0) {
            ed->network_sequence = header->sequence;
            obj->flags |= _object_took_network_update_bit;

            if (header->force_baseline != 0) {
                ed->network_baseline_index = header->baseline_index;
                ed->network_state = decoded;
            }

            obj->velocity = decoded.velocity;
            obj->angular_velocity = decoded.angular_velocity;
            obj->network_position = decoded.position;
            obj->network_velocity = decoded.velocity;
            obj->network_position_valid = 1;
            obj->network_velocity_valid = 1;

            dx = decoded.position.x - obj->position.x;
            dy = decoded.position.y - obj->position.y;
            dz = decoded.position.z - obj->position.z;
            if (equipment_network_update_position_tolerance < (real)sqrt(dx * dx + dy * dy + dz * dz) ||
                (obj->flags & _object_at_rest_bit) != 0) {
                halo::objects::object_set_position_and_recalculate(&decoded.position, item_index);
            }

            ed->last_update_valid = 1;
            ed->last_update_state = decoded;
        }
    }
}

/**
 * Member form of the original equipment_build_creation_message: build creation message.
 *
 * @address 0x4bbc90
 */
void equipment_ref::build_creation_message(uint32_t unused_arg2, uint32_t unused_arg3, uint32_t object_flags)
{
    uint32_t item_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[item_index & halo::k_slot_mask].data;
    int32_t item_hash = 0;
    int32_t parent_hash = 0;
    int32_t owner_hash = 0;
    equipment_creation_message message;
    void *item_ptr;

    if (item_index != halo::k_dword_none) {
        item_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, item_index);
    }
    if (obj->creator_object != halo::k_dword_none) {
        parent_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, obj->creator_object);
        if (parent_hash == -1) {
            parent_hash = 0;
        }
    }
    if (obj->owner_linkage != halo::k_dword_none) {
        owner_hash = halo::objects::hash_table_get(&machine_table->id_to_index, obj->owner_linkage);
        if (owner_hash == -1) {
            owner_hash = 0;
        }
    }
    if (item_hash == -1) {
        item_hash = halo::networking::network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)item_index);
    }

    message.definition_tag = obj->definition_tag;
    message.object_hash = item_hash;
    message.name_index = obj->owner_team;
    message.owner_hash = owner_hash;
    message.parent_hash = parent_hash;
    message.object_flags = object_flags;
    message.forward = obj->forward;
    message.up = obj->up;
    message.baseline_index = *((uint8_t *)obj + 0x245);
    {
        equipment_network_state *net = (equipment_network_state *)((uint8_t *)obj + 0x248);
        message.position = net->position;
        message.velocity = net->velocity;
        message.angular_velocity = net->angular_velocity;
    }

    item_ptr = &message;
    halo::networking::message_delta_encode_message((int32_t)unused_arg2, (int32_t)unused_arg3, 0, k_message_equipment_creation, 0, &item_ptr, 0, 1, 0);
}

/**
 * Builds and sends one network delta update for an equipment item: update_type == 1 sends a
 * full baseline (the live position/velocity/angular_velocity), anything else sends a delta
 * against the stored equipment_network_state. Advances the per-object sequence byte, wrapping
 * 0xff back to 0, whenever the send reports a positive result.
 *
 * @address 0x4bc0f0
 */
int32_t equipment_ref::build_network_update(uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type)
{
    uint32_t item_index = datum;
    object *obj = halo::objects::object_try_and_get(item_index, _object_mask_equipment);
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
        equipment_network_state *net = (equipment_network_state *)((uint8_t *)obj + 0x248);
        int32_t message_type = object_type_definitions[obj->type]->network_delta_message_type;
        void *header_ptr = &header;
        int32_t is_full_snapshot = (update_type == 1);
        void *items_array[1];
        void *type_offset;
        real snapshot[9];

        header.item_hash = 0;
        if (item_index != (uint32_t)k_datum_index_none) {
            header.item_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, item_index);
            if (header.item_hash == -1) {
                header.item_hash = 0;
            }
        }
        header.sequence = *((uint8_t *)obj + 0x246);
        header.baseline_index = *((uint8_t *)obj + 0x245);
        header.is_first_update = (uint8_t)(update_type == 0);

        if (!is_full_snapshot) {
            items_array[0] = net;
            type_offset = 0;
        } else {
            void *net_ptr = net;

            snapshot[0] = obj->position.x;
            snapshot[1] = obj->position.y;
            snapshot[2] = obj->position.z;
            snapshot[3] = obj->velocity.i;
            snapshot[4] = obj->velocity.j;
            snapshot[5] = obj->velocity.k;
            snapshot[6] = obj->angular_velocity.i;
            snapshot[7] = obj->angular_velocity.j;
            snapshot[8] = obj->angular_velocity.k;

            items_array[0] = snapshot;
            type_offset = &net_ptr;
        }

        result = halo::networking::message_delta_encode_message((int32_t)unused_arg2, (int32_t)unused_arg3, is_full_snapshot, message_type,
            (int)&header_ptr, items_array, (int)type_offset, 1, 0);
    }

    if (0 < result) {
        uint8_t sequence = *((uint8_t *)obj + 0x246) + 1;
        *((uint8_t *)obj + 0x246) = sequence;
        if ((int8_t)sequence == -1) {
            *((uint8_t *)obj + 0x246) = 0;
        }
    }
    return result;
}

/**
 * Handles an incoming network equipment-creation update: decodes the message, re-orthonormalizes
 * the replicated forward/up basis, resolves the owner and parent hashes into object handles,
 * spawns the equipment through object_new_with_datum_role_control, registers it in the
 * networking hash table, and then seeds its network block, position, velocity and spin.
 *
 * @address 0x4bbe20
 */
void equipment_ref::create_from_creation_message(void *incoming_record)
{
    equipment_creation_message decoded;
    real_vector3d forward, up, cross;
    object_placement_data placement;
    uint32_t role_material, owner_material;
    datum_index new_object_index;
    object *obj;
    equipment_data *ed;
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
    ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);

    obj->flags |= decoded.object_flags;
    ed->network_state.position = decoded.position;
    ed->network_state.velocity = decoded.velocity;
    ed->network_state.angular_velocity = decoded.angular_velocity;
    ed->network_baseline_index = decoded.baseline_index;
    ed->network_state_valid = 1;
    ed->network_sequence = 0;

    halo::objects::object_set_position_and_recalculate(&ed->network_state.position, new_object_index);

    obj->velocity = ed->network_state.velocity;
    obj->angular_velocity = ed->network_state.angular_velocity;
}

/**
 * Plays the pickup_sound of an Equipment tag named by tag id, with no live object involved.
 *
 * @address 0x4bbbd0
 */
void equipment_ref::definition_play_pickup_sound(uint32_t equipment_tag_id)
{
    Equipment *tag;
    int32_t pickup_sound_tag_id;
    uint8_t parameters[16];

    tag = (Equipment *)halo::cache::globals().tag_instances[equipment_tag_id & halo::k_slot_mask].data;
    pickup_sound_tag_id = *(int32_t *)&tag->pickup_sound.tag_id;

    if (pickup_sound_tag_id != -1) {
        ((sound_location *)parameters)->type = 0;
        ((sound_location *)parameters)->scale = 1.0f;
        ((sound_location *)parameters)->gain = 1.0f;
        halo::sound::sound_play_new((uint32_t)pickup_sound_tag_id, (sound_location *)parameters, 0xffffffff, 0, 0, 0, 0);
    }
}

/**
 * The equipment row's "is old enough" hook (object_type_definition +0x74). An object that has
 * never been stamped (object.network_update_tick == -1) always counts as old enough; otherwise it is old
 * enough once the game tick has advanced past the stamped tick plus this type's minimum age.
 *
 * @address 0x4bc420
 */
uint8_t equipment_ref::is_old_enough()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    int32_t stamp = obj->network_update_tick;

    if (stamp == -1) {
        return 1;
    }
    return stamp + k_equipment_minimum_age_ticks <= *(int32_t *)((uint8_t *)game_time + 0xc);
}

/**
 * Takes a fresh network baseline for an equipment item: bumps network_baseline_index, snapshots
 * the object's current position, velocity and angular_velocity into equipment_data.network_state,
 * marks the state valid and resets the sequence counter to 0.
 * FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x4bc070
 */
void equipment_ref::network_baseline_take()
{
    uint32_t item_index = datum;
    object *obj = halo::objects::object_try_and_get(item_index, _object_mask_equipment);

    if (obj != 0) {
        equipment_data *ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);

        ed->network_baseline_index++;
        ed->network_state.position = obj->position;
        ed->network_state.velocity = obj->velocity;
        ed->network_state_valid = 1;
        ed->network_sequence = 0;
        ed->network_state.angular_velocity = obj->angular_velocity;
    }
}

/**
 * The equipment row's query_create hook (object_type_definition +0x28), run once for a freshly
 * activated equipment object. When the game is networked it clears the item's replicated-state
 * bookkeeping (equipment_data's three network bytes, plus the still-unnamed object+0x9 byte) so
 * a locally-created equipment item does not start out claiming a stale network baseline. Always
 * reports success.
 *
 * @address 0x4bba90
 */
uint8_t equipment_ref::create()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    equipment_data *ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);

    if (halo::networking::globals().game_mode == 1 || halo::networking::globals().game_mode == 2) {
        ed->network_state_valid = 0;
        ed->network_baseline_index = 0;
        ed->network_sequence = 0;
        obj->network_state_009 = 0;
    }
    return 1;
}

/**
 * The equipment row's notify_two_args_2c hook (object_type_definition +0x2c). Applies a
 * ScenarioEquipment placement record's at-rest and does-accelerate flags to a freshly created
 * equipment object, and nudges it slightly off the placement point when it is not going to
 * start out resting.
 *
 * @address 0x4bbae0
 */
void equipment_ref::new_from_placement(ScenarioEquipment *placement)
{
    uint32_t equipment_object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[equipment_object_index & halo::k_slot_mask].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((placement->misc_flags & 1) == 0) {
        obj->flags = obj->flags & ~(uint32_t)_object_at_rest_bit;
    } else {
        obj->flags = obj->flags | _object_at_rest_bit;
    }
    obj->flags = obj->flags | 0x60000;

    if ((placement->misc_flags & 4) == 0) {
        id->flags = id->flags | _item_does_not_accelerate_bit;
    } else {
        id->flags = id->flags & ~(uint32_t)_item_does_not_accelerate_bit;
    }

    if ((placement->misc_flags & 1) == 0) {
        obj->position.z = obj->position.z + 0.05f;
    }
}

/**
 * Clears item_flags bit 0x40 on an item that is being picked up, then plays the Equipment tag's
 * pickup_sound if the tag has one.
 *
 * @address 0x4bbb50
 */
void equipment_ref::pickup_play_sound()
{
    uint32_t object_index = datum;
    object *obj;
    item_data *item;
    Equipment *tag;
    int32_t pickup_sound_tag_id;
    uint8_t parameters[16];

    obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    tag = (Equipment *)halo::cache::globals().tag_instances[obj->definition_tag & halo::k_slot_mask].data;

    item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    item->flags &= ~(uint32_t)_item_unknown_40_bit;

    pickup_sound_tag_id = *(int32_t *)&tag->pickup_sound.tag_id;
    if (pickup_sound_tag_id != -1) {
        ((sound_location *)parameters)->type = 0;
        ((sound_location *)parameters)->scale = 1.0f;
        ((sound_location *)parameters)->gain = 1.0f;
        halo::sound::sound_play_new((uint32_t)pickup_sound_tag_id, (sound_location *)parameters, 0xffffffff, 0, 0, 0, 0);
    }
}

/**
 * The equipment row's "send creation" hook (object_type_definition +0x64). Builds and sends the
 * equipment creation message, advertising _object_at_rest_bit in the message's object_flags only
 * when the item is at rest and that rest is not on a structure surface (i.e. it is resting on
 * another object, or its rest has not been resolved to a surface yet).
 *
 * @address 0x4bbc30
 */
void equipment_ref::send_creation(uint32_t arg2, uint32_t arg3)
{
    uint32_t item_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[item_index & halo::k_slot_mask].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((obj->flags & _object_at_rest_bit) != 0 && (id->flags & _item_at_rest_on_structure_bit) == 0) {
        halo::items::equipment_build_creation_message(item_index, arg2, arg3, _object_at_rest_bit);
        return;
    }
    halo::items::equipment_build_creation_message(item_index, arg2, arg3, 0);
}

}

namespace halo::items {

void equipment_apply_network_update(datum_index item_index, uint32_t *update_record)
{
    halo::items::equipment_ref(item_index).apply_network_update(update_record);
}

void equipment_build_creation_message(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, uint32_t object_flags)
{
    halo::items::equipment_ref(item_index).build_creation_message(unused_arg2, unused_arg3, object_flags);
}

int32_t equipment_build_network_update(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type)
{
    return halo::items::equipment_ref(item_index).build_network_update(unused_arg2, unused_arg3, update_type);
}

void equipment_create_from_creation_message(void *incoming_record)
{
    halo::items::equipment_ref::create_from_creation_message(incoming_record);
}

void equipment_definition_play_pickup_sound(uint32_t equipment_tag_id)
{
    halo::items::equipment_ref::definition_play_pickup_sound(equipment_tag_id);
}

uint8_t equipment_is_old_enough(uint32_t object_index)
{
    return halo::items::equipment_ref(object_index).is_old_enough();
}

void equipment_network_baseline_take(uint32_t item_index)
{
    halo::items::equipment_ref(item_index).network_baseline_take();
}

uint8_t equipment_new(uint32_t object_index)
{
    return halo::items::equipment_ref(object_index).create();
}

void equipment_new_from_placement(uint32_t equipment_object_index, ScenarioEquipment *placement)
{
    halo::items::equipment_ref(equipment_object_index).new_from_placement(placement);
}

void equipment_pickup_play_sound(uint32_t object_index)
{
    halo::items::equipment_ref(object_index).pickup_play_sound();
}

void equipment_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3)
{
    halo::items::equipment_ref(item_index).send_creation(arg2, arg3);
}

}
