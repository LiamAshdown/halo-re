#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

namespace halo::game {

/**
 * Non-owning view of the fixed-record circular queue used by the update and network layers.
 */
class CircularQueue {
public:
    circular_queue * queue;

    explicit constexpr CircularQueue(circular_queue * queue_) : queue(queue_) {}

    int32_t count();
    uint8_t pop(void **out_record);
    uint8_t push(void *source);
    void destroy();
};

/**
 * View of a circular queue holding remote player position updates.
 */
class PositionUpdateQueue {
public:
    circular_queue * queue;

    explicit constexpr PositionUpdateQueue(circular_queue * queue_) : queue(queue_) {}

    void create();
    uint8_t find_and_remove(int32_t target_tick, real_point3d *out);
    uint8_t push(real x, real y, real z, int32_t tick, int32_t sequence);
};

/**
 * View of a circular queue holding remote vehicle position updates.
 */
class VehicleUpdateQueue {
public:
    circular_queue * queue;

    explicit constexpr VehicleUpdateQueue(circular_queue * queue_) : queue(queue_) {}

    void create();
    uint8_t find_and_remove(int32_t target_tick, vehicle_update_record *out);
};

/**
 * View of the per-player update queue consumed by the client update path.
 */
class PlayerUpdateQueue {
public:
    player_update_queue * queue;

    explicit constexpr PlayerUpdateQueue(player_update_queue * queue_) : queue(queue_) {}

    void create();
    uint8_t pop_current(player_update_record *out);
};

}  // namespace halo::game
