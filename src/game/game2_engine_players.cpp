#include "halo/units/animation_states.hpp"
#include "halo/game/game2_engine_players.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/game/variant_flags.hpp"
#include "halo/game/constants.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_block.hpp"
#include "models.h"
#include "halo/objects/record_access.hpp"
#include "halo/core/lcg.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/ai/api.hpp"

static const int8_t k_unit_exit_seat_request[2] = {0x14, 0};

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &game_engine_auto_team_counter = halo::link::ref<int32_t>(halo::game::vars().game_engine_auto_team_counter);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &player_control_globals_ptr = halo::link::ref<player_control_globals *>(halo::game::vars().player_control_globals_ptr);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &look_pitch_rate_setting = halo::link::ref<real [k_maximum_local_players]>(halo::game::vars().look_pitch_rate_setting);
static auto &look_yaw_rate_setting = halo::link::ref<real [k_maximum_local_players]>(halo::game::vars().look_yaw_rate_setting);
static auto &player_profile_cache = halo::link::ref<player_profile [16]>(halo::game::vars().player_profile_cache);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &game_engine_recent_location_count = halo::link::ref<int16_t>(halo::game::vars().game_engine_recent_location_count);
static auto &game_engine_recent_location_table = halo::link::ref<int16_t []>(halo::game::vars().game_engine_recent_location_table);

namespace halo::game {

/**
 * Posts the "player changed object" (type 0x1c) message about `param` to every local player's chat line, then
 * forwards `param` to the active game engine's player_changed_object slot.
 *
 * @address 0x45c570
 */
void EnginePlayers::player_changed_object(uint32_t param)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        datum_index recipient = iterator.index;
        int16_t index = (int16_t)recipient;

        if (recipient != k_datum_index_none && index >= 0 && index < player_data->maximum_count) {
            player *r = halo::game::player_at(index);
            int16_t salt = (int16_t)((uint32_t)recipient >> 16);

            if (r->identifier != 0 && (salt == 0 || r->identifier == salt) &&
                r->local_player_index != -1) {
                wchar_t message[0x400];
                char built = 0;

                if (current_game_engine->build_message_text != 0) {
                    built = ((char (*)(datum_index, uint32_t, uint32_t, wchar_t *, size_t))
                        current_game_engine->build_message_text)
                        (recipient, 0x1c, param, message, 0x400);
                }
                if (built == 0) {
                    built = halo::game::game_engine_build_kill_feed_message_text(recipient, message, 0x1c, param, 0x400);
                }
                if (built != 0) {
                    message[0x3ff] = 0;
                    halo::interface::chimera__multiplayer_message(message);
                }
            }
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }

    if (current_game_engine->player_changed_object != nullptr) {
        ((void (*)(uint32_t))current_game_engine->player_changed_object)(param);
    }
}

/**
 * Only meaningful under the single-life-per-round rule (game_engine_variant.unknown_40) and for a player that
 * is currently dead. A dedicated client just returns the server-computed cached value;.
 *
 * @address 0x460e40
 */
uint8_t EnginePlayers::player_has_respawn_priority(uint32_t player_index)
{
    player *self = halo::game::player_at(player_index);
    uint8_t result = 0;

    if (game_engine_variant.odd_man_out != 0 && self->unit == (datum_index)halo::k_dword_none) {
        result = 1;
        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
            return self->odd_man_out;
        }
        if (self->deaths < 1) {
            result = 0;
        } else {
            data_iterator iter;
            void *element;

            iter.data = player_data;
            iter.next_index = 0;
            iter.index = (datum_index)halo::k_dword_none;
            iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

            element = halo::memory::data_iterator_next(&iter);
            if (element != 0) {
                do {
                    player *other = (player *)element;
                    if (other->unit == (datum_index)halo::k_dword_none && other != self &&
                        (self->last_death_tick < other->last_death_tick ||
                         (other->last_death_tick == self->last_death_tick &&
                          (player_index & halo::k_datum_slot_mask) < halo::k_word_none))) {
                        result = 0;
                    }
                    element = halo::memory::data_iterator_next(&iter);
                } while (element != 0);
                self->odd_man_out = result;
                return result;
            }
        }
        self->odd_man_out = result;
    }
    return result;
}

/**
 * True when the game has a lives limit, the player is currently dead, and their death count has reached (or
 * passed) that limit.
 *
 * @address 0x460f30
 */
