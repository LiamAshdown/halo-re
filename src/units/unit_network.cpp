#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "networking.h"
#include "halo/memory/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"

static auto &object_network_id_table = halo::link::ref<uint8_t *>(halo::units::vars().object_network_id_table);
static auto &network_object_index_cache = halo::link::ref<uint8_t []>(halo::units::vars().network_object_index_cache);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);

namespace halo::units {

namespace unit_apply_network_control_update_local {

typedef struct unit_network_control_message {
    int32_t unit_key;
    uint8_t update_stance;
    uint8_t stance_flags[5];
    uint8_t no_throttle;
    uint8_t pad_0b;
    int16_t weapon_class_index;
    int16_t pad_0e;
    float turn_angle;
    real_vector2d throttle;
    uint32_t player_2c;
    uint32_t pad_20;
} unit_network_control_message;

}

/**
 * Engine function unit_apply_network_control_update.
 *
 * Original register convention: EAX -> packet (decode context).
 *
 * @address 0x566c90
 */
void halo::units::unit_apply_network_control_update(unit_network_control_packet *packet)
{
    using namespace unit_apply_network_control_update_local;
    unit_network_control_message message;
    const real_vector2d *throttle;
    uint32_t unit_index;
    uint8_t *unit;

    if (*packet->kind_ptr != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)packet);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)packet, &message) == 0 || message.unit_key == 0) {
        return;
    }
    unit_index = (uint32_t)(*(int32_t **)(object_network_id_table + 0x28))[message.unit_key];
    if (unit_index == k_datum_index_none) {
        return;
    }
    throttle = message.no_throttle == 1 ? (const real_vector2d *)0 : &message.throttle;
    unit = (uint8_t *)halo::objects::object_try_and_get(unit_index, 3);
    if (unit != 0) {
        set_flag(((struct object *)unit)->vitality_flags, objects::vitality_flag::health_frozen);
        ((unit_object *)unit)->base.body_vitality = 0.0f;
        ((unit_object *)unit)->base.shield_vitality = 0.0f;
    }
    if (message.update_stance == 1) {
        UnitView(unit_index).update_stance_and_jump(message.stance_flags[0], message.stance_flags[1], message.stance_flags[2], message.stance_flags[3], message.stance_flags[4], message.turn_angle, message.weapon_class_index, throttle, 1);
    }
    unit = (uint8_t *)halo::objects::object_try_and_get(unit_index, 3);
    if (unit != 0 && ((unit_object *)unit)->unit.controlling_player != k_datum_index_none) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(((unit_object *)unit)->unit.controlling_player, halo::game::globals().player_data);

        if (player != 0) {
            *(uint32_t *)&((struct player *)player)->respawn_timer = message.player_2c;
        }
    }
    UnitView(unit_index).release_transient_state_and_detach(message.stance_flags[1]);
    unit = (uint8_t *)halo::objects::object_try_and_get(unit_index, 3);
    if (unit != 0) {
        ((unit_object *)unit)->base.network_role = 3;
    }
    if ((((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].flags & 8) == 0) {
        halo::networking::network_index_cache_remove(network_object_index_cache, (int32_t)unit_index);
    }
}

namespace unit_apply_network_health_update_local {

typedef struct biped_network_health_block {
    uint32_t grenade_counts;
    uint32_t body_vitality;
    real shield_vitality;
    uint32_t shield_stunned;
} biped_network_health_block;

}

/**
 * Engine function unit_apply_network_health_update.
 *
 * @address 0x55b5f0
 */
void UnitView::apply_network_health_update(void *message)
{
    using namespace unit_apply_network_health_update_local;
    uint32_t object_index = datum_handle;
    uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get((datum_index)object_index, 1);
    uint8_t *guard;
    uint8_t *record;
    int32_t reliable;
    biped_network_health_block block;
    uint8_t accepted;
    real shield;

    if (unit == 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)message);
        return;
    }
    guard = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    record = (uint8_t *)((void **)message)[0x11];
    reliable = **(int32_t **)message == 1;
    if (test_flag(((struct object *)guard)->flags, objects::object_flag::took_network_update) && reliable) {
        int32_t incoming = record[5];
        int32_t current = unit[0x528];

        if (record[4] != unit[0x527] || (incoming <= current && incoming - current + 0xff >= 0x1e)) {
            halo::networking::message_delta_decode_compound_field_staged((void **)message);
            return;
        }
    }
    memcpy(&block, unit + 0x52c, sizeof(block));
    if (reliable) {
        accepted = halo::networking::message_delta_decode_compound_field_forced((void **)message, &block, (int32_t)(unit + 0x52c), 0);
    } else {
        accepted = halo::networking::message_delta_decode_compound_field((void **)message, &block);
    }
    if (!accepted) {
        return;
    }
    unit[0x528] = record[5];
    set_flag(((unit_object *)unit)->base.flags, objects::object_flag::took_network_update);
    if (record[6] != 0) {
        unit[0x527] = record[4];
        memcpy(unit + 0x52c, &block, sizeof(block));
    }
    shield = block.shield_vitality * 3.0f;
    *(int16_t *)(unit + 0x31e) = (int16_t)block.grenade_counts;
    *(uint32_t *)&((unit_object *)unit)->base.body_vitality = block.body_vitality;
    if (record[7] == 1) {
        ((unit_object *)unit)->base.shield_vitality = shield;
    }
    *(uint32_t *)(unit + 0x540) = block.grenade_counts;
    *(uint32_t *)(unit + 0x544) = block.body_vitality;
    *(real *)(unit + 0x548) = shield;
    *(uint32_t *)(unit + 0x54c) = block.shield_stunned;
    ((unit_object *)unit)->base.shield_stun_ticks = (uint8_t)block.shield_stunned == 1;
    ((struct unit_object *)unit)->unit.network_update_applied = 1;
    unit[0x53c] = 1;
}

/**
 * Engine function unit_broadcast_state_change_event.
 *
 * @address 0x566c00
 */
void halo::units::unit_broadcast_state_change_event(unit_state_change_record record)
{
    int32_t resolved = 0;
    void *items[2];
    int32_t sent;

    if (record.unit != k_datum_index_none) {
        resolved = halo::objects::hash_table_get((hash_table *)(object_network_id_table + 0xc), (int32_t)record.unit);
        if (resolved == -1) {
            resolved = 0;
        }
    }
    record.unit = (datum_index)resolved;
    items[0] = &record;
    items[1] = 0;
    sent = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0xc, 0, items, 0, 1, 0);
    if (sent > 0) {
        halo::networking::network_session_broadcast_to_flagged(sent, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
    }
}

}
