/**
 * Fixed-rate simulation tick accounting, tick records, time scale and the multiplayer clock.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#include <string.h>
#include <stdint.h>

#include "halo/game/game1_clock.hpp"

extern "C" {
extern game_main_globals *main_game_globals;
extern game_time_globals *game_time;
extern real chimera_contrail_scale;
extern void contrail_update(real delta_time);
extern void particle_systems_update(real delta_time);
extern void particles_update(real tick_delta_time);
extern void widgets_update_all(real tick_delta_time);
extern void weather_update(void);
extern void numeric_countdown_timer_update(void);
extern void game_sound_update(void);
extern int16_t network_game_mode;
extern double floor(double x);
extern int32_t game_time_force_single_tick;
extern void game_simulate_tick(uint32_t predict_pass);
extern void update_run_catchup_ticks(int16_t tick_count);
extern network_server_globals *network_server;
extern void network_game_server_per_frame_tick(int16_t update_count, uint8_t *server);
extern void game_effects_update(float delta_time);
extern int32_t game_engine_accumulate_simulation_ticks(float elapsed_seconds, char keep_remainder);
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length);
extern game_engine_definition *current_game_engine;
extern data_array *player_data;
extern void *data_iterator_next(data_iterator *iterator);
extern uint8_t game_engine_players_ready_for_bsp_switch_strict(void);
extern int32_t game_engine_get_time_remaining(void);
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast);
extern game_variant game_engine_variant;
extern int32_t game_engine_round_reset_tick;
extern uint8_t game_time_unknown_49;
extern int32_t game_time_unknown_48;
extern void update_server_dispose(void);
extern void update_client_dispose(void);
}

namespace halo::game::engine1 {

/**
 * Implements game effects update.
 *
 * @address 0x45b4f0
 */
void SimulationClock::effects_update(real delta_time)
{
    real scale;
    int16_t ticks_this_frame;
    real tick_delta_time;

    scale = (main_game_globals->players_are_double_speed == 0) ? 1.0f : 0.5f;
    ticks_this_frame = game_time->ticks_this_frame;
    tick_delta_time = (real)ticks_this_frame * scale * 0.033333335f;
    delta_time = scale * delta_time;

    if (ticks_this_frame != 0) {
        particles_update(tick_delta_time);
    }
    contrail_update(delta_time);
    particle_systems_update(delta_time);
    if (ticks_this_frame != 0) {
        widgets_update_all(tick_delta_time);
    }
    game_sound_update();
    weather_update();
    chimera_contrail_scale = delta_time;
    numeric_countdown_timer_update();
}

/**
 * Implements a fixed-30Hz-timestep accumulator, converting a frame's elapsed time into a whole number of
 * simulation ticks while carrying the fractional remainder forward.
 *
 * @address 0x470b30
 */
int32_t SimulationClock::accumulate_simulation_ticks(float elapsed_seconds, char keep_remainder)
{
    float scale;
    double floor_result;
    int32_t tick_count;

    if (network_game_mode == 1 || network_game_mode == 2) {
        scale = 1.0f;
    } else {
        scale = game_time->speed;
    }
    scale = scale * 30.0f;

    elapsed_seconds = elapsed_seconds + game_time->leftover_time;
    floor_result = (float)floor((double)(elapsed_seconds * scale));

    tick_count = (int32_t)((float)floor_result > 1000.0f ? 1000.0f : (float)floor_result);

    if (0.0f < scale && keep_remainder == 0) {
        elapsed_seconds = elapsed_seconds - (float)floor_result / scale;
        game_time->leftover_time = elapsed_seconds;
        if (elapsed_seconds < 0.0f) {
            game_time->leftover_time = 0.0f;
        }
    }
    return tick_count;
}

/**
 * Advances the game simulation by the appropriate number of fixed 30Hz ticks for this frame, running per-tick
 * hooks and updating game effects.
 *
 * @address 0x470bf0
 */
void SimulationClock::advance_simulation_ticks(float delta_time)
{
    int32_t tick_count = game_engine_accumulate_simulation_ticks(delta_time, 0);
    int32_t i;

    if (game_time_force_single_tick != 0) {
        tick_count = 1;
    }

    if (network_game_mode == 0) {
        if (!game_time->active) {
            game_time->ticks_this_frame = 0;
            return;
        }
        update_run_catchup_ticks((int16_t)tick_count);
    } else if (network_game_mode == 2) {
        network_game_server_per_frame_tick((int16_t)tick_count, (uint8_t *)network_server);
    }

    for (i = tick_count; i > 0; i--) {
        game_simulate_tick((uint32_t)(i - 1));
        game_time->elapsed_ticks = game_time->elapsed_ticks + 1;
        game_time->game_time = game_time->game_time + 1;
    }
    game_time->ticks_this_frame = (int16_t)tick_count;

    if (network_game_mode != 1 && network_game_mode != 2) {
        game_effects_update(game_time->speed * delta_time);
    } else {
        game_effects_update(delta_time * 1.0f);
    }
}

/**
 * Allocates and zero-initializes a new per-tick game-state record, updates the running CRC over it, and makes
 * it the current tick record.
 *
 * @address 0x470a80
 */