uint8_t EnginePlayers::player_is_eliminated(uint32_t player_index)
{
    if (0 < game_engine_variant.lives_per_round) {
        player *p = halo::game::player_at(player_index);
        if (p->unit == (datum_index)halo::k_dword_none && game_engine_variant.lives_per_round <= p->deaths) {
            return 1;
        }
    }
    return 0;
}

/**
 * Resets a player for a new life: seeds the per-life fields, assigns the player a team under the current
 * variant (auto-assignment alternates through the engine's team counter), runs the kill-feed notification loop
 * and then the active engine's player_new_life slot.
 *
 * @address 0x45c440
 */
void EnginePlayers::player_new_life(uint32_t player_handle)
{
    player *p;
    uint32_t kill_feed_handle;
    uint32_t sentinel;
    int32_t team;

    p = halo::game::player_at(player_handle);
    p->hud_message_index = (datum_index)halo::k_dword_none;
    p->hud_message_player = (datum_index)halo::k_dword_none;
    p->speed = 1.0f;
    p->teleporter_flag_index = (datum_index)halo::k_dword_none;
    p->nameplate_target_player = (datum_index)halo::k_dword_none;
    p->objective_time = 0;

    if (current_game_engine == (game_engine_definition *)0) {
        return;
    }

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
        if (game_engine_variant.teams == 0) {
            p->team_index = p->team_index_desired;
            p->team = (int32_t)p->team_index_desired;
            game_engine_auto_team_counter = game_engine_auto_team_counter + 1;
        } else if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client || halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
            team = (int32_t)p->team_index % 2;
            p->team = team;
        } else {
            p->team_index = (int8_t)game_engine_auto_team_counter;
            p->team = (int32_t)(int8_t)game_engine_auto_team_counter;
            game_engine_auto_team_counter = (game_engine_auto_team_counter + 1) % 2;
        }
    }

    sentinel = halo::k_dword_none;
    {
        data_iterator iter;
        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)halo::k_dword_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        p = (player *)halo::memory::data_iterator_next(&iter);
        while (p != (player *)0) {
            kill_feed_handle = (player_handle == halo::k_dword_none) ? sentinel : player_handle;
            halo::game::chimera__kill_feed(iter.index, kill_feed_handle, 0, (datum_index)sentinel, 0);
            p = (player *)halo::memory::data_iterator_next(&iter);
        }
    }

    if (current_game_engine->player_new_life != nullptr) {
        ((void (*)(uint32_t))current_game_engine->player_new_life)(player_handle);
    }
}

/**
 * Per-tick check that counts down a dead player's respawn timer, playing countdown cues and staggering
 * respawns across frames.
 *
 * @address 0x460f70
 */
uint8_t EnginePlayers::player_ready_to_respawn(uint32_t player_index)
{
    player *p;
    uint8_t ready;

    if (current_game_engine == 0) {
        return 0;
    }
    p = halo::game::player_at(player_index);
    if (p->marked_for_deletion == 1) {
        return 0;
    }

    if (p->deaths != 0) {
        if (halo::game::game_engine_player_is_eliminated(player_index) != 0) {
            return 0;
        }
        if (halo::game::game_engine_player_has_respawn_priority(player_index) != 0) {
            return 0;
        }
        if (game_engine_state_value == _game_engine_state_post_game) {
            return 0;
        }
        if (game_engine_state_value == _game_engine_state_ended) {
            return 0;
        }
        if (0 < p->respawn_timer) {
            if (p->local_player_index != -1 &&
                (p->respawn_timer == 0x5a || p->respawn_timer == 0x3c ||
                 p->respawn_timer == 0x1e || p->respawn_timer == 1)) {
                halo::game::game_engine_queue_multiplayer_sound(p->respawn_timer == 1 ? 0x1f : 0x1d, halo::k_dword_none, 0);
            }
            p->respawn_timer = p->respawn_timer - 1;
            ready = (p->respawn_timer == 0);
            if (!ready) {
                return ready;
            }
        }
    }
    ready = 1;

    if (game_time->game_time < 4) {
        return ready;
    }
    {
        uint32_t phase = (uint32_t)game_time->game_time & 0x8000001f;
        if ((int32_t)phase < 0) {
            phase = (phase - 1 | 0xffffffe0) + 1;
        }
        if (phase != (player_index & 0x1f)) {
            return 0;
        }
    }
    return ready;
}

