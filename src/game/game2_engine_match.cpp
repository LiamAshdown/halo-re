#include "halo/game/game2_engine_match.hpp"
#include "halo/core/cstring.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/game/variant_flags.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/memory/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/networking/channel_queue.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/ai/api.hpp"

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &game_engine_end_game_timer = halo::link::ref<float>(halo::game::vars().game_engine_end_game_timer);
static auto &game_engine_unknown_aa00 = halo::link::ref<uint32_t>(halo::game::vars().game_engine_unknown_aa00);
static auto &sv_tk_cooldown_ticks = halo::link::ref<int32_t>(halo::game::vars().sv_tk_cooldown_ticks);
static auto &k_empty_string = halo::link::ref<char []>(halo::networking::vars().k_empty_string);
static auto &player_profile_cache_initialized = halo::link::ref<uint8_t>(halo::game::vars().player_profile_cache_initialized);
static auto &player_profile_cache = halo::link::ref<player_profile [16]>(halo::game::vars().player_profile_cache);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &shared_hud_text_draw_state = halo::link::ref<uint8_t>(halo::game::vars().shared_hud_text_draw_state);
static auto &game_engine_teams_enabled_flag = halo::link::ref<uint8_t>(halo::game::vars().game_engine_teams_enabled_flag);
static auto &network_client = halo::link::ref<uint8_t []>(halo::networking::vars().network_client);

namespace halo::game {

/**
 * 0x46042e..0x4604f8 (type 0x0d) and 0x4606a4..0x460769 (type 8), and the same body inside the two player-
 * iterator loops at 0x4604f9 / 0x460770 (reached only when `target` is -1, which the category tests above make
 * impossible): validates the target handle, builds the localized text for a locally driven player and, in a
 * network game, always tells the target's machine.
 */
void EngineMatch::send_message(datum_index target, uint32_t message_type, datum_index victim, wchar_t *buffer)
{
    int16_t index = (int16_t)target;
    int16_t salt = (int16_t)((uint32_t)target >> 16);
    player *t;

    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    t = halo::game::player_at(index);
    if (t->identifier == 0 || (salt != 0 && t->identifier != salt)) {
        return;
    }
    if (t->local_player_index != -1) {
        if ((current_game_engine->build_message_text != 0 &&
             ((char (*)(datum_index, uint32_t, datum_index, wchar_t *, uint32_t))current_game_engine->build_message_text)(
                 target, message_type, victim, buffer, 0x400) != 0) ||
            halo::game::game_engine_build_kill_feed_message_text(target, buffer, message_type, victim, 0x400) != 0) {
            buffer[0x3ff] = 0;
            halo::interface::chimera__multiplayer_message(buffer);
        }
    }
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_notify_kill_event(target, target, message_type, victim);
    }
}

/**
 * 0x4604f9 / 0x460770: with a valid `killer` only that player is messaged; -1 walks every player.
 */
void EngineMatch::message_players(datum_index killer, uint32_t message_type, datum_index victim, wchar_t *buffer)
{
    if (killer != (datum_index)halo::k_dword_none) {
        send_message(killer, message_type, victim, buffer);
    } else {
        data_iterator iter;
        void *element;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)halo::k_dword_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = halo::memory::data_iterator_next(&iter);
        while (element != 0) {
            send_message(iter.index, message_type, victim, buffer);
            element = halo::memory::data_iterator_next(&iter);
        }
    }
}

/**
 * Schedules `victim`'s respawn timer (base + accumulated growth, plus suicide/betrayal penalties, clamped to
 * [0x5a, 9000] ticks), refunds the killer's own accumulated growth on a clean kill, then, unless the victim is
 * already marked for deletion, classifies the death into one of five kill-feed message categories and
 * broadcasts the resulting text.
 *
 * @address 0x460200
 */
