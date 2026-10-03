/**
 * Network message handlers and gameplay event notifications of the game engine.
 */

#include "tags.h"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/core/datum.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

#include "halo/game/game1_notifications.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"

static auto &shared_hud_text_draw_state = halo::link::ref<uint8_t>(halo::game::vars().shared_hud_text_draw_state);
static auto &machine_table = halo::link::ref<uint8_t *>(halo::game::vars().machine_table);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);

namespace halo::game::engine1 {

/**
 * Sends an object value event to the machines that need it.
 *
 * Original register convention: EAX -> value_byte, ECX -> hash_key, EDI -> machine_index, stack -> subject.
 *
 * @address 0x4779d0
 */
void Notifications::notify_object_value_event(uint8_t value_byte, int32_t hash_key, int32_t machine_index, void *subject)
{
    struct { uint8_t value_byte; int32_t hash_result; void *subject; } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.value_byte = value_byte;
    fields.hash_result = 0;
    if (hash_key != -1) {
        fields.hash_result = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)hash_key);
        if (fields.hash_result == -1) {
            fields.hash_result = 0;
        }
    }
    fields.subject = subject;
    fields_ptr = &fields;

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::object_value_event), 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        if (machine_index == -1) {
            halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
            return;
        }
        halo::networking::network_session_send_to_machine(machine_index, halo::networking::globals().server, 1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
    }
}

/**
 * Notifies the machines of a player interaction (for example entering a seat).
 *
 * Original register convention: ECX -> primary_key, EDI -> edi_key, stack -> mode, interaction_type_and_seat,
 * secondary_key.
 *
 * @address 0x478ff0
 */
void Notifications::notify_player_interaction(uint32_t primary_key, uint32_t edi_key, uint32_t mode, int32_t interaction_type, int32_t interaction_seat, int32_t secondary_key)
{
    struct {
        int32_t primary_hash;
        int32_t mode;
        int32_t edi_hash;
        int16_t interaction_type;
        int16_t low_secondary_key;
        int32_t secondary_hash;
    } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.primary_hash = 0;
    if (primary_key != halo::k_dword_none) {
        fields.primary_hash = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)primary_key);
        if (fields.primary_hash == -1) {
            fields.primary_hash = 0;
        }
    }
    fields.mode = mode;
    fields.edi_hash = 0;
    if (edi_key != halo::k_dword_none) {
        fields.edi_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)edi_key);
        if (fields.edi_hash == -1) {
            fields.edi_hash = 0;
        }
    }
    fields.interaction_type = (int16_t)interaction_type;
    fields.low_secondary_key = (int16_t)interaction_seat;
    fields.secondary_hash = 0;
    if (secondary_key != -1) {
        fields.secondary_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)secondary_key);
        if (fields.secondary_hash == -1) {
            fields.secondary_hash = 0;
        }
    }
    fields_ptr = &fields;

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::player_interaction), 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

}
