#include "halo/projectiles/network.hpp"
#include "halo/projectiles/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"

static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::projectiles::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &network_object_index_cache = halo::link::ref<void *>(halo::units::vars().network_object_index_cache);

namespace halo::projectiles {

/**
 * Broadcasts a projectile-attach network event: the projectile's own hash, the hash of the
 * object it just stuck to, and the marker it attached at. Sent by the attach response
 * (projectile_response, this batch) when both ends are authoritative.
 *
 * Register convention in the original: projectile index in ECX, parent object index in EDI
 * (both confirmed by `cmp ecx,0xffffffff` / `cmp edi,0xffffffff` immediately guarding each
 * hash lookup); the marker index is Ghidra's own recognized stack parameter (`param_1`, used
 * only for its low 16 bits).
 *
 * @address 0x4bf120
 */
void ProjectileNetwork::send_attach(datum_index parent_object_index, int16_t marker_index)
{
    datum_index projectile_index = (datum_index)handle;

    projectile_attach_message message;
    void *items[1];

    message.object_hash = 0;
    if (projectile_index != (datum_index)k_datum_index_none) {
        message.object_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, projectile_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.parent_hash = 0;
    if (parent_object_index != (datum_index)k_datum_index_none) {
        message.parent_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, parent_object_index);
        if (message.parent_hash == -1) {
            message.parent_hash = 0;
        }
    }
    message.parent_marker_index = marker_index;

    items[0] = &message;
    halo::networking::network_session_broadcast_to_flagged(halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::projectiles::k_network_message_scratch_size, 0, k_message_projectile_attach, 0, items, 0, 1, 0), halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
}

/**
 * Broadcasts a projectile-detonation network event (thrown-grenade projectiles only; see
 * projectile_update's thrown_grenade check) with the projectile's own hash and its current
 * position, forces the object into network_role 3 (the "waiting to be deleted by the network"
 * role the receiver 0x4bdb40 also uses), and, unless the object is already pending delete,
 * notifies the pooled-node globals of the role change.
 *
 * @address 0x4bda60
 */
void ProjectileNetwork::send_detonation()
{
    datum_index projectile_index = (datum_index)handle;

    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].data;
    projectile_detonation_message message;
    void *items[1];

    message.object_hash = 0;
    if (projectile_index != (datum_index)k_datum_index_none) {
        message.object_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, projectile_index);
    }
    message.position = obj->position;

    items[0] = &message;
    halo::networking::network_session_broadcast_to_flagged(halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::projectiles::k_network_message_scratch_size, 0, k_message_projectile_detonation, 0, items, 0, 1, 0), halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);

    obj->network_role = 3;
    if ((((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].flags & _object_header_delete_pending_bit) == 0) {
        halo::networking::network_index_cache_remove((uint8_t *)&network_object_index_cache, projectile_index); 
    }
}

}

namespace halo::projectiles {

void projectile_send_attach(datum_index projectile_index, datum_index parent_object_index, int16_t marker_index)
{
    halo::projectiles::ProjectileNetwork(projectile_index).send_attach(parent_object_index, marker_index);
}

void projectile_send_detonation(datum_index projectile_index)
{
    halo::projectiles::ProjectileNetwork(projectile_index).send_detonation();
}

}
