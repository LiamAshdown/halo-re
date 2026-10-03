#include "halo/game/gamerest_updates.hpp"
#include "halo/game/constants.hpp"
#include "halo/core/datum.hpp"
#include <stdint.h>
#include "halo/memory/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"

static auto &update_server_tick = halo::link::ref<int32_t>(halo::game::vars().update_server_tick);
static auto &update_server_history = halo::link::ref<uint32_t [32 * (0x308 / 4)]>(halo::game::vars().update_server_history);
static auto &update_server_queues = halo::link::ref<data_array *>(halo::game::vars().update_server_queues);

namespace halo::game {

/**
 * REWRITTEN (first-boot track, objdump 0x472cc0..0x472e9f): the queues iterated are update_server_queues
 *   (0x006f1d90; the old version iterated nothing and dereferenced NULL), and update_client_advance_read_cursor
 *   (EBX the tick, EDX the slot's count word) always runs at the end. Per tick: the ring slot (0x308 bytes, tick &
 *   0x1f, NULL when the tick counter overflowed) gets the tick and a zero count; then for every queue: when its
 *   ring has a record (read +0x38 != write +0x34; records at +0x30, capacity +0x28) the record's refcount (+4)
 *   drops, and at zero the record is popped (read advances modulo the capacity); either way its 11 dwords are the
 *   tick's input -- the queue's +0x44 (and +0x8) get the record's 8 input dwords (+0xc), +0x40 = 1 -- and the
 *   slot gets the 8 dwords at +8 + 0x20 * n and a summary {1, record +4 == 0, ?, ?; record +0, record +8,
 *   record +4} at +0x208 + 0x10 * n. A queue without a record copies its own +8 input and summary {0,...; -1}.
 *   The summary bytes/dwords the original leaves as stack garbage (bytes 2..3, and the last two dwords of an
 *   empty queue's summary) are written as 0 here.
 * blam-cc: none
 *
 * @address 0x472cc0
 */
void UpdateServer::push_player_tick_history()
{
    int32_t tick = update_server_tick;
    update_record *record;
    uint16_t *count;
    data_iterator iterator;
    update_server_queue *entry;

    update_server_tick = tick + 1;
    record = (tick < tick + 1 && tick >= (tick + 1) - 0x20)
        ? (update_record *)((uint8_t *)update_server_history + (tick & 0x1f) * sizeof(update_record)) : 0;
    record->tick = tick;
    count = &record->player_count;
    *count = 0;

    iterator.data = update_server_queues;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)update_server_queues ^ halo::game::k_iterator_signature_key;
    for (entry = (update_server_queue *)halo::memory::data_iterator_next(&iterator); entry != 0; entry = (update_server_queue *)halo::memory::data_iterator_next(&iterator)) {
        circular_queue *ring = &entry->queue.queue;
        int32_t read = ring->read_index;
        uint32_t *queued = 0;
        uint8_t have = 0;
        client_update_carry *summary = &record->carry[*count];
        int32_t i;

        if (read != ring->write_index) {
            queued = ((uint32_t **)ring->records)[read];
            have = 1;
            queued[1] -= 1;
            if (queued[1] == 0) {
                if (read != ring->write_index) {
                    queued = ((uint32_t **)ring->records)[read];
                    ring->read_index = (read + 1) % ring->capacity;
                } else {
                    queued = 0;
                    have = 0;
                }
            }
            if (have) {
                uint32_t local_record[11];

                for (i = 0; i < 11; i++) {
                    local_record[i] = queued[i];
                }
                memcpy(entry->queue.current, &local_record[3], sizeof(entry->queue.current));
                entry->queue.has_current = 1;
                memcpy(&entry->last_action, &local_record[3], sizeof(entry->last_action));
                memcpy(&record->actions[*count], &local_record[3], sizeof(player_action));
                summary->flag_a = 1;
                summary->flag_b = (local_record[1] == 0);
                summary->pad_02[0] = 0;
                summary->pad_02[1] = 0;
                summary->field1 = (int32_t)local_record[0];
                summary->field2 = (int32_t)local_record[2];
                summary->field3 = (int32_t)local_record[1];
                *count += 1;
                continue;
            }
        }
        memcpy(&record->actions[*count], &entry->last_action, sizeof(player_action));
        summary->flag_a = 0;
        summary->flag_b = 0;
        summary->pad_02[0] = 0;
        summary->pad_02[1] = 0;
        summary->field1 = (int32_t)halo::k_dword_none;
        summary->field2 = 0;
        summary->field3 = 0;
        *count += 1;
    }
    UpdateClient::advance_read_cursor(tick, (const uint32_t *)count);
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::UpdateServer::push_player_tick_history; forwards to the C++ implementation.
 * blam-cc: iterator in EDI
 * blam-cc: none
 *
 * @address 0x472cc0
 */
void update_server_push_player_tick_history(void)
{
    halo::game::UpdateServer::push_player_tick_history();
}

}