void EngineMatch::on_player_death(datum_index killer, datum_index death_object, datum_index victim, char is_suicide)
{
    player *v;
    uint8_t message_category;
    wchar_t kill_feed_buffer[1024];

    if (current_game_engine == 0) {
        return;
    }

    v = halo::game::player_at(victim);
    v->last_death_tick = game_time->game_time;

    if (current_game_engine->on_player_death != 0) {
        ((void (*)(datum_index, datum_index, datum_index, char))current_game_engine->on_player_death)(killer, death_object,
            victim, is_suicide);
    }

    {
        uint8_t clean_kill = killer != (datum_index)halo::k_dword_none && victim != (datum_index)halo::k_dword_none;
        clean_kill = (is_suicide == 0 && clean_kill && killer != victim);

        v->respawn_timer = v->respawn_time_growth + game_engine_variant.respawn_time;

        if (0 < game_engine_variant.respawn_time_growth) {
            int32_t grown = v->respawn_time_growth + game_engine_variant.respawn_time_growth;
            int32_t cap = game_engine_variant.respawn_time_growth * 5;
            v->respawn_time_growth = (grown <= cap) ? grown : cap;

            if (clean_kill && killer != (datum_index)halo::k_dword_none) {
                player *k = halo::game::player_at(killer);
                int32_t refunded = k->respawn_time_growth - game_engine_variant.respawn_time_growth;
                k->respawn_time_growth = (refunded < 1) ? 0 : refunded;
            }
        }
    }

    if (killer == victim) {
        v->respawn_timer = v->respawn_timer + game_engine_variant.suicide_penalty;
    }
    if (v->betrayal_penalty_count != 0) {
        v->respawn_timer = v->respawn_timer + v->betrayal_penalty_count * game_engine_variant.betrayal_penalty;
        v->betrayal_penalty_count = 0;
    }
    if (v->respawn_timer < 0x5a) {
        v->respawn_timer = 0x5a;
    }
    if (9000 < v->respawn_timer) {
        v->respawn_timer = 9000;
    }

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_player_profile_cache_sync_all(1, (void *)halo::k_dword_none);
    }

    if (v->marked_for_deletion != 0) {
        return;
    }

    if (killer != (datum_index)halo::k_dword_none) {
        if (killer == victim) {
            halo::game::game_engine_broadcast_kill_feed_gated(6, -1, victim, killer, 1);
            return;
        }
        message_category = (is_suicide != 0) + 4;
    } else if (death_object != (datum_index)halo::k_dword_none) {
        object *obj = halo::game::object_at(death_object);
        if (obj->type == 0) {
            message_category = 2;
        } else if (obj->type == 1) {
            message_category = 3;
        } else {
            message_category = 1;
        }
    } else {
        message_category = 1;
    }
    halo::game::game_engine_broadcast_kill_feed_gated(message_category, killer, victim, killer, 1);

    if (message_category == 5) {
        message_players(killer, 0x0d, victim, kill_feed_buffer);
    } else if (message_category == 4) {
        player *k = halo::game::player_at(killer);
        int32_t spree_type = 0;
        int32_t send_spree = 1;

        if (k->multikill_count >= 4) {
            spree_type = 0x0a;
        } else if (k->multikill_count == 3) {
            spree_type = 9;
        } else if (k->multikill_count == 2) {
            spree_type = 7;
        } else if (k->killing_spree_count == 5) {
            spree_type = 0x0b;
        } else if (k->killing_spree_count % 5 == 0) {
            spree_type = 0x0c;
        } else {
            send_spree = 0;
        }
        if (send_spree) {
            halo::game::game_engine_broadcast_kill_feed_or_direct(killer, spree_type, 1, killer, victim);
        }

        message_players(killer, 8, victim, kill_feed_buffer);
    }
}

/**
 * The multiplayer game engine's per-tick update: advances the announcer queue, item cleanup, netgame-equipment
 * respawns and (while hosting) each player's kill-streak/teleporter/per-player engine hook, then drives the
 * end-of-game state machine's two timed stages.
 *
 * @address 0x45ff30
 */
