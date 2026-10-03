#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "win32.h"
#include "crt.h"

namespace halo::game {

/**
 * Client side of the update-queue protocol: staging, distribution and per-tick application of
 * server updates.
 */
class UpdateClient {
public:
    UpdateClient() = delete;

    static void advance_read_cursor(int32_t target_tick, const uint32_t *record);
    static void dispose();
    static uint32_t distribute_staged_entry(uint8_t *out);
    static uint32_t update_client_new();
    static uint32_t queue_apply_tick(player_action *out_actions, client_update_carry *out_carry);
    static update_record * queue_get_slot(int32_t tick);
    static void stage_entry(uint32_t *source);
};

/**
 * Server side of the update-queue protocol: per-player tick history and queue creation.
 */
class UpdateServer {
public:
    UpdateServer() = delete;

    static void dispose();
    static uint8_t update_server_new();
    static void push_player_tick_history();
    static void queue_create_entry(datum_index requested_handle);
    static void queue_get_history_entry(int32_t *out_record, int32_t *out_tick, datum_index queue_handle);
    static void queue_push_history(int16_t machine_index, int32_t tick_count, uint32_t *source, uint32_t extra);
};

/**
 * Shared update-queue lifecycle and catch-up ticking.
 */
class UpdateQueues {
public:
    UpdateQueues() = delete;

    static void dispose();
    static void revert();
    static void run_catchup_ticks(int16_t tick_count);
};

/**
 * Non-owning view of a player record receiving remote position updates.
 */
class PlayerNetworkState {
public:
    player * plr;

    explicit constexpr PlayerNetworkState(player * plr_) : plr(plr_) {}

    void apply_remote_position_update(object *unit_obj);
    void apply_remote_vehicle_position_update(object *unit_obj);
    void apply_first_position_update(uint32_t field0);
};

}  // namespace halo::game
