#include "halo/game/gamerest_queues.hpp"

extern "C" {
extern uint8_t circular_queue_pop(circular_queue *queue, void **out_record);
extern uint8_t circular_queue_push(circular_queue *queue, void *source);
}

namespace halo::game {

/**
 * Implements the original `circular_queue_count`.
 *
 * @address 0x47a230
 */
int32_t CircularQueue::count()
{
    int32_t write_index = queue->write_index;
    int32_t read_index = queue->read_index;
    if (read_index < write_index) {
        return write_index - read_index;
    }
    if (write_index < read_index) {
        return (write_index - read_index) + queue->capacity;
    }
    return 0;
}

/**
 * Implements the original `circular_queue_pop`.
 *
 * @address 0x47a200
 */
uint8_t CircularQueue::pop(void **out_record)
{
    int32_t read_index = queue->read_index;
    if (read_index != queue->write_index) {
        *out_record = queue->records[read_index];
        queue->read_index = (read_index + 1) % queue->capacity;
        return 1;
    }
    *out_record = 0;
    return 0;
}

/**
 * Implements the original `circular_queue_push`.
 *
 * @address 0x47a1a0
 */
uint8_t CircularQueue::push(void *source)
{
    int32_t write_index = queue->write_index;
    int32_t read_index = queue->read_index;
    int32_t used;
    int32_t i;
    uint8_t *dst;
    uint8_t *src;

    if (read_index < write_index) {
        used = write_index - read_index;
    } else if (write_index < read_index) {
        used = (queue->capacity - read_index) + write_index;
    } else {
        used = 0;
    }

    if (used < queue->capacity - 1) {
        dst = (uint8_t *)queue->records[write_index];
        src = (uint8_t *)source;
        for (i = 0; i < queue->record_size; i++) {
            dst[i] = src[i];
        }
        queue->write_index = (write_index + 1) % queue->capacity;
        return 1;
    }
    return 0;
}

/**
 * Implements the original `network_queue_destroy`.
 *
 * @address 0x47a090
 */
void CircularQueue::destroy()
{
    GlobalFree(queue->records);
    queue->records = 0;
    GlobalFree(queue->storage);
    queue->storage = 0;
}

/**
 * Implements the original `position_update_queue_create`.
 *
 * @address 0x47a020
 */
void PositionUpdateQueue::create()
{
    uint32_t *storage;
    uint32_t *records;
    int32_t i;
    uint8_t *record_cursor;

    storage = (uint32_t *)GlobalAlloc(0, 600);
    queue->storage = storage;
    for (i = 0; i < 0x96; i++) {
        storage[i] = 0;
    }

    record_cursor = (uint8_t *)queue->storage;
    records = (uint32_t *)GlobalAlloc(0, 0x78);
    queue->records = (void **)records;
    records[0] = 0;
    queue->capacity = 0x1e;
    queue->record_size = 0x14;
    queue->read_index = 0;
    queue->write_index = 0;

    i = 0;
    do {
        ((uint32_t *)queue->records)[i / 4] = (uint32_t)record_cursor;
        i = i + 4;
        record_cursor = record_cursor + 0x14;
    } while (i < 0x78);
}

/**
 * Peeks (without removing) the head of `queue`. If it matches `target_tick`, copies its x/y/z
 * into `out` and removes it, returning 1. If it is older than `target_tick` by more than its
 * own "sequence" distance (mod 0x40, wrapping), discards it (this branch DOES remove it) and
 * retries. Otherwise (or once the queue is empty) returns 0 and leaves the queue untouched.
 *
 * @address 0x47a100
 */
uint8_t PositionUpdateQueue::find_and_remove(int32_t target_tick, real_point3d *out)
{
    position_update_record *record;

    if (queue->read_index == queue->write_index) {
        return 0;
    }
    record = (position_update_record *)queue->records[queue->read_index];

    if ((int32_t)record->tick == target_tick) {
        *out = record->position;

        if (queue->read_index != queue->write_index) {
            queue->read_index = (queue->read_index + 1) % queue->capacity;
            return 1;
        }
        return 0;
    }

    {
        int32_t distance;
        int32_t tick = (int32_t)record->tick;
        if (tick < target_tick) {
            distance = (tick - target_tick) + 0x40;
        } else if (target_tick < tick) {
            distance = tick - target_tick;
        } else {
            distance = 0;
        }

        if ((int32_t)record->sequence < distance) {
            void *discarded;
            CircularQueue(queue).pop(&discarded);
            return PositionUpdateQueue(queue).find_and_remove(target_tick, out);
        }
        return 0;
    }
}

/**
 * blam-cc: EBX -> queue, stack -> tick, sequence, x, y, z
 *
 * @address 0x47a0c0
 */
uint8_t PositionUpdateQueue::push(real x, real y, real z, int32_t tick, int32_t sequence)
{
    position_update_record record;
    record.tick = tick;
    record.sequence = sequence;
    record.position.x = x;
    record.position.y = y;
    record.position.z = z;
    return CircularQueue(queue).push(&record);
}

/**
 * Implements the original `vehicle_update_queue_create`.
 *
 * @address 0x47a250
 */
void VehicleUpdateQueue::create()
{
    uint32_t *storage;
    uint8_t *record_cursor;
    int32_t i;

    storage = (uint32_t *)GlobalAlloc(0, 0x870);
    queue->storage = storage;
    for (i = 0; i < 0x21c; i++) {
        storage[i] = 0;
    }

    record_cursor = (uint8_t *)queue->storage;
    queue->records = (void **)GlobalAlloc(0, 0x78);
    ((uint32_t *)queue->records)[0] = 0;
    queue->capacity = 0x1e;
    queue->record_size = 0x48;
    queue->read_index = 0;
    queue->write_index = 0;

    i = 0;
    do {
        ((uint32_t *)queue->records)[i / 4] = (uint32_t)record_cursor;
        i = i + 4;
        record_cursor = record_cursor + 0x48;
    } while (i < 0x78);
}

/**
 * Peeks (without removing) the head of `queue`. If it matches `target_tick`, copies the whole
 * 18-dword record into `out` and removes it, returning 1. If it is older than `target_tick` by
 * more than its own "sequence" distance (mod 0x40, wrapping), discards it and retries.
 * Otherwise (or once the queue is empty) returns 0 and leaves the queue untouched.
 *
 * @address 0x47a2c0
 */
uint8_t VehicleUpdateQueue::find_and_remove(int32_t target_tick, vehicle_update_record *out)
{
    vehicle_update_record *record;

    if (queue->read_index == queue->write_index) {
        return 0;
    }
    record = (vehicle_update_record *)queue->records[queue->read_index];

    if ((int32_t)record->tick == target_tick) {
        uint32_t *src = (uint32_t *)record;
        uint32_t *dst = (uint32_t *)out;
        int32_t i;
        for (i = 0; i < 0x12; i++) {
            dst[i] = src[i];
        }
        if (queue->read_index != queue->write_index) {
            queue->read_index = (queue->read_index + 1) % queue->capacity;
            return 1;
        }
        return 0;
    }

    {
        int32_t distance;
        int32_t tick = (int32_t)record->tick;
        if (tick < target_tick) {
            distance = (tick - target_tick) + 0x40;
        } else if (target_tick < tick) {
            distance = tick - target_tick;
        } else {
            distance = 0;
        }

        if ((int32_t)record->sequence < distance) {
            void *discarded;
            CircularQueue(queue).pop(&discarded);
            return VehicleUpdateQueue(queue).find_and_remove(target_tick, out);
        }
        return 0;
    }
}

/**
 * Implements the original `player_update_queue_create`.
 *
 * @address 0x479f40
 */
void PlayerUpdateQueue::create()
{
    uint32_t *storage;
    uint8_t *record_cursor;
    int32_t i;

    storage = (uint32_t *)GlobalAlloc(0, 0x14a0);
    queue->queue.storage = storage;
    for (i = 0; i < 0x528; i++) {
        storage[i] = 0;
    }

    record_cursor = (uint8_t *)queue->queue.storage;
    queue->queue.records = (void **)GlobalAlloc(0, 0x1e0);
    ((uint32_t *)queue->queue.records)[0] = 0;
    queue->queue.capacity = 0x78;
    queue->queue.record_size = 0x2c;
    queue->queue.read_index = 0;
    queue->queue.write_index = 0;

    i = 0;
    do {
        ((uint32_t *)queue->queue.records)[i / 4] = (uint32_t)record_cursor;
        i = i + 4;
        record_cursor = record_cursor + 0x2c;
    } while (i < 0x1e0);

    queue->has_current = 0;
}

/**
 * Implements the original `player_update_queue_pop_current`.
 *
 * @address 0x479fb0
 */
uint8_t PlayerUpdateQueue::pop_current(player_update_record *out)
{
    uint32_t *raw_out = (uint32_t *)out;
    uint32_t *record;
    uint8_t result;
    int32_t i;

    raw_out[0] = 0xffffffff;
    raw_out[1] = 0xffffffff;
    raw_out[2] = 0xffffffff;

    if (queue->queue.read_index == queue->queue.write_index) {
        return 0;
    }
    record = (uint32_t *)queue->queue.records[queue->queue.read_index];
    result = 1;

    record[1] = record[1] - 1;
    if (record[1] == 0) {
        if (queue->queue.read_index == queue->queue.write_index) {
            record = 0;
            result = 0;
        } else {
            record = (uint32_t *)queue->queue.records[queue->queue.read_index];
            queue->queue.read_index = (queue->queue.read_index + 1) % queue->queue.capacity;
            result = 1;
        }
    }

    for (i = 0; i < 11; i++) {
        raw_out[i] = record[i];
    }
    queue->has_current = 1;
    for (i = 0; i < 8; i++) {
        queue->current[i] = (int32_t)record[3 + i];
    }
    return result;
}

}  // namespace halo::game

