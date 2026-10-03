#include "halo/projectiles/network.hpp"
#include "halo/projectiles/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/projectiles/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"

static auto &projectile_network_update_position_tolerance = halo::link::ref<real>(halo::projectiles::vars().projectile_network_update_position_tolerance);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);
static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);
static auto &network_object_index_cache = halo::link::ref<void *>(halo::units::vars().network_object_index_cache);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);

namespace halo::projectiles {

/**
 *
 * Register convention in the original: item/projectile index in the first parameter; the
 * incoming update record pointer in the second.
 *
 * @address 0x4c1070
 */
void ProjectileNetwork::apply_update(uint32_t *update_record)
{
    datum_index projectile_index = (datum_index)handle;

    object *obj;
    projectile_data *proj;
    projectile_network_update_header *header;
    projectile_network_state decoded;

    obj = halo::objects::object_try_and_get(projectile_index, _object_mask_projectile);
    if (obj == 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)update_record);
        return;
    }
    proj = &reinterpret_cast<projectile_object *>(obj)->projectile;
    header = (projectile_network_update_header *)update_record[k_projectile_update_header_slot];

    if ((obj->flags & _object_took_network_update_bit) != 0 && *(int32_t *)update_record[0] == 1 &&
        (header->baseline_index != proj->network_baseline_index ||
         (header->sequence <= proj->network_sequence &&
          (int)((uint32_t)(header->sequence - proj->network_sequence) + 0xff) > k_projectile_network_stale_window))) {
        halo::networking::message_delta_decode_compound_field_staged((void **)update_record);
        return;
    }

    
    decoded = proj->network_state;

    {
        uint8_t accept = (*(int32_t *)update_record[0] == 1)
                             ? halo::networking::message_delta_decode_compound_field_forced((void **)update_record, &decoded, (int32_t)&proj->network_state, 0)
                             : halo::networking::message_delta_decode_compound_field((void **)update_record, &decoded);
        if (accept != 0) {
            proj->network_sequence = header->sequence;
            obj->flags |= _object_took_network_update_bit;

            if (header->is_delta != 0) {
                proj->network_baseline_index = header->baseline_index;
                proj->network_state = decoded; 
            }

            obj->velocity = decoded.velocity;
            
            obj->network_position = decoded.position;
            obj->network_velocity = decoded.velocity;
            obj->network_position_valid = 1;
            obj->network_velocity_valid = 1;

            if ((*(int32_t *)update_record[0] != 1 ||
                 projectile_network_update_position_tolerance < halo::math::vector3d_distance(obj->position, decoded.position)) &&
                (obj->flags & _object_needs_cluster_update_bit) != 0) {
                halo::objects::object_set_position_and_recalculate(&decoded.position, projectile_index);
            }

            proj->last_update_valid = 1;
            proj->last_update_state = decoded;
        }
    }
}

/**
 * Builds and sends one network delta update for a projectile: update_type == 1 sends a full
 * baseline (live position/velocity), anything else sends a delta against the stored
 * projectile_network_state. Advances the per-object sequence byte, wrapping 0xff back to 0,
 * whenever the send reports a positive result.
 *
 * Register convention in the original: matches weapon_build_network_update.c: the projectile
 * index feeds object_try_and_get's hidden index slot; the function's own four parameters are
 * already a plain __cdecl stack signature.
 *
 * @address 0x4c0f30
 */
