#include "halo/objects/record_access.hpp"
#include "halo/objects/object_lifetime.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/core/tag_groups.hpp"
#include "game.h"
#include "units.h"
#include "effects.h"
#include "networking.h"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"

static auto &game_looping_sound_data = halo::link::ref<data_array *>(halo::ui::vars().game_looping_sound_data);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &network_object_index_cache = halo::link::ref<void *>(halo::units::vars().network_object_index_cache);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &particle_system_data = halo::link::ref<data_array *>(halo::effects::vars().particle_system_data);
static auto &object_delete_callbacks = halo::link::ref<void (*[3])(uint32_t object_index)>(halo::objects::vars().object_delete_callbacks);

/**
 * Runs the common teardown before an object datum is freed.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX).
 *
 * @address 0x004edc80
 */
void halo::objects::ObjectLifetime::delete_teardown()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    halo::objects::object_set_health_frozen_flag(object_index);

    if (definition->collision_model.tag_id.index != halo::k_word_none) {

        halo::effects::effect_new_on_object(object_index,
            *(datum_index *)(halo::objects::tag_record_bytes(definition->collision_model.tag_id.index) + 0xc8),
            object_index, -1, 0.0f, 0.0f, 0, 0);
    }

    halo::objects::object_children_recurse_prune(object_index);

    obj = headers[halo::datum_slot(object_index)].data;
    if (obj->network_role == 0 || obj->network_role == 3) {
        if (obj->network_role == 0) {
            halo::objects::object_delete_unparented(object_index);
        }
        halo::objects::object_delete_recursive(object_index, 0);
    }
}

/**
 * Clears the object header's pending flag and returns whether it was set.
 *
 * Original register convention: object index in ECX (in_ECX).
 *
 * @address 0x004f46b0
 */
uint8_t halo::objects::ObjectLifetime::datum_consume_pending_flag()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    uint8_t *raw = (uint8_t *)obj;
    int changed = (obj->flags & _object_changed_bit) != 0;

    if (changed) {
        obj->flags &= ~(uint32_t)_object_changed_bit;
    }
    if (raw[8] != 0 && (obj->flags & _object_at_rest_bit) != 0) {
        uint8_t latch = raw[9];
        raw[9] = 1;
        return latch == 0 || changed;
    }
    raw[9] = 0;
    return changed;
}

/**
 * Marks an object for deletion at the next garbage collection.
 *
 * Original register convention: object index in EAX (in_EAX).
 *
 * @address 0x004f50f0
 */
void halo::objects::ObjectLifetime::mark_pending_delete()
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;

    if ((header->flags & _object_header_active_bit) == 0 &&
        (obj->flags & _object_do_not_delete_bit) == 0 &&
        obj->parent_object == k_datum_index_none) {
        header->flags |= _object_header_active_bit;
    }
}

/**
 * Clears the pending-delete flag of an object header.
 *
 * Original register convention: object index in EAX (in_EAX).
 *
 * @address 0x004f5130
 */
void halo::objects::ObjectLifetime::clear_pending_delete_flag()
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);

    if ((header->flags & _object_header_active_bit) != 0) {
        header->flags &= (uint8_t)~_object_header_active_bit;
    }
}

namespace {
}

/**
 * Deletes an object together with its children and optionally its siblings.
 *
 * @address 0x004f59d0
 */
void halo::objects::ObjectLifetime::delete_recursive(uint8_t recurse_siblings)
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;
    Object *object_tag;

    if (obj->first_child_object != k_datum_index_none) {
        halo::objects::object_delete_recursive(obj->first_child_object, 1);
    }
    if (recurse_siblings != 0 && obj->next_object != k_datum_index_none) {
        halo::objects::object_delete_recursive(obj->next_object, 1);
    }

    header->flags |= _object_header_delete_pending_bit;

    header = (object_header *)object_data->data + halo::datum_slot(object_index);
    obj = header->data;
    object_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    if (halo::objects::tag_handle(object_tag->model) != k_datum_index_none &&
        (obj->flags & _object_no_collision_bit) == 0) {

        halo::objects::object_for_each_light_attachment(object_index, 1, 0);
    }

    header = (object_header *)object_data->data + halo::datum_slot(object_index);
    obj->flags |= _object_no_collision_bit;
    header->flags &= (uint8_t)~_object_header_visible_bit;

    halo::objects::object_release_render_cache_slot(object_index);
}

/**
 * Deletes an object that has no parent.
 *
 * Original register convention: object index in EDI (unaff_EDI).
 *
 * @address 0x004f5aa0
 */
