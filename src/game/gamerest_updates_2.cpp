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
 * UNSURE: see header.
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
    uint8_t *slot;
    uint16_t *count;
    data_iterator iterator;
    uint8_t *queue;

    update_server_tick = tick + 1;
    slot = (tick < tick + 1 && tick >= (tick + 1) - 0x20)
        ? (uint8_t *)update_server_history + (tick & 0x1f) * 0x308 : 0;
    *(int32_t *)slot = tick;
    count = (uint16_t *)(slot + 4);
    *count = 0;

    iterator.data = update_server_queues;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)update_server_queues ^ halo::game::k_iterator_signature_key;
    for (queue = (uint8_t *)halo::memory::data_iterator_next(&iterator); queue != 0; queue = (uint8_t *)halo::memory::data_iterator_next(&iterator)) {
        int32_t read = *(int32_t *)(queue + 0x38);
        uint32_t *record = 0;
        uint8_t have = 0;
        uint32_t *summary = (uint32_t *)(slot + 0x208 + *count * 0x10);
        int32_t i;

        if (read != *(int32_t *)(queue + 0x34)) {
            record = ((uint32_t **)*(uint32_t *)(queue + 0x30))[read];
            have = 1;
            record[1] -= 1;
            if (record[1] == 0) {
                if (read != *(int32_t *)(queue + 0x34)) {
                    record = ((uint32_t **)*(uint32_t *)(queue + 0x30))[read];
                    *(int32_t *)(queue + 0x38) = (read + 1) % *(int32_t *)(queue + 0x28);
                } else {
                    record = 0;
                    have = 0;
                }
            }
            if (have) {
                uint32_t local_record[11];

                for (i = 0; i < 11; i++) {
                    local_record[i] = record[i];
                }
                for (i = 0; i < 8; i++) {
                    ((uint32_t *)(queue + 0x44))[i] = local_record[3 + i];
                }
                queue[0x40] = 1;
                for (i = 0; i < 8; i++) {
                    ((uint32_t *)(queue + 8))[i] = local_record[3 + i];
                    ((uint32_t *)(slot + 8 + *count * 0x20))[i] = local_record[3 + i];
                }
                summary[0] = 1u | (uint32_t)(local_record[1] == 0) << 8;
                summary[1] = local_record[0];
                summary[2] = local_record[2];
                summary[3] = local_record[1];
                *count += 1;
                continue;
            }
        }
        for (i = 0; i < 8; i++) {
            ((uint32_t *)(slot + 8 + *count * 0x20))[i] = ((uint32_t *)(queue + 8))[i];
        }
        summary[0] = 0;
        summary[1] = halo::k_dword_none;
        summary[2] = 0;
        summary[3] = 0;
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