/**
 * Returns false only for a player that is not marked for deletion, is not yet eliminated by the lives limit
 * and does not have odd-man-out respawn priority; true in every other case.
 *
 * @address 0x463100
 */
uint8_t EnginePlayers::player_respawn_priority_gate(uint32_t player_index)
{
    player *p = halo::game::player_at(player_index);

    if (p->marked_for_deletion == 0 &&
        (game_engine_variant.lives_per_round < 1 || p->unit != (datum_index)halo::k_dword_none ||
         p->deaths < game_engine_variant.lives_per_round)) {
        if (halo::game::game_engine_player_has_respawn_priority(player_index) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * Clears a player per-round engine bookkeeping and invokes the active game variant optional reset callback.
 *
 * @address 0x463620
 */
void EnginePlayers::player_round_reset(int32_t player_handle, int32_t callback_argument)
{
    if (current_game_engine != 0) {
        halo::game::player_kill_and_release_unit((uint32_t)player_handle, 0);
        if (current_game_engine->player_round_reset != 0) {
            ((void (*)(int32_t, int32_t))current_game_engine->player_round_reset)(
                player_handle, callback_argument);
        }
    }
}

/**
 * Picks a random live player who is not `player_or_all` itself, not the player's previous pick, on the
 * opposing team and currently driving a unit; stores the pick (or k_datum_index_none) into the player's own
 * unknown_88 field, then dispatches a kill-feed message (id 0x20) about it -- either to `player_or_all` alone,
 * or, when `player_or_all` is k_datum_index_none, to every player in turn.
 *
 * @address 0x46f1a0
 */
void EnginePlayers::player_select_random_target(datum_index player_or_all)
{
    uint16_t self_index = (uint16_t)player_or_all;
    player *self = halo::game::player_at(self_index);
    int32_t previous_target = self->slayer_target;
    int32_t match_count = 0;
    datum_index winner = k_datum_index_none;
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    element = halo::memory::data_iterator_next(&iter);
    while (element != 0) {
        datum_index candidate = iter.index;
        player *candidate_player =
            halo::game::player_at(candidate);

        if (candidate != player_or_all && (int32_t)candidate != previous_target &&
            candidate_player->team != self->team && candidate_player->unit != k_datum_index_none) {
            match_count = match_count + 1;
        }
        element = halo::memory::data_iterator_next(&iter);
    }

    if (match_count > 0) {
        int16_t pick;

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        pick = (int16_t)(((halo::math::globals().random_seed_global >> 16) *
                          (uint32_t)(int32_t)(int16_t)match_count) >> 16);

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = k_datum_index_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = halo::memory::data_iterator_next(&iter);
        while (element != 0) {
            datum_index candidate = iter.index;
            player *candidate_player =
                halo::game::player_at(candidate);

            if (candidate != player_or_all && (int32_t)candidate != previous_target &&
                candidate_player->team != self->team && candidate_player->unit != k_datum_index_none) {
                if (pick == 0) {
                    winner = candidate;
                    break;
                }
                pick = pick - 1;
            }
            element = halo::memory::data_iterator_next(&iter);
        }
    }

    self->slayer_target = (int32_t)winner;
    if (winner == k_datum_index_none) {
        return;
    }

    if (player_or_all != k_datum_index_none) {
        halo::game::chimera__kill_feed(player_or_all, (int32_t)player_or_all, 0x20, winner, 1);
        return;
    }

    {
        data_iterator broadcast_iter;
        void *broadcast_element;

        broadcast_iter.data = player_data;
        broadcast_iter.next_index = 0;
        broadcast_iter.index = k_datum_index_none;
        broadcast_iter.signature = (uint32_t)(uintptr_t)broadcast_iter.data ^ k_data_iterator_signature;
        broadcast_element = halo::memory::data_iterator_next(&broadcast_iter);
        while (broadcast_element != 0) {
            halo::game::chimera__kill_feed(broadcast_iter.index, (int32_t)broadcast_iter.index, 0x20, winner, 1);
            broadcast_element = halo::memory::data_iterator_next(&broadcast_iter);
        }
    }
}

/**
 * Returns true at once when fewer than two players are active. Otherwise scans the players: one passes when it
 * is marked for deletion, has no unit and is odd man out (or out of lives in a limited-lives game), or its
 * team matches the running reference team.
 *
 * @address 0x45c750
 */
uint8_t EnginePlayers::players_ready_for_bsp_switch(void)
{
    data_iterator iterator;
    player *p;
    int32_t reference_team;
    datum_index reread_unit;
    int16_t reread_deaths;
    uint8_t result;
    uint8_t odd_man_out_result;
    uint8_t no_reference_team;

    if (halo::game::players_active_count() < 2) {
        return 1;
    }

    result = 0;
    reference_team = -1;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    if (p != (player *)0) {
        while (1) {
            uint8_t keep_going;

            if (p->marked_for_deletion != 0) {
                keep_going = 1;
            } else if (p->unit == k_datum_index_none) {
                odd_man_out_result = halo::game::game_engine_player_has_respawn_priority(iterator.index);
                if (odd_man_out_result != 0) {
                    keep_going = 1;
                } else if (0 < game_engine_variant.lives_per_round &&
                           p->unit == k_datum_index_none &&
                           game_engine_variant.lives_per_round <= p->deaths) {
                    keep_going = 1;
                } else {
                    keep_going = 0;
                }
            } else {
                keep_going = 0;
            }

            if (!keep_going) {
                if (p->team == reference_team) {
                    keep_going = 1;
                } else {
                    no_reference_team = (reference_team == -1);
                    reference_team = p->team;
                    keep_going = no_reference_team;
                }
            }

            if (!keep_going) {
                break;
            }

            p = (player *)halo::memory::data_iterator_next(&iterator);
            if (p == (player *)0) {
                return 0;
            }
        }
        result = 1;
    }
    return result;
}

/**
 * Stricter variant of the BSP-switch readiness check that also detects players disagreeing about which
 * structure BSP to switch to.
 *
 * @address 0x45c830
 */
uint8_t EnginePlayers::players_ready_for_bsp_switch_strict(void)
{
    data_iterator iterator;
    player *p;
    uint8_t result;
    int32_t lives_cached;
    datum_index reread_unit;
    int16_t reread_deaths;

    if (halo::game::players_active_count() < 2) {
        result = 1;
        if (halo::game::players_active_count() == 1) {
            iterator.data = player_data;
            iterator.next_index = 0;
            iterator.index = k_datum_index_none;
            iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
            p = (player *)halo::memory::data_iterator_next(&iterator);
            lives_cached = game_engine_variant.lives_per_round;
            while (p != (player *)0) {
                reread_unit = p->unit;
                reread_deaths = p->deaths;
                if (lives_cached < 1 || reread_unit != -1 || reread_deaths < lives_cached) {
                    result = 1;
                } else {
                    result = 0;
                }
                p = (player *)halo::memory::data_iterator_next(&iterator);
            }
        }
        return result;
    }

    if (0 < game_engine_variant.lives_per_round || game_engine_variant.odd_man_out != 0) {
        int32_t spawned_reference_team;
        uint8_t disagreement_found;
        int32_t previous_team;
        int32_t this_team;
        int32_t carry_team;
        uint8_t counts;
        uint8_t has_spawned_reference_team;

        spawned_reference_team = -1;
        disagreement_found = 0;
        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        p = (player *)halo::memory::data_iterator_next(&iterator);
        previous_team = -1;
        if (p != (player *)0) {
            do {
                if (p->marked_for_deletion == 0 && p->unit == k_datum_index_none &&
                    (halo::game::game_engine_player_has_respawn_priority(iterator.index) != 0 ||
                     (0 < game_engine_variant.lives_per_round &&
                      (reread_unit = p->unit,
                       reread_unit == k_datum_index_none) &&
                      (reread_deaths = p->deaths,
                       game_engine_variant.lives_per_round <= reread_deaths)))) {
                    counts = 0;
                } else {
                    counts = 1;
                }

                this_team = p->team;
                carry_team = this_team;
                if (previous_team != -1) {
                    carry_team = previous_team;
                    if (previous_team != this_team) {
                        disagreement_found = 1;
                    }
                }

                if (counts && this_team != spawned_reference_team) {
                    has_spawned_reference_team = (spawned_reference_team != -1);
                    spawned_reference_team = this_team;
                    if (has_spawned_reference_team) {
                        return 1;
                    }
                }

                p = (player *)halo::memory::data_iterator_next(&iterator);
                previous_team = carry_team;
            } while (p != (player *)0);

            if (disagreement_found) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Iterates the player datum pool calling game_engine_player_new_life once per entry, then notifies the active
 * multiplayer game engine's reset_round callback.
 *
 * @address 0x45b8b0
 */
void EnginePlayers::reset_all_players(void)
{
    data_iterator iterator;
    void *entry;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = halo::memory::data_iterator_next(&iterator);
    while (entry != nullptr) {
        halo::game::game_engine_player_new_life(iterator.index);
        entry = halo::memory::data_iterator_next(&iterator);
    }

    if (current_game_engine != (game_engine_definition *)0 && current_game_engine->reset_round != nullptr) {
        ((void (*)(void))current_game_engine->reset_round)();
    }
}

/**
 * Zeroes the player_control_globals header (action flags) and local player 0's whole look-state record, then
 * reinitializes its defaults (unit = none, desired weapon/grenade/zoom = none, nameplate_target = none, pitch
 * clamped to +-1.4906585 rad), and, the first time this runs, seeds the two global default look-rate constants
 * from the globals tag's player_control block.
 *
 * @address 0x470de0
 */
void EnginePlayers::reset_player_look_state(void)
{
    local_player_control *look = &player_control_globals_ptr->local_players[0];
    GlobalsPlayerControl *player_control =
        (GlobalsPlayerControl *)global_globals->player_control.pointer;

    player_control_globals_ptr->action_flags = 0;
    player_control_globals_ptr->action_flags_latched = 0;
    player_control_globals_ptr->action_flags_edge = 0;
    player_control_globals_ptr->flags = 0;

    memset(look, 0, sizeof(*look));
    look->unit = k_datum_index_none;
    look->desired_weapon_index = -1;
    look->desired_grenade_index = -1;
    look->desired_zoom_level = -1;
    look->nameplate_target = k_datum_index_none;
    look->autolevelling_active = 0;
    look->pitch_maximum = 1.4906585f;
    look->pitch_minimum = -1.4906585f;
    look->suppressed_buttons = 0;
    look->suppressed_until_released = 0;

    if (look_pitch_rate_setting[0] == 0.0f) {
        look_pitch_rate_setting[0] = player_control->look_default_pitch_rate;
    }
    if (look_yaw_rate_setting[0] == 0.0f) {
        look_yaw_rate_setting[0] = player_control->look_default_yaw_rate;
    }
}

/**
 * For every player_profile still marked in_use in the 16-slot cache, zeroes 15 dwords (player + 0x90 .. +
 * 0xcc) of statistics on that profile's player.
 *
 * @address 0x468150
 */
void EnginePlayers::reset_player_profile_stats(void)
{
    int32_t i;

    for (i = 0; i < 16; i++) {
        if (player_profile_cache[i].in_use == 1) {
            constexpr size_t k_stats_offset = 0x90;
            constexpr size_t k_stats_size = 15 * sizeof(uint32_t);
            static_assert(offsetof(player, killing_spree_count) >= k_stats_offset &&
                          offsetof(player, telefrag_ticks) == k_stats_offset + k_stats_size, "stats block spans spree count .. objective score");
            memset(reinterpret_cast<uint8_t *>(halo::game::player_at(player_profile_cache[i].player)) + k_stats_offset, 0, k_stats_size);
        }
    }
}

/**
 * While hosting a multiplayer game, forwards every player (with its clamped respawn_time, >= 90 ticks) to
 * player_kill_and_release_unit and resets their respawn_timer to 0; then, unconditionally, sweeps every live
 * biped object and deletes any whose network_role is 0 (also via object_delete_unparented first) or 3 (via
 * object_delete_recursive).
 *
 * @address 0x467e60
 */
void EnginePlayers::reset_respawns_and_cleanup_bipeds(void)
{
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        data_iterator player_iter;
        uint32_t unused_checksum;
        player *p;

        player_iter.data = player_data;
        player_iter.next_index = 0;
        player_iter.index = (datum_index)halo::k_dword_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        unused_checksum = (uint32_t)player_data ^ halo::game::k_iterator_signature_key;

        p = (player *)halo::memory::data_iterator_next(&player_iter);
        while (p != (player *)0) {
            int32_t respawn_time = game_engine_variant.respawn_time;
            if (respawn_time < 0x5a) {
                respawn_time = 0x5a;
            }
            halo::game::player_kill_and_release_unit((uint32_t)player_iter.index, respawn_time);
            p->respawn_timer = 0;
            p = (player *)halo::memory::data_iterator_next(&player_iter);
        }
        (void)unused_checksum;
    }

    {
        object_iterator obj_iter;
        object *obj;

        obj_iter.type_mask = 0x001;
        obj_iter.flags_mask = 0;
        obj_iter.unknown_05 = 0;
        obj_iter.index = 0;
        obj_iter.handle = (datum_index)halo::k_dword_none;

        obj = halo::objects::object_iterator_next(&obj_iter);
        while (obj != (object *)0) {
            if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
                if (obj->network_role == 0) {
                    halo::objects::object_delete_unparented(obj_iter.handle);
                    halo::objects::object_delete_recursive(obj_iter.handle, 0);
                } else if (obj->network_role == 3) {
                    halo::objects::object_delete_recursive(obj_iter.handle, 0);
                }
            }
            obj = halo::objects::object_iterator_next(&obj_iter);
        }
    }
}

/**
 * If the active game engine implements player_team_changed, defers the whole team assignment to it. Otherwise
 * falls back to alternating teams by parity of the player's local_player_index.
 *
 * @address 0x4611b0
 */
void EnginePlayers::resolve_player_team(uint32_t player_index)
{
    player *p;

    if (current_game_engine == 0) {
        return;
    }
    if (current_game_engine->player_team_changed != 0) {
        ((void (*)(uint32_t))current_game_engine->player_team_changed)(player_index);
        return;
    }

    p = halo::game::player_at(player_index);
    {
        uint32_t team = (uint32_t)p->local_player_index & 0x80000001;
        if ((int32_t)team < 0) {
            team = (team - 1 | 0xfffffffe) + 1;
        }
        p->team = (int32_t)team;
    }
}

/**
 * Reattaches a player's unit to a new parent object, recomputing its transform and light attachments. Dead
 * code in the retail build (no callers), superseded by a sibling routine.
 *
 * @address 0x475270
 */
void EnginePlayers::reattach_player_unit_unused(uint32_t player_index, uint32_t target_object, void *local_offset)
{
    datum_index unit_handle;
    object *target_obj;
    uint8_t skip_trigger_check;
    int32_t unknown_result;

    unit_handle = halo::game::player_at(player_index)->unit;
    target_obj = halo::objects::object_try_and_get((datum_index)target_object, _object_mask_biped);
    if (target_obj == (object *)0) {
        return;
    }

    if (local_player_globals->bsp_switch_trigger_volume_index == -1 ||
        (unit_handle != (datum_index)-1 &&
         halo::scenario::scenario_query::trigger_volume_contains_point(static_cast<int16_t>(halo::tag_block_at<ScenarioBSPSwitchTriggerVolume>(halo::scenario::globals().scenario->bsp_switch_trigger_volumes, local_player_globals->bsp_switch_trigger_volume_index).trigger_volume), &halo::game::object_at(unit_handle)->bounding_center)
             != 0)) {
        skip_trigger_check = 0;
    } else {
        skip_trigger_check = 1;
    }

    unknown_result = (int32_t)halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)0, (real_point3d *)0);
    if (unknown_result == -1 || skip_trigger_check != 0) {
        object *current_parent_obj = halo::game::object_at(unit_handle);
        if (target_obj->parent_object != (datum_index)-1 &&
            target_obj->parent_object != current_parent_obj->parent_object &&
            halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
            object *unit_obj = halo::game::object_at(unit_handle);
            unit_data *unit = halo::game::unit_data_of(unit_obj);
            datum_index driver = unit->driver_unit_index;

            if (driver != (datum_index)-1 && unit->vehicle_seat_index != -1) {
                object *driver_obj = halo::game::object_at(driver);
                unit_data *driver_unit = halo::game::unit_data_of(driver_obj);
                Unit *driver_tag = (Unit *)halo::game::tag_data_at(driver_obj->definition_tag);
                real_matrix4x3 local_transform;
                real_matrix4x3 result_transform;
                Unit *unit_tag;
                Model *model_tag;

                halo::objects::object_get_node_local_transform(
                    driver, halo::tag_block_at<UnitSeat>(driver_tag->seats, unit->vehicle_seat_index).marker_name.string,
                    (object_marker *)&local_transform, 1);

                unit_tag = (Unit *)halo::game::tag_data_at(unit_obj->definition_tag);
                model_tag = reinterpret_cast<Model *>(halo::game::tag_data_at(unit_tag->base.model.tag_id.index));
                {
                    ModelNode *model_nodes = halo::tag_block_elements<ModelNode>(model_tag->nodes);

                    if (driver_unit->driver_unit_index == unit_handle &&
                        driver_unit->animation_state != halo::units::animation_state_value(halo::units::unit_animation_state_id::opening) && unit_obj->parent_object != (datum_index)-1) {
                        halo::units::unit_try_set_animation_state(unit_obj->parent_object, halo::units::animation_state_value(halo::units::unit_animation_state_id::opening));
                    }

                    unit->last_parent_object_index = driver;
                    unit->last_seat_change_tick = game_time->game_time;
                    if (unit->driver_unit_index == unit_handle) {
                        unit->driver_unit_index = (datum_index)-1;
                    }
                    if (unit->gunner_unit_index == unit_handle) {
                        unit->gunner_unit_index = (datum_index)-1;
                    }

                    halo::objects::object_snap_to_parent_marker_and_detach(unit_handle);
                    halo::objects::object_set_position_and_orientation(unit_handle, 0, 0, 0);
                    halo::math::matrix4x3_multiply(&local_transform, reinterpret_cast<real_matrix4x3 *>(&model_nodes->scale),
                                        &result_transform);
                    unit_obj->forward = result_transform.forward;
                    unit_obj->up = result_transform.up;

                    {
                        uint8_t *unit_tag_data = halo::game::tag_data_at(unit_obj->definition_tag);
                        if (halo::tag_id_bits<int32_t>(reinterpret_cast<Unit *>(unit_tag_data)->base.model.tag_id) != -1) {
                            if ((unit_obj->flags & 1) != 0) {
                                halo::objects::object_for_each_light_attachment(0, 1, 0);
                            }
                            if (halo::tag_id_bits<int32_t>(reinterpret_cast<Unit *>(unit_tag_data)->base.model.tag_id) != -1) {
                                object_header *unit_header = &halo::game::object_header_at(unit_handle);
                                unit_obj->flags = unit_obj->flags & ~1u;
                                unit_header->flags = unit_header->flags | 2;
                            }
                        }
                    }

                    unit->vehicle_seat_index = -1;
                    unit->base_animation_state = 2;
                    if (driver_unit->driver_unit_index == unit_handle) {
                        driver_unit->driver_unit_index = (datum_index)-1;
                    }
                    if (driver_unit->gunner_unit_index == unit_handle) {
                        driver_unit->gunner_unit_index = (datum_index)-1;
                    }
                }

                halo::units::unit_recompute_seat_occupants(unit_handle);
                halo::units::unit_pick_and_ready_next_weapon(unit_handle);
                halo::units::unit_update_animation_state_machine(unit_handle, k_unit_exit_seat_request);

                {
                    real_point3d &translation = halo::objects::object_block<real_orientation>(*unit_obj, unit_obj->node_function_values)[0].translation;
                    memcpy(&translation, &result_transform.forward, sizeof(result_transform.forward));
                }

                if (unit_obj->type == _object_type_biped) {
                    halo::units::unit_reset_orientation_and_find_position(unit_handle, driver);
                }
                halo::objects::object_recalculate_bounding_radius_recursive(unit_handle);

                if (halo::units::unit_all_seats_unoccupied(unit_handle) == 1) {
                    object *local_obj = halo::objects::object_try_and_get((datum_index)-1, _object_mask_vehicle);
                    if (local_obj != (object *)0) {
                        ((vehicle_object *)local_obj)->vehicle.network_update_tick = game_time->game_time;
                    }
                }

                if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
                    player *client_player = static_cast<player *>(halo::memory::datum_get(unit_handle, halo::objects::globals().object_data));
                    if (client_player != nullptr && client_player->local_player_index == -1) {
                        client_player->position_updates.read_index = 0;
                        client_player->position_updates.write_index = 0;
                        client_player->vehicle_updates.read_index = 0;
                        client_player->vehicle_updates.write_index = 0;
                    }
                }
            }

            if (unit_obj->network_role == 0) {
                halo::units::unit_dispatch_scripted_event_9(1, (int32_t)unit_handle);
            }

            if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
                datum_index controlling_player = unit->controlling_player;
                if (controlling_player != (datum_index)-1) {
                    int16_t index = (int16_t)controlling_player;
                    if (index >= 0 && index < player_data->maximum_count) {
                        player *plr = halo::game::player_at(index);
                        int16_t salt = (int16_t)(controlling_player >> 16);
                        if (plr->identifier != 0 && (salt == 0 || plr->identifier == salt) &&
                            plr->local_player_index == -1 && halo::networking::globals().client != 0) {
                            halo::networking::player_update_history_free_all(halo::networking::globals().client->update_history);
                        }
                    }
                }
            }
        }

        if (target_obj->parent_object == (datum_index)-1) {
            halo::game::player_find_placement_position(player_index, target_object, (real_point3d *)local_offset);
        }
        local_player_globals->teleported = 0;
    }
}