void EngineMatch::tick(void)
{
    if (current_game_engine == 0) {
        return;
    }

    halo::game::game_engine_multiplayer_sound_queue_tick();
    halo::game::game_engine_cleanup_dropped_objects();
    halo::game::game_engine_update_item_scale_and_pickup();

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host || halo::networking::globals().game_mode == halo::networking::k_game_mode_local) {
        halo::game::game_engine_update_netgame_equipment(0);
    }
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_player_profile_cache_sync_all(1, (void *)halo::k_dword_none);
    }

    {
        data_iterator player_iter;
        void *player_element;

        player_iter.data = player_data;
        player_iter.next_index = 0;
        player_iter.index = (datum_index)halo::k_dword_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_element = halo::memory::data_iterator_next(&player_iter);

        while (player_element != 0) {
            halo::game::game_engine_clear_unit_shields_when_disabled(player_iter.index);

            if (current_game_engine != 0 &&
                (halo::game::variant_flag_set(game_engine_variant.flags, halo::game::game_variant_flags::invisible_players) ||
                 (current_game_engine->time_scale_override != 0 &&
                  ((char (*)(datum_index, int32_t))current_game_engine->time_scale_override)(
                      player_iter.index, 1) != 0)) &&
                ((player *)player_element)->unit != k_datum_index_none) {
                halo::game::player_kill_streak_set_max(0, player_iter.index, 0xf);
            }

            halo::game::game_engine_update_teleporter(player_iter.index);
            if (current_game_engine->update != 0) {
                ((void (*)(datum_index))current_game_engine->update)(player_iter.index);
            }

            {
                datum_index handle = player_iter.index;
                int16_t index = (int16_t)handle;
                int16_t salt = (int16_t)((uint32_t)handle >> 16);

                if (handle != (datum_index)halo::k_dword_none && index >= 0 && index < player_data->maximum_count) {
                    player *q = halo::game::player_at(index);

                    if (q->identifier != 0 && (salt == 0 || q->identifier == salt) && q->medal_streak_count != 0) {
                        int32_t timer = q->medal_streak_timer;

                        if (timer < 0) {
                            q->medal_streak_timer = timer + 1;
                            if (timer + 1 >= 0) {
                                q->medal_streak_timer = sv_tk_cooldown_ticks;
                            }
                        } else if (timer > 0) {
                            q->medal_streak_timer = timer - 1;
                            if (timer - 1 <= 0) {
                                int32_t count = q->medal_streak_count - 1;

                                q->medal_streak_count = count;
                                if (count < 0) {
                                    q->medal_streak_count = 0;
                                }
                                q->medal_streak_timer = (q->medal_streak_count != 0) ? sv_tk_cooldown_ticks : 0;
                            }
                        }
                    }
                }
            }

            player_element = halo::memory::data_iterator_next(&player_iter);
        }
    }

    if (current_game_engine->get_score != 0) {
        ((void (*)(void))current_game_engine->get_score)();
    }

    if (game_engine_state_value == _game_engine_state_not_started) {
        if (halo::game::game_engine_announce_time_remaining() != 0) {
            halo::game::game_engine_begin_end_game_sequence();
        }
    } else if (game_engine_state_value == _game_engine_state_ending) {
        if (game_engine_end_game_timer <= 2.0f && (game_engine_unknown_aa00 & 0x10) == 0) {
            halo::sound::sound_class_set_gain_by_name(k_empty_string, 0.0f, 0x1e);
            halo::sound::sound_class_set_gain_by_name(halo::mutable_literal("ambient_nature"), 0.2f, 0x1e);
            halo::sound::sound_class_set_gain_by_name(halo::mutable_literal("ambient_machinery"), 0.2f, 0x1e);
            halo::sound::sound_class_set_gain_by_name(halo::mutable_literal("ambient_computers"), 0.2f, 0x1e);
            game_engine_unknown_aa00 = game_engine_unknown_aa00 | 0x10;
        }

        game_engine_end_game_timer = game_engine_end_game_timer - 0.033333335f;
        if (game_engine_end_game_timer <= 0.0f && halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
            halo::game::game_engine_end_game_sequence_stage2();
            halo::game::game_engine_send_end_game_notification(2);
            halo::networking::network_server_advance_connect_state(halo::networking::globals().server);
        }
    }
}

/**
 * Disposes of the currently loaded game engine (via its vtable's dispose entry) and, if it had been populated,
 * zeroes the 16-entry player profile cache.
 *
 * @address 0x45c330
 */
void __cdecl EngineMatch::unload(void)
{
    uint32_t *cache_words;
    int32_t i;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }
    if (player_profile_cache_initialized == 1) {
        cache_words = (uint32_t *)player_profile_cache;
        for (i = 0xc0; i != 0; i = i - 1) {
            *cache_words = 0;
            cache_words = cache_words + 1;
        }
        player_profile_cache_initialized = 0;
    }
}

/**
 * Encodes and broadcasts a network message of type 0x16 (an end-of-game notification, per this function's
 * callers) carrying `reason`.
 *
 * @address 0x4671d0
 */
