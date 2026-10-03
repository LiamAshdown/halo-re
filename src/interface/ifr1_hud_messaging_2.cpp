#include "halo/interface/ifr1_hud_messaging.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "tags.h"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &hud_messaging = halo::link::ref<uint8_t *>(halo::ui::vars().hud_messaging);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
extern "C" {
extern void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind,
                                 int16_t count);
extern void player_trigger_shield_recharge_effect(uint32_t player_index);
extern void player_trigger_kill_streak_effect(uint32_t player_index);
extern void player_trigger_full_health_effect(uint32_t player_index);
}

namespace halo::interface {

/**
 * Original engine function hud_messaging_clear_after_load; the author notes are in
 * docs/original/interface/hud_messaging_clear_after_load.txt.
 *
 * @address 0x4ae530
 */
void HudMessaging::messaging_clear_after_load(void)
{
    int32_t i;

    for (i = 0; i < 4; i++) {
        hud_messaging[0x82 + i * 0x8c] = 0;
    }
}

/**
 * Client side of the HUD item message: posts the pickup text and plays the pickup feedback.
 * blam-cc: message -> EAX
 *
 * @address 0x4ae200
 */
void HudMessaging::receive_item_message(void **message)
{
    hud_item_message payload;
    data_iterator iterator;
    player *p;
    int16_t *item_tag;
    datum_index sound;

    if (*(int32_t *)*message != 0) {
        halo::networking::message_delta_decode_compound_field_staged(message);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field(message, &payload) == 0) {
        return;
    }
    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (p = (player *)halo::memory::data_iterator_next(&iterator); p != 0;
         p = (player *)halo::memory::data_iterator_next(&iterator)) {
        if (p->local_player_index != -1) {
            break;
        }
    }
    if (p == 0) {
        return;
    }

    halo::interface::hud_add_item_message(p->local_player_index, payload.item_definition, payload.kind, payload.count);
    item_tag = halo::interface::tag_data<int16_t>(payload.item_definition);
    if (item_tag[0] == 3) {
        switch (*(int16_t *)((uint8_t *)item_tag + 0x308)) {
        case 2:
            halo::game::player_trigger_shield_recharge_effect(iterator.index);
            if (halo::networking::globals().game_mode == 1) {
                object *unit = halo::objects::object_try_and_get(p->unit, 1);
                if (unit != 0) {
                    *((uint8_t *)unit + 0x106) |= 0x10;
                }
            }
            break;
        case 3:
            halo::game::player_trigger_kill_streak_effect(iterator.index);
            break;
        case 5:
            halo::game::player_trigger_full_health_effect(iterator.index);
            break;
        }
        sound = *(datum_index *)((uint8_t *)item_tag + 0x31c);
    } else if (item_tag[0] == 2 && item_tag != 0) {
        sound = halo::interface::tag_handle(((struct Weapon *)item_tag)->pickup_sound.tag_id);
    } else {
        return;
    }
    if (sound != (datum_index)-1) {
        halo::sound::sound_start_unspatialized(sound, 1.0f);
    }
}

}