int32_t ProjectileNetwork::build_update(uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type)
{
    uint32_t projectile_index = (uint32_t)handle;

    object *obj = halo::objects::object_try_and_get(projectile_index, _object_mask_projectile);
    int32_t result;

    if (obj == 0) {
        return 0;
    }

    {
        
        struct {
            int32_t projectile_hash;  
            uint8_t baseline_index;   
            uint8_t sequence;         
            uint8_t is_first_update;  
        } header;
        projectile_data *proj = &reinterpret_cast<projectile_object *>(obj)->projectile;
        int32_t message_type = object_type_definitions[obj->type]->network_delta_message_type; 
        void *header_ptr = &header;
        int32_t is_full_snapshot = (update_type == 1);
        void *items_array[1];
        void *type_offset;
        struct {
            real position[3];
            real velocity[3];
        } snapshot; 
                    

        header.projectile_hash = 0;
        if (projectile_index != (uint32_t)k_datum_index_none) {
            header.projectile_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, projectile_index);
            if (header.projectile_hash == -1) {
                header.projectile_hash = 0;
            }
        }
        header.sequence = proj->network_sequence;
        header.baseline_index = proj->network_baseline_index;
        header.is_first_update = (uint8_t)(update_type == 0);

        if (!is_full_snapshot) {
            items_array[0] = &proj->network_state;
            type_offset = 0;
        } else {
            void *net_ptr = &proj->network_state;

            snapshot.position[0] = obj->position.x;
            snapshot.position[1] = obj->position.y;
            snapshot.position[2] = obj->position.z;
            snapshot.velocity[0] = obj->velocity.i;
            snapshot.velocity[1] = obj->velocity.j;
            snapshot.velocity[2] = obj->velocity.k;

            items_array[0] = &snapshot;
            type_offset = &net_ptr;
        }

        result = halo::networking::message_delta_encode_message((int32_t)unused_arg2, (int32_t)unused_arg3, is_full_snapshot, message_type,
            (int)&header_ptr, items_array, (int)type_offset, 1, 0);
    }

    if (0 < result) {
        projectile_data *proj = &reinterpret_cast<projectile_object *>(obj)->projectile;
        uint8_t sequence = proj->network_sequence + 1;
        proj->network_sequence = sequence;
        if ((int8_t)sequence == -1) {
            proj->network_sequence = 0;
        }
    }
    return result;
}

/**
 * Takes a fresh network baseline for a projectile: bumps network_baseline_index, snapshots the
 * object's current position and velocity into projectile_data.network_state, marks the state
 * valid and resets the sequence counter to 0. FIXED (register inputs, objdump): the original
 * never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the
 * stack (1 stack argument(s) read). blam-cc: stack -> object_index
 *
 * @address 0x4c0ed0
 */
void ProjectileNetwork::baseline_take()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = halo::objects::object_try_and_get(object_index, _object_mask_projectile);

    if (obj != 0) {
        projectile_data *proj = &reinterpret_cast<projectile_object *>(obj)->projectile;

        proj->network_baseline_index++;
        proj->network_state.position = obj->position;
        proj->network_state_valid = 1;
        proj->network_sequence = 0;
        proj->network_state.velocity = obj->velocity;
    }
}

/**
 * Raises the projectile's state to requested_state, but only if it is not already at or past
 * it -- state never moves backward. Every caller in this module uses this instead of writing
 * projectile_data.state directly for exactly that reason.
 *
 * Register convention in the original: projectile index in EAX, requested state in CX (the low
 * 16 bits of ECX).
 *
 * @address 0x4bf0f0
 */
void ProjectileNetwork::request_state(int16_t requested_state)
{
    datum_index projectile_index = (datum_index)handle;

    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].data;
    projectile_data *pd = &reinterpret_cast<projectile_object *>(obj)->projectile;

    if (pd->state < requested_state) {
        pd->state = requested_state;
    }
}

/**
 *
 * Register convention in the original: none -- Ghidra recovered a single stack parameter, the
 * projectile index.
 *
 * @address 0x4c0b10
 */
int32_t ProjectileNetwork::send_creation()
{
    uint32_t projectile_index = (uint32_t)handle;

    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].data;
    int32_t projectile_hash = 0;
    int32_t creating_object_hash = 0;
    int32_t owner_hash = 0;
    projectile_creation_message message;
    void *message_ptr;

    if (projectile_index != halo::k_dword_none) {
        projectile_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, projectile_index);
    }
    if (obj->creator_object != halo::k_dword_none) {
        creating_object_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, obj->creator_object);
        if (creating_object_hash == -1) {
            creating_object_hash = 0;
        }
    }
    if (obj->owner_linkage != halo::k_dword_none) {
        owner_hash = halo::objects::hash_table_get(&machine_table->id_to_index, obj->owner_linkage);
        if (owner_hash == -1) {
            owner_hash = 0;
        }
    }
    if (projectile_hash == -1) {
        projectile_hash = halo::networking::network_index_cache_find_or_allocate_slot((uint8_t *)&network_object_index_cache, projectile_index); 
    }

    message.definition_tag = obj->definition_tag;
    message.object_hash = projectile_hash;
    message.owner_team = obj->owner_team;
    
    message.owner_hash = owner_hash;
    message.creating_object_hash = creating_object_hash;
    message.forward = obj->forward;
    message.up = obj->up;
    message.angular_velocity = obj->angular_velocity;
    message.baseline_index = ((projectile_object *)obj)->projectile.network_baseline_index; 
    {
        projectile_network_state *net = &((projectile_object *)obj)->projectile.network_state;
        message.position = net->position;
        message.velocity = net->velocity;
    }

    message_ptr = &message;
    return halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_projectile_creation, 0, &message_ptr, 0, 1, 0);  
}