/**
 * Advances the shared LCG once, then scans the small recent-location table starting from a pseudo-random
 * offset for the first entry that differs from `exclude_value`, wrapping around; returns `fallback` if the
 * table is empty or every entry matches `exclude_value`.
 *
 * @address 0x46a1b0
 */
int32_t EnginePlayers::pick_random_recent_location(int32_t exclude_value, int32_t fallback)
{
    int16_t i;

    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);

    if (game_engine_recent_location_count < 1) {
        return fallback;
    }

    for (i = 0; i < game_engine_recent_location_count; i++) {
        int16_t slot = (int16_t)(((int32_t)i +
            (int16_t)(((int32_t)(halo::math::globals().random_seed_global >> 0x10) * (int32_t)game_engine_recent_location_count) >> 0x10))
            % (int32_t)game_engine_recent_location_count);
        if (exclude_value != (int32_t)game_engine_recent_location_table[slot]) {
            return (int32_t)game_engine_recent_location_table[slot];
        }
    }
    return fallback;
}

/**
 * Zeroes both grenade-type counts (unit_data::grenade_counts[0] and [1]) on every player's current unit, e.g.
 * as part of a new-round reset.
 *
 * @address 0x467de0
 */
void EnginePlayers::reset_all_unit_grenade_counts(void)
{
    data_iterator iterator;
    uint32_t unused_checksum;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)halo::k_dword_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    unused_checksum = (uint32_t)player_data ^ halo::game::k_iterator_signature_key;

    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != (datum_index)halo::k_dword_none) {
            object *unit_obj = halo::game::object_at(p->unit);
            unit_data *unit = halo::game::unit_data_of(unit_obj);
            unit->grenade_counts[0] = 0;
            unit->grenade_counts[1] = 0;
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }

    (void)unused_checksum;
}