void EngineMatch::send_end_game_notification(uint32_t reason)
{
    uint32_t payload;
    void *payload_ptr;
    int32_t encoded_size;

    payload = reason;
    payload_ptr = &payload;

    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::end_game), 0, &payload_ptr, 0, 1, 0);
    if (encoded_size > 0) {
        halo::networking::network_session_broadcast_to_flagged(encoded_size, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
    }
}

/**
 * Encodes a one-byte (value 1) network message of type 0x17 and, if the encoder reports it was actually
 * written (return > 0), broadcasts it via network_session_broadcast_to_flagged.
 *
 * @address 0x4682c0
 */
void EngineMatch::send_round_reset_message(void)
{
    uint8_t payload_value = 1;
    uint8_t *payload = &payload_value;
    int32_t encoded_bits;

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::round_reset), 0, (void **)&payload, 0, 1, 0);
    if (encoded_bits > 0) {
        halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

/**
 * While a multiplayer engine is loaded and teams are enabled, encodes network event 0x1a with the first local
 * player's team_index_desired byte (0xff if there is no local player) and `broadcast`, then, if there is room
 * in the outgoing packet (or the session can be flushed to make room), commits it and writes it out in two
 * chunked bit-stream calls.
 *
 * @address 0x4704d0
 */
void EngineMatch::send_team_allegiance_message(char broadcast)
{
    data_iterator player_iter;
    void *player_element;
    uint8_t team_index_desired = 0xff;
    struct {
        uint8_t team;
        uint8_t broadcast;
    } record;
    void *fields_ptr[2];
    int32_t encoded_bits;

    if (current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        return;
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = halo::memory::data_iterator_next(&player_iter);
    while (player_element != 0) {
        if (((player *)player_element)->local_player_index != -1) {
            team_index_desired = (uint8_t)((player *)player_element)->team_index_desired;
            break;
        }
        player_element = halo::memory::data_iterator_next(&player_iter);
    }

    record.broadcast = (uint8_t)broadcast;
    record.team = team_index_desired;
    fields_ptr[0] = &record;
    fields_ptr[1] = 0;

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::team_allegiance), 0, fields_ptr, 0, 1, 0);
    if (encoded_bits > 0) {
        network_channel *channel = ((network_client_globals *)network_client)->channel;

        if ((channel->flags & 1) == 0) {
            halo::networking::channel_queue_bits(channel, reinterpret_cast<const uint32_t *>(network_message_scratch), encoded_bits, 1);
        }
    }
}

/**
 * For `side` 0 or 1, gathers the team totals; if `side` is ahead of the other side in the first array it
 * returns 1, otherwise 1 when the other side's margin over `side` in the second array is at least 20% of the
 * other side's own value, else 0. Any other `side` returns 0.
 *
 * @address 0x470790
 */
uint8_t EngineMatch::team_close_game_check(int32_t side, int32_t filter_value)
{
    uint32_t out_count[2];
    uint32_t out_score[2];
    int32_t other_side = 1 - side;

    if (side != 0 && side != 1) {
        return 0;
    }

    halo::game::game_engine_gather_team_score_totals(out_count, out_score, filter_value);

    if ((int32_t)out_count[side] > (int32_t)out_count[other_side]) {
        int32_t margin = (int32_t)out_score[other_side] - (int32_t)out_score[side];
        double allowed_margin = (double)(int32_t)out_score[other_side] * (double)0.2f;

        if (allowed_margin > (double)margin) {
            return 0;
        }
    }
    return 1;
}

/**
 * Returns false if no multiplayer engine with teams is loaded. Otherwise gathers the two team score/count
 * totals; if the two sides' match counts differ, the side with more matches is "leading". If the counts are
 * tied, refreshes both team scores via get_team_score(0)/(1) (return values discarded, matching Ghidra) and
 * falls back to comparing the two side's scores.
 *
 * @address 0x470720
 */
uint8_t EngineMatch::team_is_leading(int32_t filter_value)
{
    uint32_t out_count[2];
    uint32_t out_score[2];

    if (current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        return 0;
    }

    halo::game::game_engine_gather_team_score_totals(out_count, out_score, filter_value);
    if (out_count[0] != out_count[1]) {
        return out_count[1] <= out_count[0];
    }

    ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(0);
    ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(1);
    return out_score[1] < out_score[0];
}

}