/**
 * Receiver of network message 0x33 (k_message_projectile_attach), sent by the attach response
 * (projectile_response, this batch) when a projectile stuck to an object and both ends are
 * authoritative.
 *
 * Register convention in the original: the incoming decoded-message record pointer in EAX
 * (in_EAX), same shape as projectile_detonation_message_apply's incoming_record.
 *
 * @address 0x4bf1c0
 */
void ProjectileNetwork::attach_apply(void *incoming_record)
{
    projectile_attach_message decoded;
    datum_index projectile_handle, parent_handle;
    object *self, *parent;

    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)incoming_record);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)incoming_record, &decoded) == 0) {
        return;
    }

    projectile_handle = (datum_index)k_datum_index_none;
    if (decoded.object_hash != 0) {
        projectile_handle = object_network_id_table->handles[decoded.object_hash];
    }
    parent_handle = (datum_index)k_datum_index_none;
    if (decoded.parent_hash != 0) {
        parent_handle = object_network_id_table->handles[decoded.parent_hash];
    }

    self = halo::objects::object_try_and_get(projectile_handle, _object_mask_projectile);
    if (self == 0 || halo::objects::object_try_and_get(parent_handle, _object_mask_all) == 0) {
        return;
    }

    {
        Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[halo::datum_slot(self->definition_tag)].data;
        projectile_data *self_pd = &reinterpret_cast<projectile_object *>(self)->projectile;

        if ((tag->projectile_flags & _projectile_definition_has_super_combining_explosion_bit) != 0) {
            parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(parent_handle)].data;
            datum_index sibling_index = parent->first_child_object;
            int16_t sibling_count = 0;
            while (sibling_index != (datum_index)k_datum_index_none) {
                object *sibling = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(sibling_index)].data;
                projectile_data *sibling_pd = &reinterpret_cast<projectile_object *>(sibling)->projectile;
                if (sibling->definition_tag == self->definition_tag &&
                    (sibling_pd->flags & _projectile_super_detonation_counted_bit) == 0) {
                    sibling_pd->arming_timer = 0.0f;
                    sibling_pd->detonation_timer = 0.0f;
                    sibling_count++;
                }
                if (k_projectile_super_combine_attach_threshold < sibling_count) {
                    self_pd->flags |= _projectile_super_detonation_bit;
                    break;
                }
                sibling_index = sibling->next_object;
            }
        }

        self->velocity.i = 0.0f;
        self->velocity.j = 0.0f;
        self->velocity.k = 0.0f;
        self->angular_velocity.i = 0.0f;
        self->angular_velocity.j = 0.0f;
        self->angular_velocity.k = 0.0f;
        self_pd->flags |= _projectile_attached_bit;
        self->flags |= _object_at_rest_bit;

        halo::objects::object_attach_to_object(parent_handle, projectile_handle, decoded.parent_marker_index);

        if ((tag->projectile_flags & _projectile_definition_detonation_max_time_if_attached_bit) != 0) {
            real t = tag->timer[1];
            if (1.0f <= t * 30.0f) {
                self_pd->detonation_timer_rate = 1.0f / (t * 30.0f);
            }
        } else if ((tag->projectile_flags & _projectile_definition_random_attached_detonation_time_bit) != 0) {
            real t = halo::math::random_real_range(tag->timer[0], tag->timer[1]);
            if (1.0f <= t * 30.0f) {
                self_pd->detonation_timer_rate = 1.0f / (t * 30.0f);
            }
        }
    }
}

/**
 * Handles an incoming network projectile-creation update: decodes the message, re-
 * orthonormalizes the replicated forward/up basis, resolves the creating-object and owner
 * hashes into object handles, spawns the projectile through object_new_with_datum_role_control
 * (with the connect-to-map placement flag set), registers it in the networking hash table, and
 * then seeds its network block, position and velocity.
 *
 * Register convention in the original: the incoming record in EAX.
 *
 * @address 0x4c0ca0
 */