void halo::objects::ObjectLifetime::delete_unparented()
{
    uint32_t object_index = handle;
    object_header *header;
    int32_t looked_up = 0;
    int32_t *scratch_pointer;
    int32_t scratch_tail;
    int32_t encoded_length;

    if (object_index != k_datum_index_none) {
        looked_up = halo::objects::hash_table_get(&object_network_id_table->id_to_index, object_index);

        if (looked_up == -1) {
            looked_up = 0;
        }
    }

    scratch_pointer = &looked_up;
    scratch_tail = 0;

    encoded_length = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0, 0,
                                                  (void **)&scratch_pointer, 0, 1, 0);
    (void)scratch_tail;

    header = (object_header *)object_data->data + halo::datum_slot(object_index);
    if ((header->flags & _object_header_delete_pending_bit) == 0) {
        halo::networking::network_index_cache_remove((uint8_t *)&network_object_index_cache, object_index);
    }

    if (encoded_length > 0) {
        halo::networking::network_session_broadcast_to_flagged(encoded_length, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
    }
}

/**
 * Deletes the object referenced by a pooled network node record.
 *
 * Original register convention: some caller-owned record pointer in EAX (in_EAX, same shape as
 * object_type_override_call_0x70_release_node's first argument), pooled node id in ECX (in_ECX).
 *
 * @address 0x004f5b50
 */
void halo::objects::ObjectLifetime::delete_by_pooled_node_id(int32_t **record)
{
    uint32_t pooled_node_id;
    char preconditions_ok;
    uint32_t object_index;
    object_header *header;
    int32_t node_table;

    if (**record != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)record);
        return;
    }

    preconditions_ok = halo::networking::message_delta_decode_compound_field((void **)record, &pooled_node_id);

    if (preconditions_ok != 0 && pooled_node_id != 0) {
        node_table = (int32_t)object_network_id_table->handles;
        object_index = *(uint32_t *)(node_table + pooled_node_id * 4);
        if (object_index != k_datum_index_none) {
            header = (object_header *)object_data->data + halo::datum_slot(object_index);
            if ((header->flags & _object_header_delete_pending_bit) == 0) {
                halo::networking::network_index_cache_remove((uint8_t *)&network_object_index_cache, object_index);
            }
            if (halo::objects::object_try_and_get(object_index, _object_mask_all) != 0) {

                halo::objects::object_delete_recursive(object_index, 0);
            }
        }
    }
}

/**
 * Deletes an object according to its type's deletion category.
 *
 * @address 0x004f5bd0
 */
void halo::objects::ObjectLifetime::destroy()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data + halo::datum_slot(object_index))->data;

    if (obj->network_role == 0) {
        halo::objects::object_delete_unparented(object_index);
    } else if (obj->network_role != 3) {
        return;
    }
    halo::objects::object_delete_recursive(object_index, 0);
}

/**
 * Returns whether the object is marked for deletion.
 *
 * Original register convention: object index in EAX (in_EAX).
 *
 * @address 0x004f5c10
 */
uint8_t halo::objects::ObjectLifetime::is_delete_pending()
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    return (header->flags & _object_header_delete_pending_bit) != 0;
}

/**
 * Clears every reference other objects hold to an object that is about to die.
 *
 * Original register convention: the dying object index is the sole, genuinely-stack, parameter, confirmed against objdump 0x4f7415 mov esi,[esp+0x20].
 *
 * @address 0x004f73e0
 */
void halo::objects::ObjectLifetime::clear_references_to_object()
{
    uint32_t dying_object_index = handle;
    object_iterator iterator;
    object *obj;

    iterator.type_mask = halo::to_bits(halo::objects::object_mask::all);
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != nullptr) {
        if (obj->damage_owner == dying_object_index) {
            obj->damage_owner = k_datum_index_none;
        }
        halo::objects::object_type_definitions_notify_0x3c(iterator.handle, dying_object_index);
        obj = halo::objects::object_iterator_next(&iterator);
    }
}

/**
 * Deletes an object and optionally its siblings, through the second deletion path.
 *
 * Original register convention: both parameters are plain stack arguments (Ghidra's own "object_delete_4f9030(uint
 * param_1,char param_2)"). Confirmed against objdump -d -M intel bin/halo.exe: 0x4f9031 mov ebx,[esp+0x8] at entry.
 *
 * @address 0x004f9030
 */
void halo::objects::ObjectLifetime::delete_4f9030(char recurse_siblings)
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;
    int i;

    if ((obj->flags & _object_in_tracked_list_bit) != 0) {
        halo::objects::object_list_membership_set(object_index, 0);
    }

    for (i = 0; i < 3; i++) {
        object_delete_callbacks[i](object_index);
    }

    if (obj->first_child_object != k_datum_index_none) {
        halo::objects::object_delete_4f9030(obj->first_child_object, 1);
    }
    if ((recurse_siblings != 0) && (obj->next_object != k_datum_index_none)) {
        halo::objects::object_delete_4f9030(obj->next_object, 1);
    }

    if ((header->flags & _object_header_active_bit) != 0) {
        header->flags &= (uint8_t)~_object_header_active_bit;
    }

    halo::objects::widget_delete_all(object_index);
    halo::objects::object_delete_attachments(object_index);

    if ((obj->flags & _object_needs_cluster_update_bit) != 0) {
        halo::objects::object_unlink_cluster_or_notify_parent(object_index);
    }

    halo::objects::object_type_definitions_notify_0x30(object_index);
    halo::objects::object_block_data_free(object_data, object_index);
}