extern "C" {

/**
 * C entry point for halo::game::CircularQueue::count; forwards to the C++ implementation.
 * register convention: the queue in EDX (in_EDX); no stack parameters.
 * // blam-cc: EDX -> queue
 *
 * @address 0x47a230
 */
int32_t circular_queue_count(circular_queue *queue)
{
    return halo::game::CircularQueue(queue).count();
}

/**
 * C entry point for halo::game::CircularQueue::pop; forwards to the C++ implementation.
 * register convention: the queue in ECX (in_ECX), an out-parameter for the popped record
 * pointer in EDX (in_EDX).
 * // blam-cc: ECX -> queue, EDX -> out_record
 *
 * @address 0x47a200
 */
uint8_t circular_queue_pop(circular_queue *queue, void **out_record)
{
    return halo::game::CircularQueue(queue).pop(out_record);
}

/**
 * C entry point for halo::game::CircularQueue::push; forwards to the C++ implementation.
 * register convention: the queue in EBX (unaff_EBX); the source record pointer is this
 * function's own recognized stack parameter.
 * // blam-cc: EBX -> queue, stack -> source
 *
 * @address 0x47a1a0
 */
uint8_t circular_queue_push(circular_queue *queue, void *source)
{
    return halo::game::CircularQueue(queue).push(source);
}

/**
 * C entry point for halo::game::CircularQueue::destroy; forwards to the C++ implementation.
 * register convention: the queue to tear down in ESI (unaff_ESI); no stack parameters.
 * // blam-cc: ESI -> queue
 *
 * @address 0x47a090
 */
void network_queue_destroy(circular_queue *queue)
{
    halo::game::CircularQueue(queue).destroy();
}

/**
 * C entry point for halo::game::PositionUpdateQueue::create; forwards to the C++ implementation.
 * register convention: the queue to initialize in ESI (unaff_ESI); no stack parameters.
 * // blam-cc: ESI -> queue
 *
 * @address 0x47a020
 */
void position_update_queue_create(circular_queue *queue)
{
    halo::game::PositionUpdateQueue(queue).create();
}

/**
 * C entry point for halo::game::PositionUpdateQueue::find_and_remove; forwards to the C++ implementation.
 * register convention: none -- all three are genuine stack parameters (Ghidra's own
 * param_1/param_2/param_3, unchanged across the recursive tail call).
 *
 * @address 0x47a100
 */
uint8_t position_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick, real_point3d *out)
{
    return halo::game::PositionUpdateQueue(queue).find_and_remove(target_tick, out);
}

