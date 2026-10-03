#pragma once

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "cutscene.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original HudMessaging functions.
 */
class HudMessaging {
public:
    static void hud_message(int16_t local_player_index, const wchar_t *text);
    static void multiplayer_message(const wchar_t *text);
    static void add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind, int16_t count);
    static void display_checkpoint_message(uint8_t is_begin);
    static void display_loading_message(uint8_t is_begin);
    static uint16_t * get_message_string(int32_t message_index);
    static void message_broadcast_to_local_players(const uint16_t *text);
    static int32_t message_compare(const void *a, const void *b);
    static hud_message_slot * message_find_slot(int32_t source, hud_player_messaging_state *record, uint8_t source_kind);
    static void messaging_clear_after_load(void);
    static void messaging_update(int16_t local_player_index);
    static void play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code);
    static void post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index, int8_t machine_id);
    static void receive_item_message(void **message);
    static void set_action_text_shown(int16_t local_player_index, uint8_t shown);
    static void set_help_text(int16_t message_index);
    static void set_message_icon_argument(int16_t local_player_index, int16_t slot, const hud_messaging_information *information);
    static void set_message_string_argument(int16_t local_player_index, int16_t slot, int16_t string_index, uint8_t from_scenario_names);
    static void set_objective_text(int16_t message_index);
    static void set_player_message(int16_t message_index, int16_t local_player_index);
};

}
