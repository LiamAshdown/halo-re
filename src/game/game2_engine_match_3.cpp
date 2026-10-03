#include "halo/game/game2_engine_match.hpp"

extern "C" {
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag;
extern uint8_t *network_client;
extern data_array *player_data;
extern uint8_t game_engine_unknown_1cfc;
extern uint8_t network_message_scratch[0x7ff8];
extern void game_engine_player_round_reset(void);
extern void *data_iterator_next(data_iterator *iterator);
extern void chat_queue_team_message(int32_t color, int32_t message_id);
extern uint8_t message_delta_decode_compound_field(void *event, void *out_values);
extern void message_delta_decode_compound_field_staged(void *event);
extern uint8_t game_engine_team_close_game_check(int32_t side, int32_t filter_value);
extern uint8_t game_engine_team_is_leading(int32_t filter_value);
extern uint8_t player_customization_slot_set(uint8_t *base, uint8_t new_value, uint32_t key);
extern void player_set_team_by_color(uint8_t new_team, int8_t target_team_index_desired);
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data, int32_t param_3, int32_t param_4, int32_t force, int32_t param_6);
}

namespace halo::game {

/**
 * Detects a lead change or close-game condition for a given team color, updates the tracked state and HUD
 * indicator, and broadcasts the notification to all clients.
 *
 * @address 0x470810
 */
void EngineMatch::update_lead_change_state(void **envelope, uint8_t *message)
{
    uint8_t out_pair[2] = { 0xff, 0xff };
    uint8_t color, side_selector;

    if (*(int32_t *)*envelope != 0 || current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }

    if (!message_delta_decode_compound_field(envelope, out_pair)) {
        return;
    }
    color = out_pair[0];
    side_selector = out_pair[1];

    if (color > 0xf ||
        (int16_t)*(int8_t *)((uint8_t *)network_server + (uint32_t)color * 0x20 + 0x1c6) != *(int16_t *)(message + 0xc)) {
        chat_queue_team_message(color, 0x91);
        return;
    }

    {
        int32_t leading_or_side;

        if (side_selector == 0 || side_selector == 1) {
            if (game_engine_unknown_1cfc != 0 && !game_engine_team_close_game_check(color, 0)) {
                chat_queue_team_message(color, 0x91);
                return;
            }
            leading_or_side = side_selector;
        } else {
            leading_or_side = game_engine_team_is_leading(color) & 0xff;
        }

        if (leading_or_side == -1) {
            return;
        }

        if (!player_customization_slot_set(network_client + 8, (uint8_t)leading_or_side, (uint8_t)color)) {
            return;
        }

        if ((network_client[6] >> 2 & 1) == 0) {
            player_customization_slot_set(network_client + 0xb14, (uint8_t)leading_or_side, (uint8_t)color);
        }
        player_set_team_by_color((uint8_t)leading_or_side, (uint8_t)color);

        {
            data_iterator player_iter;
            void *player_element;

            player_iter.data = player_data;
            player_iter.next_index = 0;
            player_iter.index = k_datum_index_none;
            player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
            player_element = data_iterator_next(&player_iter);
            while (player_element != 0) {
                if (((player *)player_element)->team_index_desired == (int8_t)color) {
                    game_engine_player_round_reset();
                    break;
                }
                player_element = data_iterator_next(&player_iter);
            }
        }

        {
            uint8_t local_team_byte = color;
            uint8_t *fields_ptr = &local_team_byte;

            message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x1a, 0, (void **)&fields_ptr, 0, 1, 0);
        }
        network_session_broadcast_to_flagged(network_server, 1, network_message_scratch, 1, 0, 1, 3);
    }
}

}