/**
 * Scans every in-use player on `team` with deaths below score_limit; if lives_per_round is configured (> 0)
 * and any such player's unit is gone (-1) while their own deaths have already reached lives_per_round, the
 * team is reported as NOT having scoring capacity.
 *
 * @address 0x46e250
 */
uint8_t EnginePlayers::team_has_scoring_capacity(int32_t team)
{
    data_iterator iter;
    player *p;
    uint8_t has_capacity = 1;

    if (game_engine_variant.engine.race.team_scoring != 0) {
        return 1;
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)halo::memory::data_iterator_next(&iter);
    while (p != (player *)0) {
        if (p->team == team &&
            ((struct player *)p)->objective_time_words.race_laps < game_engine_variant.score_limit &&
            game_engine_variant.lives_per_round > 0) {
            if (p->unit == (datum_index)halo::k_dword_none &&
                (int32_t)p->deaths >= game_engine_variant.lives_per_round) {
                has_capacity = 0;
            }
        }
        p = (player *)halo::memory::data_iterator_next(&iter);
    }
    return has_capacity;
}

/**
 * Returns whether player scores should be tracked/ displayed individually rather than by team, based on the
 * team-mode flag and an option bit.
 *
 * @address 0x4635e0
 */
uint8_t EnginePlayers::scores_tracked_individually(void)
{
    uint8_t result = 1;

    if (current_game_engine != 0) {
        uint8_t no_team_mode = (game_engine_variant.objective_indicator == 0);
        if (game_engine_variant.game_engine_index == _game_engine_slayer &&
            game_engine_variant.engine.slayer.kill_in_order == 0) {
            no_team_mode = 0;
        }
        result = ((halo::game::variant_flag_set(game_engine_variant.flags, halo::game::game_variant_flags::players_on_radar) ? 1 : 0)) | no_team_mode;
    }
    return result;
}

}