/**
 * Creates the attachments (lights, effects, widgets) defined by the object's tag.
 *
 * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
 * "object_create_attachments(uint param_1)").
 *
 * @address 0x004f9750
 */
void halo::objects::ObjectLifetime::create_attachments()
{
    uint32_t object_index = handle;
    object *obj = reinterpret_cast<object *>(halo::objects::object_record_bytes(object_index));
    Object *definition = halo::objects::tag_as<Object>(*(datum_index *)obj);
    int16_t i;

    for (i = 0; i < (int32_t)definition->attachments.count; i++) {
        ObjectAttachment *attachment = reinterpret_cast<ObjectAttachment *>(&halo::objects::block_element<ObjectAttachment>(definition->attachments, i));
        datum_index tag = halo::objects::tag_handle(attachment->type);
        int16_t first_scale = (int16_t)(attachment->primary_scale - 1);
        int16_t second_scale = (int16_t)((uint16_t)attachment->secondary_scale - 1);
        int16_t change_color = (int16_t)((uint16_t)attachment->change_color - 1);
        int8_t type = -1;
        datum_index handle = k_datum_index_none;

        if (tag != k_datum_index_none) {
            switch (*(uint32_t *)attachment) {
            case halo::fourcc('l', 'i', 'g', 'h'): type = 0; break;
            case halo::fourcc('l', 's', 'n', 'd'): type = 1; break;
            case halo::fourcc('e', 'f', 'f', 'e'): type = 2; break;
            case halo::fourcc('c', 'o', 'n', 't'): type = 3; break;
            case halo::fourcc('p', 'c', 't', 'l'): type = 4; break;
            }
        }
        switch (type) {
        case 0:
            handle = halo::objects::light_new_attached(tag, object_index, i, first_scale, change_color);
            if (handle != k_datum_index_none) {
                set_flag(obj->flags, objects::object_flag::has_attached_lights);
            }
            break;
        case 1:
            handle = halo::sound::looping_sound_new(object_index, tag, (char *)&attachment->marker, first_scale);
            if (handle != k_datum_index_none) {
                set_flag(obj->flags, objects::object_flag::has_attached_looping_sounds);
            }
            break;
        case 2:
            handle = halo::effects::effect_new_at_texture_coordinate(tag, object_index, change_color, first_scale, second_scale);
            break;
        case 3:
            handle = halo::effects::contrail_new(i, object_index, tag);
            break;
        case 4:
            handle = halo::effects::particle_system_new_on_marker(tag, object_index, i);
            break;
        }
        reinterpret_cast<uint8_t *>(obj)[0x144 + i] = (uint8_t)type;
        obj->attachment_handles[i] = handle;
    }
}

/**
 * Deletes the attachments of an object.
 *
 * Original register convention: object index in EBX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f9912 mov
 * eax,ebx at entry, with EBX never otherwise assigned in the function body. // blam-cc: EBX -> object_index.
 *
 * @address 0x004f9900
 */
void halo::objects::ObjectLifetime::delete_attachments()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    int16_t i;

    for (i = 0; i < (int16_t)definition->attachments.count; i++) {
        int8_t type;
        datum_index handle;

        type = obj->attachment_types[i];
        handle = obj->attachment_handles[i];
        if ((type != -1) && (handle != k_datum_index_none)) {
            switch (type) {
                case _object_attachment_type_light:
                    halo::objects::light_delete(handle);
                    break;
                case _object_attachment_type_looping_sound:
                    halo::memory::datum_delete(halo::sound::globals().game_looping_sound_data, handle);
                    break;
                case _object_attachment_type_effect:
                    halo::effects::effect_delete(handle);
                    break;
                case _object_attachment_type_contrail:
                    halo::objects::object_recalculate_bounding_radius(object_index);
                    halo::effects::contrail_advance(handle, 1, 0.0f);
                    break;
                case _object_attachment_type_particle_system: {

                    uint8_t *entry = (uint8_t *)particle_system_data->data + halo::datum_slot(handle) * 0x158;
                    ((particle_system *)entry)->flags &= ~1u;
                    *(int32_t *)&((particle_system *)entry)->object_index = -1;
                    break;
                }
            }
        }
    }
}