void ProjectileNetwork::create_from_network(void *incoming_record)
{
    projectile_creation_message decoded;
    real_vector3d forward, up, cross;
    object_placement_data placement;
    uint32_t role_material, owner_material;
    datum_index new_object_index;
    object *obj;
    projectile_data *proj;
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
    if (decoded.creating_object_hash != 0) {
        role_material = ((uint32_t *)object_network_id_table->handles)[decoded.creating_object_hash];
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
    placement.owner_team = decoded.owner_team;          
    placement.position = decoded.position;              
    placement.forward = forward;
    placement.up = up;
    placement.angular_velocity = decoded.angular_velocity;
    placement.flags |= 0x02; 
    placement.owner_linkage = owner_material;
    placement.role = role_material;

    new_object_index = halo::objects::object_new_with_datum_role_control(&placement, 1);
    if (new_object_index == (datum_index)k_datum_index_none) {
        return;
    }

    halo::networking::network_index_cache_insert_if_free((uint8_t *)&network_object_index_cache, decoded.object_hash, new_object_index);

    obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(new_object_index)].data;
    proj = &reinterpret_cast<projectile_object *>(obj)->projectile;

    proj->network_state.position = decoded.position;
    proj->network_state.velocity = decoded.velocity;
    proj->network_baseline_index = decoded.baseline_index;
    proj->network_state_valid = 1;
    proj->network_sequence = 0;

    halo::objects::object_set_position_and_recalculate(&proj->network_state.position, new_object_index);

    obj->velocity = proj->network_state.velocity;
}

/**
 * Receiver of network message 0x30 (k_message_projectile_detonation), sent only by
 * projectile_send_detonation for a thrown-grenade projectile detonating on the authority.
 * Resolves the message's object hash back to a local handle, repositions the projectile to the
 * position it detonated at on the sender, runs the normal detonation effects, requests
 * _projectile_state_disappearing (so nothing else tries to detonate it a second time) and
 * deletes it outright.
 *
 * Register convention in the original: the incoming decoded-message record pointer in EAX
 * (in_EAX), same shape as weapon_create_from_creation_message's incoming_record.
 *
 * @address 0x4bdb40
 */
void ProjectileNetwork::detonation_message_apply(void *incoming_record)
{
    projectile_detonation_message decoded;
    datum_index projectile_index;
    object *obj;

    
    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)incoming_record);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)incoming_record, &decoded) == 0 || decoded.object_hash == 0) {
        return;
    }

    projectile_index = object_network_id_table->handles[decoded.object_hash];
    if (projectile_index == (datum_index)k_datum_index_none) {
        return;
    }

    if ((((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].flags & _object_header_delete_pending_bit) == 0) {
        halo::networking::network_index_cache_remove((uint8_t *)&network_object_index_cache, projectile_index); 
    }

    obj = halo::objects::object_try_and_get(projectile_index, _object_mask_projectile);
    if (obj == 0) {
        return;
    }
    obj->network_role = 3;
    halo::objects::object_set_position_and_recalculate(&decoded.position, projectile_index);
    projectile_detonate(projectile_index, 0, 0);
    projectile_request_state(projectile_index, _projectile_state_disappearing);
    halo::objects::object_delete(projectile_index);
}

}

namespace halo::projectiles {

void projectile_apply_network_update(datum_index projectile_index, uint32_t *update_record)
{
    halo::projectiles::ProjectileNetwork(projectile_index).apply_update(update_record);
}

int32_t projectile_build_network_update(uint32_t projectile_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type)
{
    return halo::projectiles::ProjectileNetwork(projectile_index).build_update(unused_arg2, unused_arg3, update_type);
}

void projectile_network_baseline_take(uint32_t object_index)
{
    halo::projectiles::ProjectileNetwork(object_index).baseline_take();
}

void projectile_request_state(datum_index projectile_index, int16_t requested_state)
{
    halo::projectiles::ProjectileNetwork(projectile_index).request_state(requested_state);
}

int32_t projectile_send_creation(uint32_t projectile_index)
{
    return halo::projectiles::ProjectileNetwork(projectile_index).send_creation();
}

void projectile_attach_apply(void *incoming_record)
{
    halo::projectiles::ProjectileNetwork::attach_apply(incoming_record);
}

void projectile_create_from_network(void *incoming_record)
{
    halo::projectiles::ProjectileNetwork::create_from_network(incoming_record);
}

void projectile_detonation_message_apply(void *incoming_record)
{
    halo::projectiles::ProjectileNetwork::detonation_message_apply(incoming_record);
}

}