/**
 * C entry point for halo::game::PositionUpdateQueue::push; forwards to the C++ implementation.
 * register convention: none recovered by Ghidra as a register argument, but the destination
 * queue is never loaded in this function's own body -- it relies on EBX already holding it,
 * exactly like circular_queue_push (this batch), to which it tail-forwards.
 * // blam-cc: EBX -> queue, stack -> tick, sequence, x, y, z
 * blam-cc: EBX -> queue, stack -> tick, sequence, x, y, z
 *
 * @address 0x47a0c0
 */
uint8_t position_update_queue_push(circular_queue *queue, real x, real y, real z, int32_t tick, int32_t sequence)
{
    return halo::game::PositionUpdateQueue(queue).push(x, y, z, tick, sequence);
}

/**
 * C entry point for halo::game::VehicleUpdateQueue::create; forwards to the C++ implementation.
 * register convention: the queue to initialize in ESI (unaff_ESI); no stack parameters.
 * // blam-cc: ESI -> queue
 *
 * @address 0x47a250
 */
void vehicle_update_queue_create(circular_queue *queue)
{
    halo::game::VehicleUpdateQueue(queue).create();
}

/**
 * C entry point for halo::game::VehicleUpdateQueue::find_and_remove; forwards to the C++ implementation.
 * register convention: none -- all three are genuine stack parameters (Ghidra's own
 * param_1/param_2/param_3, unchanged across the recursive tail call).
 *
 * @address 0x47a2c0
 */
uint8_t vehicle_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick, vehicle_update_record *out)
{
    return halo::game::VehicleUpdateQueue(queue).find_and_remove(target_tick, out);
}

/**
 * C entry point for halo::game::PlayerUpdateQueue::create; forwards to the C++ implementation.
 * register convention: the queue to initialize in ESI (unaff_ESI); no stack parameters.
 * // blam-cc: ESI -> queue
 *
 * @address 0x479f40
 */
void player_update_queue_create(player_update_queue *queue)
{
    halo::game::PlayerUpdateQueue(queue).create();
}

/**
 * C entry point for halo::game::PlayerUpdateQueue::pop_current; forwards to the C++ implementation.
 * register convention: an 11-dword output record in EAX (in_EAX); the queue in EBX (unaff_EBX).
 * // blam-cc: EAX -> out, EBX -> queue
 *
 * @address 0x479fb0
 */
uint8_t player_update_queue_pop_current(player_update_record *out, player_update_queue *queue)
{
    return halo::game::PlayerUpdateQueue(queue).pop_current(out);
}

}