void SimulationClock::allocate_tick_record(void)
{
    game_time_globals *record = (game_time_globals *)(game_state_base + game_state_cursor);
    int32_t record_size = 0x20;

    game_state_cursor = game_state_cursor + 0x20;
    crc32_update(&game_state_crc, (uint8_t *)&record_size, 4);

    memset(record, 0, sizeof(*record));

    game_time = record;
}

/**
 * Implements game engine announce time remaining.
 *
 * Original register convention: EDI -> iterator (matches src/memory/data_iterator_next.c).
 *
 * @address 0x45cae0
 */
int32_t SimulationClock::announce_time_remaining(void)
{
    uint8_t ready;
    int32_t time_remaining;
    int32_t interval;
    data_iterator iterator;
    uint32_t unused_checksum;
    player *p;

    if (current_game_engine == (game_engine_definition *)0) {
        return 0;
    }
    if (network_game_mode != 2) {
        return 0;
    }
    ready = game_engine_players_ready_for_bsp_switch_strict();
    if (ready == 0) {
        return 1;
    }

    time_remaining = game_engine_get_time_remaining();
    if (time_remaining == -1) {
        return 0;
    }
    if (time_remaining == 0) {
        return 1;
    }

    if (time_remaining < 0x97) {
        interval = 0x1e;
    } else if (time_remaining == 900) {
        goto announce;
    } else {
        interval = (8999 < time_remaining) ? 9000 : 0x708;
    }

    if (time_remaining % interval != 0) {
        return 0;
    }

announce:

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    unused_checksum = (uint32_t)player_data ^ 0x69746572;

    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        chimera__kill_feed(iterator.index, (int32_t)iterator.index, 0x1e,
                            (datum_index)time_remaining, 1);
        p = (player *)data_iterator_next(&iterator);
    }
    return 0;
}

/**
 * HUD highlight/pulse weight... how recently each object last scored.
 *
 * @address 0x46e310
 */
void SimulationClock::apply_catchup_speed_boost(void)
{
    data_iterator iter;
    player *p;
    int32_t leader = 0;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        int16_t value = *(int16_t *)((uint8_t *)p + 0xc6);
        if (leader <= value) {
            leader = value;
        }
        p = (player *)data_iterator_next(&iter);
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        float speed = 1.0f;
        int32_t gap = leader - *(int16_t *)((uint8_t *)p + 0xc6);
        if (game_engine_variant.engine.race.race_type == 2) {
            gap /= 3;
        }
        if (gap < 2) {
            if (gap > 0) {
                speed = 1.1f;
            }
        } else {
            speed = 1.2f;
        }
        p->speed = speed;
        p = (player *)data_iterator_next(&iter);
    }
}

/**
 * Computes a time-scaling multiplier from the game speed option and the active game variant's callback
 * overrides.
 *
 * Original register convention: EDX -> param_a, unaff_ESI -> param_b.
 *
 * @address 0x461550
 */
float SimulationClock::compute_time_scale(int32_t param_a, int32_t param_b)
{
    float scale = 1.0f;

    if (current_game_engine != 0) {
        float speed = game_engine_variant.health;
        if (speed < 0.25f) {
            speed = 0.25f;
        } else if (4.0f < speed) {
            speed = 4.0f;
        }
        scale = 1.0f / speed;
    }

    if (param_a != -1 && param_b != -1 && current_game_engine != 0) {
        if (current_game_engine->time_scale_override != 0) {
            char slow = ((char (*)(int32_t, int32_t))current_game_engine->time_scale_override)(param_a, 2);
            if (slow != 0) {
                scale = scale * 1.5f;
            }
        }
        if (current_game_engine != 0 && current_game_engine->time_scale_override != 0) {
            char fast = ((char (*)(int32_t, int32_t))current_game_engine->time_scale_override)(param_b, 3);
            if (fast != 0) {
                return scale * 0.5f;
            }
        }
    }
    return scale;
}

/**
 * Implements game engine get current tick.
 *
 * @address 0x470cd0
 */
int32_t SimulationClock::get_current_tick(void)
{
    return game_time->game_time;
}

/**
 * Implements game engine get time remaining.
 *
 * @address 0x45cab0
 */
int32_t SimulationClock::get_time_remaining(void)
{
    int32_t remaining;

    remaining = -1;
    if (0 < game_engine_variant.time_limit) {
        remaining = (game_engine_variant.time_limit - game_time->game_time) + game_engine_round_reset_tick;
        if (remaining < 0) {
            remaining = 0;
        }
    }
    return remaining;
}

/**
 * Implements game engine get time scale.
 *
 * @address 0x470ce0
 */
float SimulationClock::get_time_scale(void)
{
    if (network_game_mode != 1 && network_game_mode != 2) {
        return game_time->speed;
    }
    return 1.0f;
}

/**
 * Initializes the current tick record's time-scale and accumulator fields and performs game-mode-specific
 * setup.
 *
 * @address 0x470ae0
 */
void SimulationClock::init_tick_record_for_mode(void)
{
    game_time->speed = 1.0f;
    game_time->leftover_time = 0.0f;
    game_time->active = 1;
    game_time_unknown_49 = 1;
    game_time_unknown_48 = 0;

    switch (network_game_mode) {
    case 0:
    case 2:
        update_server_dispose();
        return;
    case 1:
    case 3:
        update_client_dispose();
        return;
    default:
        return;
    }
}

}
