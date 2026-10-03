#include "halo/game/gamerest_updates.hpp"
#include <string.h>
#include <stdint.h>
#include "halo/memory/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern int32_t update_client_unknown_ea0;
extern data_array *update_client_queues;
extern data_array *player_data;
extern uint32_t update_client_staged[8];
extern uint32_t update_client_unknown_ec8;
extern int32_t update_client_unknown_ec4;
extern int32_t update_client_base_tick;
extern uint8_t update_client_initialized;
extern update_record update_client_history[128];
extern int32_t update_client_write_cursor;
extern uint32_t update_client_unknown_ea8;
extern uint32_t update_client_unknown_eac;
extern data_array *update_server_queues;
extern uint8_t update_server_initialized;
extern int32_t update_server_tick;
extern update_record update_server_history[32];
extern datum_index machine_to_player[16];
extern game_time_globals *game_time;
extern int32_t update_client_unknown_102d4;
extern int32_t wait_tick_counter;
extern uint16_t local_player_name_filter[];
extern uint8_t position_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick, real_point3d *out);
extern void unit_snap_position_if_far(real_point3d *new_position, object *obj);
extern double sqrt(double x);
extern int32_t vehicle_wait_tick_counter;
}

namespace halo::game {

/**
 * REWRITTEN (objdump 0x4734b0..0x4734fb, 2026-09-24): EBX is the tick and EDX a pointer to the 0x304-byte
 *   update record, which is copied into the queue slot after its tick word (`mov esi,edx; rep movs`, 0xc1
 *   dwords). When the tick runs ahead of the cursor at 0x6f7ea0, every skipped tick's slot is fetched and the
 *   first word of THIS slot's record is set to 0xffff (the original writes [ebp], this slot, each time), and
 *   the cursor becomes the tick. The draft copied nothing, never stored the cursor, and passed 0 as the tick in
 *   the loop; hooked, the local player's actions never reached the queue (in game: could not move or shoot).
 *   update_client_queue_get_slot (0x473500) preserves EDX, which is why EDX survives the first call.
 * blam-cc: EBX -> target_tick, EDX -> record
 *
 * @address 0x4734b0
 */
void UpdateClient::advance_read_cursor(int32_t target_tick, const uint32_t *record)
{
    update_record *slot = UpdateClient::queue_get_slot(target_tick);
    int32_t tick;

    if (slot == 0) {
        return;
    }
    slot->tick = target_tick;
    memcpy((uint8_t *)slot + 4, record, 0xc1 * 4);
    if (target_tick > update_client_unknown_ea0) {
        for (tick = update_client_unknown_ea0 + 1; tick < target_tick; tick++) {
            UpdateClient::queue_get_slot(tick);
            ((struct update_record *)slot)->player_count = 0xffff;
        }
        update_client_unknown_ea0 = target_tick;
    }
}

/**
 * Resets update_client_queues to empty, then re-creates one zero-filled update_server_queue-sized
 * slot per currently live player, reusing that player's own datum index and salt so the two
 * arrays stay handle-compatible. See the header comment: despite the name, nothing is freed here.
 *
 * @address 0x472fa0
 */
void UpdateClient::dispose()
{
    data_iterator player_iter;
    void *player_element;

    update_client_queues->valid = 1;
    halo::memory::data_delete_all(update_client_queues);
    halo::memory::data_delete_all(update_client_queues);

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = halo::memory::data_iterator_next(&player_iter);
    while (player_element != 0) {
        datum_index player_handle = player_iter.index;
        int16_t index = (int16_t)player_handle;
        int16_t salt = (int16_t)((uint32_t)player_handle >> 16);

        if (index >= 0 && index < update_client_queues->maximum_count && salt != 0) {
            uint8_t *slot = (uint8_t *)update_client_queues->data +
                (int32_t)update_client_queues->size * (int32_t)index;

            if (*(int16_t *)slot == 0) {
                int32_t i;

                update_client_queues->actual_count = update_client_queues->actual_count + 1;
                if (index >= update_client_queues->last_index) {
                    update_client_queues->last_index = (int16_t)(index + 1);
                }
                for (i = 0; i < update_client_queues->size; i++) {
                    slot[i] = 0;
                }
                update_client_queues->next_identifier = update_client_queues->next_identifier + 1;
                if (update_client_queues->next_identifier == 0) {
                    update_client_queues->next_identifier = (int16_t)k_datum_identifier_wrap;
                }
                *(int16_t *)slot = salt;
            }
        }
        player_element = halo::memory::data_iterator_next(&player_iter);
    }
}

/**
 * UNSURE: see header. Copies the staged 8-dword entry into every element of `out` (0x20 bytes
 * each, one per iterated element), overwriting dword 0 with a masked value derived from
 * update_client_unknown_ec8, and decrements update_client_unknown_ec4 the first time through.
 * Advances update_client_base_tick and returns a packed (0, success) result.
 *
 * @address 0x473270
 */
uint32_t UpdateClient::distribute_staged_entry(uint8_t *out)
{
    uint32_t masked = ~update_client_unknown_ec8 & update_client_staged[0];
    data_iterator iter;
    void *element;
    int32_t index = -1;

    update_client_unknown_ec8 = update_client_staged[0] & 0x4d0;

    iter.data = 0;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    element = halo::memory::data_iterator_next(&iter);
    while (element != 0) {
        uint32_t *record;
        int32_t i;

        index = index + 1;
        record = (uint32_t *)(out + (int32_t)index * 0x20);
        for (i = 0; i < 8; i++) {
            record[i] = update_client_staged[i];
        }
        record[0] = masked;
        if (index == 0) {
            update_client_unknown_ec4 = update_client_unknown_ec4 - 1;
        }
        element = halo::memory::data_iterator_next(&iter);
    }

    update_client_base_tick = update_client_base_tick + 1;
    return 1;
}

/**
 * Zeroes the client update-queue globals, allocates the 16-entry update_client_queues array
 * (0x64-byte elements), and fills the 128-deep history ring with -1 bytes (so every record starts
 * "no tick"). Returns a nonzero success flag (Ghidra: 0xffffff01) on success, or the
 * (zero) initialized flag on failure.
 *
 * @address 0x472f40
 */
uint32_t UpdateClient::update_client_new()
{
    memset(&update_client_initialized, 0, 0x1843c);

    update_client_queues = halo::memory::data_new(0x28, (char *)"update client queues", 16);
    if (update_client_queues != 0) {
        memset(update_client_history, 0xff, sizeof(update_client_history));
        update_client_unknown_ea0 = -1;
        update_client_base_tick = 0;
        update_client_initialized = 1;
        return 0xffffff01;
    }
    return update_client_initialized;
}

/**
 * `out_actions` receives 0x20-byte records (types/game.h player_action) and
 * `out_carry` 0x10-byte ones (types/game.h client_update_carry), both per-player over the same
 * iteration; only the low byte of the return value is meaningful.
 *
 * @address 0x4730d0
 */
uint32_t UpdateClient::queue_apply_tick(player_action *out_actions, client_update_carry *out_carry)
{
    update_record *slot = UpdateClient::queue_get_slot(update_client_base_tick);

    if (slot == 0 || update_client_base_tick > (int32_t)update_client_unknown_ea0) {
        return 0;
    }

    {
        data_iterator player_iter;
        void *player_element;
        int16_t index = -1;
        uint8_t *slot_bytes = (uint8_t *)slot;

        player_iter.data = update_client_queues;
        player_iter.next_index = 0;
        player_iter.index = k_datum_index_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_element = halo::memory::data_iterator_next(&player_iter);
        while (player_element != 0) {
            index = index + 1;
            if (index < *(int16_t *)&((struct update_record *)slot_bytes)->player_count) {
                uint8_t *record = slot_bytes + 8 + (int32_t)index * 0x20;
                uint8_t *dst = (uint8_t *)player_element;

                *(uint32_t *)(dst + 4) = *(uint32_t *)(record + 0);
                *(uint32_t *)(dst + 0xc) = *(uint32_t *)(record + 4);
                *(uint32_t *)(dst + 0x10) = *(uint32_t *)(record + 8);
                *(uint32_t *)(dst + 0x14) = *(uint32_t *)(record + 0xc);
                *(uint32_t *)(dst + 0x18) = *(uint32_t *)(record + 0x10);
                *(uint32_t *)(dst + 0x1c) = *(uint32_t *)(record + 0x14);
                *(uint16_t *)(dst + 0x20) = *(uint16_t *)(record + 0x18);
                *(uint16_t *)(dst + 0x22) = *(uint16_t *)(record + 0x1a);
                *(uint16_t *)(dst + 0x24) = *(uint16_t *)(record + 0x1c);
            }
            player_element = halo::memory::data_iterator_next(&player_iter);
        }

        index = -1;
        player_iter.data = update_client_queues;
        player_iter.next_index = 0;
        player_iter.index = k_datum_index_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_element = halo::memory::data_iterator_next(&player_iter);
        while (player_element != 0) {
            uint8_t *dst = (uint8_t *)player_element;
            uint32_t *out_record = (uint32_t *)(((uint8_t *)out_actions) + (int32_t)(++index) * 0x20);
            uint32_t *carry_src = (uint32_t *)(slot_bytes + 0x208 + (int32_t)index * 0x10);
            uint32_t *carry_dst = (uint32_t *)(((uint8_t *)out_carry) + (int32_t)index * 0x10);
            int32_t k;

            out_record[0] = ~*(uint32_t *)(dst + 8) & *(uint32_t *)(dst + 4);
            *(uint32_t *)(dst + 8) = *(uint32_t *)(dst + 4) & 0x4d0;
            out_record[1] = *(uint32_t *)(dst + 0xc);
            out_record[2] = *(uint32_t *)(dst + 0x10);
            out_record[3] = *(uint32_t *)(dst + 0x14);
            out_record[4] = *(uint32_t *)(dst + 0x18);
            out_record[5] = *(uint32_t *)(dst + 0x1c);
            *(uint16_t *)(out_record + 6) = *(uint16_t *)(dst + 0x20);
            *(uint16_t *)((uint8_t *)out_record + 0x1a) = *(uint16_t *)(dst + 0x22);
            *(uint16_t *)(out_record + 7) = *(uint16_t *)(dst + 0x24);

            for (k = 0; k < 4; k++) {
                carry_dst[k] = carry_src[k];
            }
            player_element = halo::memory::data_iterator_next(&player_iter);
        }
    }

    update_client_base_tick = update_client_base_tick + 1;
    return 1;
}

/**
 * blam-cc: EAX -> tick
 * On a client or replay connection, allocates and returns the next write slot in the 128-deep
 * ring (advancing the write cursor). Otherwise (server or single-player), returns the slot for
 * `tick` if it falls within the current 128-tick window, or NULL if it does not.
 *
 * @address 0x473500
 */
update_record * UpdateClient::queue_get_slot(int32_t tick)
{
    if (halo::networking::globals().game_mode != 2 && halo::networking::globals().game_mode != 0) {
        int32_t slot = update_client_write_cursor & 0x7f;

        update_client_write_cursor = update_client_write_cursor + 1;
        return &update_client_history[slot];
    }
    if (update_client_base_tick <= tick && tick < update_client_base_tick + 0x80) {
        return &update_client_history[tick & 0x7f];
    }
    return 0;
}

/**
 * blam-cc: EAX -> source
 * Copies 8 dwords from `source`, overwrites dwords 1 and 2 with the two extra-state globals, and
 * stores the result into the staged client-update entry.
 *
 * @address 0x473090
 */
void UpdateClient::stage_entry(uint32_t *source)
{
    uint32_t staged[8];
    int32_t i;

    for (i = 0; i < 8; i++) {
        staged[i] = source[i];
    }
    staged[1] = update_client_unknown_ea8;
    staged[2] = update_client_unknown_eac;
    for (i = 0; i < 8; i++) {
        update_client_staged[i] = staged[i];
    }
}

/**
 * Resets update_server_queues to empty, re-creates one update_server_queue slot per currently
 * live player (reusing that player's own datum index and salt) and constructs its embedded
 * player_update_queue, then does the same for the client-side ring via update_client_dispose.
 * See the header comment: despite the name, nothing is freed here.
 *
 * @address 0x472b70
 */
void UpdateServer::dispose()
{
    data_iterator player_iter;
    void *player_element;

    update_server_queues->valid = 1;
    halo::memory::data_delete_all(update_server_queues);
    halo::memory::data_delete_all(update_server_queues);

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = halo::memory::data_iterator_next(&player_iter);
    while (player_element != 0) {
        datum_index player_handle = player_iter.index;
        int16_t index = (int16_t)player_handle;
        int16_t salt = (int16_t)((uint32_t)player_handle >> 16);

        if (index >= 0 && index < update_server_queues->maximum_count && salt != 0) {
            update_server_queue *slot = (update_server_queue *)((uint8_t *)update_server_queues->data +
                (int32_t)update_server_queues->size * (int32_t)index);

            if (slot->identifier == 0) {
                int32_t i;
                uint8_t *raw = (uint8_t *)slot;

                update_server_queues->actual_count = update_server_queues->actual_count + 1;
                if (index >= update_server_queues->last_index) {
                    update_server_queues->last_index = (int16_t)(index + 1);
                }
                for (i = 0; i < update_server_queues->size; i++) {
                    raw[i] = 0;
                }
                update_server_queues->next_identifier = update_server_queues->next_identifier + 1;
                if (update_server_queues->next_identifier == 0) {
                    update_server_queues->next_identifier = (int16_t)k_datum_identifier_wrap;
                }
                slot->identifier = salt;

                halo::game::player_update_queue_create(&slot->queue);
            }
        }
        player_element = halo::memory::data_iterator_next(&player_iter);
    }

    UpdateClient::dispose();
}

/**
 * Zeroes the server update-queue globals, allocates the 16-entry update_server_queues array
 * (0x64-byte elements), zeroes the 32-deep history ring, and initializes the client-side queue
 * too. Returns true (and marks itself initialized) only if both succeed.
 *
 * @address 0x472aa0
 */
uint8_t UpdateServer::update_server_new()
{
    memset(&update_server_initialized, 0, 0x610c);

    update_server_queues = halo::memory::data_new(0x64, (char *)"update server queues", 16);
    if (update_server_queues != 0) {
        memset(update_server_history, 0, sizeof(update_server_history));
        if (UpdateClient::update_client_new() != 0) {
            update_server_initialized = 1;
            return 1;
        }
    }
    return update_server_initialized;
}

/**
 * blam-cc: EAX -> requested_handle
 * Creates the update_server_queue datum at `requested_handle`'s index/salt and constructs its
 * embedded player_update_queue in place.
 *
 * @address 0x472c90
 */
void UpdateServer::queue_create_entry(datum_index requested_handle)
{
    datum_index handle = halo::memory::datum_new_at_index_with_salt(requested_handle, update_server_queues);
    update_server_queue *entry = (update_server_queue *)
        ((uint8_t *)update_server_queues->data + ((uint32_t)handle & 0xffff) * sizeof(update_server_queue));
    halo::game::player_update_queue_create(&entry->queue);
}

/**
 * blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record
 *
 * @address 0x472ea0
 */
void UpdateServer::queue_get_history_entry(int32_t *out_record, int32_t *out_tick, datum_index queue_handle)
{
    uint8_t counter_scratch[8];
    uint8_t *entry = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)counter_scratch);

    if (queue_handle != k_datum_index_none) {
        entry = (uint8_t *)update_server_queues->data + (uint32_t)(uint16_t)queue_handle * update_server_queues->size;
        if (update_server_tick <= *(int32_t *)(entry + 4)) {
            *out_tick = -1;
            return;
        }
        *out_tick = *(int32_t *)(entry + 4);
    }

    {
        int32_t value = *out_tick;

        if (value != -1) {
            if (value < update_server_tick && update_server_tick - 0x20 <= value &&
                ((uint32_t)value & 0x1f) * sizeof(update_record) != (uint32_t)(-0x6f1d94)) {
                update_record *record = &update_server_history[value & 0x1f];
                int32_t *src = (int32_t *)record + 1;
                int32_t i;

                for (i = 0; i < 0xc1; i++) {
                    out_record[i] = src[i];
                }
            }
            if (entry != 0) {
                *(int32_t *)(entry + 4) = *(int32_t *)(entry + 4) + 1;
            }
        }
    }
}

/**
 * Implements the original `update_server_queue_push_history`.
 *
 * @address 0x473390
 */
void UpdateServer::queue_push_history(int16_t machine_index, int32_t tick_count, uint32_t *source, uint32_t extra)
{
    datum_index player = machine_to_player[(uint16_t)machine_index];
    update_server_queue *entry;
    circular_queue *q;
    uint32_t record[11];
    int32_t used;
    int32_t i;

    if (player == k_datum_index_none) {
        return;
    }
    entry = (update_server_queue *)((uint8_t *)update_server_queues->data + (player & 0xffff) * 0x64);
    q = &entry->queue.queue;
    for (i = 0; i < 8; i++) {
        record[3 + i] = source[i];
    }
    record[0] = extra;
    record[1] = (uint32_t)tick_count;
    record[2] = (uint32_t)tick_count;
    if (q->write_index > q->read_index) {
        used = q->write_index - q->read_index;
    } else if (q->write_index < q->read_index) {
        used = q->capacity - q->read_index + q->write_index;
    } else {
        used = 0;
    }
    if (used < q->capacity - 1) {
        uint8_t *destination = (uint8_t *)q->records[q->write_index];
        uint8_t *from = (uint8_t *)record;

        for (i = 0; i < q->record_size; i++) {
            destination[i] = from[i];
        }
        q->write_index = (q->write_index + 1) % q->capacity;
    }
}

/**
 * Frees update_server_queues and update_client_queues (each preceded by a 14-dword zero pass
 * over their data_array headers, matching Ghidra literally) and resets every bookkeeping global.
 *
 * @address 0x472b00
 */
void UpdateQueues::dispose()
{
    if (update_server_queues != 0) {
        uint32_t *words = (uint32_t *)update_server_queues;
        int32_t i;

        for (i = 0; i < 14; i++) {
            words[i] = 0;
        }
        GlobalFree(update_server_queues);
        update_server_queues = 0;
    }
    update_server_initialized = 0;
    update_server_tick = 0;

    if (update_client_queues != 0) {
        uint32_t *words = (uint32_t *)update_client_queues;
        int32_t i;

        for (i = 0; i < 14; i++) {
            words[i] = 0;
        }
        GlobalFree(update_client_queues);
        update_client_queues = 0;
    }

    update_client_base_tick = 0;
    update_client_initialized = 0;
    update_client_unknown_ea0 = -1;
}

/**
 * Implements the original `update_queues_revert`.
 *
 * @address 0x472980
 */
void UpdateQueues::revert()
{
    if (update_server_initialized) {
        update_server_tick = 0;
        memset(update_server_history, 0, 0x1840 * 4);
    }
    if (update_client_initialized) {
        uint8_t *header = &update_client_initialized;
        int32_t now;
        int32_t tick;
        update_record *record;

        memset(update_client_history, 0xff, 0x6100 * 4);
        memset(header + 0x0c, 0, 0x20);
        *(int32_t *)(header + 0x30) = 0;
        update_client_base_tick = 0;
        *(int32_t *)(header + 0x2c) = -1;
        update_client_unknown_ea0 = -1;

        now = game_time->game_time;
        tick = now - 0x80;
        if (tick < 0) {
            tick = 0;
        }
        for (record = update_client_history; tick < now; tick++, record++) {
            record->tick = tick;
            record->player_count = 1;
            memset((uint8_t *)record + 8, 0, 0x80 * 4);
        }
        update_client_base_tick = now;
        update_server_tick = now;
        update_client_unknown_ea0 = now - 1;
    }
    if (update_server_initialized) {
        UpdateServer::dispose();
        *(int32_t *)((uint8_t *)update_server_queues->data + 4) = update_server_tick;
    } else if (update_client_initialized) {
        UpdateClient::dispose();
    }
}

/**
 * blam-cc: BX -> tick_count
 *
 * @address 0x473310
 */
void UpdateQueues::run_catchup_ticks(int16_t tick_count)
{
    int32_t tick;
    uint32_t staged_copy[8];
    int32_t record[0xc1];
    int32_t previous;
    int32_t next;
    uint32_t remaining;
    int32_t i;

    if (tick_count <= 0) {
        return;
    }
    previous = update_client_unknown_102d4;
    next = (previous + 1) & 0x8000003f;
    if (next < 0) {
        next = ((next - 1) | 0xffffffc0) + 1;
    }
    update_client_unknown_102d4 = next;
    for (i = 0; i < 8; i++) {
        staged_copy[i] = update_client_staged[i];
    }
    UpdateServer::queue_push_history(0, tick_count, staged_copy, (uint32_t)previous);

    remaining = (uint16_t)tick_count;
    do {
        UpdateServer::push_player_tick_history();
        UpdateServer::queue_get_history_entry(record, &tick, 0);
        remaining = remaining - 1;
    } while (remaining != 0);
}

/**
 * Tries to pop the queued position update matching unit_obj's own recorded network tick
 * (unit_obj+0x4bc). On success, logs the distance between the queued and current position and
 * how long it waited, resets the wait counter if this is the "filtered" player's own name, then
 * bumps the queue's running wait-count/distance totals; if the unit is unparented and its
 * network_role is 1, snaps its position (unit_snap_position_if_far). On failure -- if the head
 * of the queue is not simply "not yet due" -- logs a "can't update" message with whatever record
 * is now at the head, and bumps the wait counter for the filtered player's own name.
 *
 * @address 0x477350
 */
void PlayerNetworkState::apply_remote_position_update(object *unit_obj)
{
    real_point3d queued;
    uint8_t found;
    int32_t target_tick = *(int32_t *)((uint8_t *)unit_obj + 0x4bc);

    found = halo::game::position_update_queue_find_and_remove(&plr->position_updates, target_tick, &queued);
    if (found == 1) {
        float dx = queued.x - unit_obj->position.x;
        float dy = queued.y - unit_obj->position.y;
        float dz = queued.z - unit_obj->position.z;
        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

        halo::networking::player_update_history_log_printf_filtered(plr, 1, "Waited [%d], dist [%f].", wait_tick_counter, (double)dist);
        if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
            wait_tick_counter = 0;
        }
        plr->position_updates_applied_count = plr->position_updates_applied_count + 1;
        *(float *)&plr->position_update_error_total = dist + *(float *)&plr->position_update_error_total;

        if (unit_obj->parent_object == (datum_index)-1 && unit_obj->network_role == 1) {
            halo::game::unit_snap_position_if_far(&queued, unit_obj);
        }
    } else {
        circular_queue *queue = &plr->position_updates;
        int32_t write_index = queue->write_index;
        int32_t read_index = queue->read_index;
        int32_t distance;

        if (read_index < write_index) {
            distance = -read_index;
        } else {
            if (read_index <= write_index) {
                return;
            }
            distance = queue->capacity - read_index;
        }

        if (write_index + distance > 0) {
            int32_t *head_record = *(int32_t **)((uint8_t *)queue->records + read_index * 4);
            halo::networking::player_update_history_log_printf_filtered(plr, 1, "Can't update pos: [%d] != [%d]",
                                                        target_tick, *head_record);
            if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
                wait_tick_counter = wait_tick_counter + 1;
            }
        }
    }
}

/**
 * Tries to pop the queued vehicle update matching unit_obj's own recorded network tick
 * (unit_obj+0x4bc). On success, if the record's parent_or_tag matches unit_obj->parent_object
 * and that object can be fetched, logs the distance moved, bumps the running wait-count/distance
 * totals, calls unit_propagate_position_delta_to_children, and overwrites the parent object's velocity, angular_velocity,
 * forward and up vectors from the record. Either way, resets the wait counter if this is the
 * filtered player's own name. On failure, if the head of the queue is not simply "not yet due"
 * and the queue is not empty, logs a "can't update" message and bumps the wait counter.
 *
 * @address 0x477490
 */
void PlayerNetworkState::apply_remote_vehicle_position_update(object *unit_obj)
{
    vehicle_update_record record;
    int32_t target_tick = *(int32_t *)((uint8_t *)unit_obj + 0x4bc);
    uint8_t found = halo::game::vehicle_update_queue_find_and_remove(&plr->vehicle_updates, target_tick, &record);

    if (found == 1) {
        if (unit_obj->parent_object == (datum_index)record.body.parent_or_tag) {
            object *parent_obj = halo::objects::object_try_and_get((datum_index)record.body.parent_or_tag, 0xffffffff);
            if (parent_obj != (object *)0) {
                float dx = record.body.position.x - parent_obj->position.x;
                float dy = record.body.position.y - parent_obj->position.y;
                float dz = record.body.position.z - parent_obj->position.z;
                float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

                halo::networking::player_update_history_log_printf_filtered(plr, 1, "Vehicle waited [%d], dist [%f].",
                                                            vehicle_wait_tick_counter, (double)dist);

                *(float *)&plr->vehicle_update_error_total = dist + *(float *)&plr->vehicle_update_error_total;
                plr->vehicle_updates_applied_count = plr->vehicle_updates_applied_count + 1;
                halo::units::unit_propagate_position_delta_to_children(&parent_obj->position, (uint32_t)record.body.parent_or_tag);
                parent_obj->velocity = record.body.velocity;
                parent_obj->angular_velocity = record.body.angular_velocity;
                parent_obj->forward = record.body.forward;
                parent_obj->up = record.body.up;
            }
        }
        if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
            vehicle_wait_tick_counter = 0;
        }
    } else {
        circular_queue *queue = &plr->vehicle_updates;
        int32_t write_index = queue->write_index;
        int32_t read_index = queue->read_index;
        int32_t distance;

        if (read_index < write_index) {
            distance = write_index - read_index;
        } else {
            if (read_index <= write_index) {
                return;
            }
            distance = (write_index - read_index) + queue->capacity;
        }

        if (distance > 0 && read_index != write_index) {
            int32_t *head_record = *(int32_t **)((uint8_t *)queue->records + read_index * 4);
            halo::networking::player_update_history_log_printf_filtered(plr, 1, "Can't update pos: [%d] != [%d]",
                                                        target_tick, *head_record);
            if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
                vehicle_wait_tick_counter = vehicle_wait_tick_counter + 1;
            }
        }
    }
}

/**
 * Only while this machine is a network client, and only for a non-local player with a unit:
 * revalidates the unit (_object_mask_unit) and stamps field0 into its +0x4bc (see
 * game_engine_server_update_player_positions.c, this batch, for the producer side). Then, based
 * on whether player_unit_has_parent says the unit's controlling player currently has a parent object (e.g.
 * boarding or seated), applies either a smooth remote-player position update or the vehicle
 * variant.
 *
 * @address 0x476cf0
 */
void PlayerNetworkState::apply_first_position_update(uint32_t field0)
{
    object *unit_obj;

    if (halo::networking::globals().game_mode != 1 || plr->local_player_index != -1 || plr->unit == (datum_index)-1) {
        return;
    }

    unit_obj = halo::objects::object_try_and_get(plr->unit, _object_mask_unit);
    if (unit_obj == (object *)0) {
        return;
    }

    {
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        uint8_t seated = halo::game::player_unit_has_parent(unit->controlling_player);
        *(uint32_t *)((uint8_t *)unit_obj + 0x4bc) = field0;
        if (seated == 0) {
            PlayerNetworkState(plr).apply_remote_position_update(unit_obj);
        } else {
            PlayerNetworkState(plr).apply_remote_vehicle_position_update(unit_obj);
        }
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::UpdateClient::advance_read_cursor; forwards to the C++ implementation.
 * blam-cc: EBX -> target_tick, EDX -> record
 * blam-cc: EBX -> target_tick, EDX -> record
 *
 * @address 0x4734b0
 */
void update_client_advance_read_cursor(int32_t target_tick, const uint32_t *record)
{
    halo::game::UpdateClient::advance_read_cursor(target_tick, record);
}

/**
 * C entry point for halo::game::UpdateClient::dispose; forwards to the C++ implementation.
 * register convention: no visible parameters; unaff_ESI/EDI patterns are this function's own
 * blam-cc: array in ESI
 * blam-cc: iterator in EDI
 *
 * @address 0x472fa0
 */
void update_client_dispose(void)
{
    halo::game::UpdateClient::dispose();
}

/**
 * C entry point for halo::game::UpdateClient::distribute_staged_entry; forwards to the C++ implementation.
 * blam-cc: iterator in EDI
 *
 * @address 0x473270
 */
uint32_t update_client_distribute_staged_entry(uint8_t *out)
{
    return halo::game::UpdateClient::distribute_staged_entry(out);
}

/**
 * C entry point for halo::game::UpdateClient::update_client_new; forwards to the C++ implementation.
 * blam-cc: EBX -> element_size
 *
 * @address 0x472f40
 */


/**
 * C entry point for halo::game::UpdateClient::queue_apply_tick; forwards to the C++ implementation.
 * blam-cc: iterator in EDI
 *
 * @address 0x4730d0
 */
uint32_t update_client_queue_apply_tick(player_action *out_actions, client_update_carry *out_carry)
{
    return halo::game::UpdateClient::queue_apply_tick(out_actions, out_carry);
}

/**
 * C entry point for halo::game::UpdateClient::queue_get_slot; forwards to the C++ implementation.
 * register convention: a tick value in EAX (Ghidra's `in_EAX`), used only on the
 * blam-cc: EAX -> tick
 * blam-cc: EAX -> tick
 *
 * @address 0x473500
 */


/**
 * C entry point for halo::game::UpdateClient::stage_entry; forwards to the C++ implementation.
 * register convention: source data pointer in EAX (Ghidra's `in_EAX`).
 * // blam-cc: EAX -> source
 * blam-cc: EAX -> source
 *
 * @address 0x473090
 */
void update_client_stage_entry(uint32_t *source)
{
    halo::game::UpdateClient::stage_entry(source);
}

/**
 * C entry point for halo::game::UpdateServer::dispose; forwards to the C++ implementation.
 * register convention: no visible parameters.
 * blam-cc: array in ESI
 * blam-cc: iterator in EDI
 * blam-cc: ESI -> queue
 *
 * @address 0x472b70
 */
void update_server_dispose(void)
{
    halo::game::UpdateServer::dispose();
}

/**
 * C entry point for halo::game::UpdateServer::update_server_new; forwards to the C++ implementation.
 * blam-cc: EBX -> element_size
 *
 * @address 0x472aa0
 */
uint8_t update_server_new(void)
{
    return halo::game::UpdateServer::update_server_new();
}

/**
 * C entry point for halo::game::UpdateServer::queue_create_entry; forwards to the C++ implementation.
 * register convention: EAX -> requested_handle.
 * // blam-cc: EAX -> requested_handle
 * blam-cc: EAX -> handle, EDX -> array
 * blam-cc: ESI -> queue
 * blam-cc: EAX -> requested_handle
 *
 * @address 0x472c90
 */
void update_server_queue_create_entry(datum_index requested_handle)
{
    halo::game::UpdateServer::queue_create_entry(requested_handle);
}

/**
 * C entry point for halo::game::UpdateServer::queue_get_history_entry; forwards to the C++ implementation.
 * register convention: a queue handle in ECX (Ghidra's `in_ECX`), a second output pointer in EAX
 * blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record
 * blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record
 *
 * @address 0x472ea0
 */


/**
 * C entry point for halo::game::UpdateServer::queue_push_history; forwards to the C++ implementation.
 * register convention: machine index in AX, tick count in EDX, source and extra on the stack.
 * // blam-cc: EAX -> machine_index, EDX -> tick_count, stack -> source, extra
 *
 * @address 0x473390
 */
void update_server_queue_push_history(int16_t machine_index, int32_t tick_count, uint32_t *source, uint32_t extra)
{
    halo::game::UpdateServer::queue_push_history(machine_index, tick_count, source, extra);
}

/**
 * C entry point for halo::game::UpdateQueues::dispose; forwards to the C++ implementation.
 *
 * @address 0x472b00
 */
void update_queues_dispose(void)
{
    halo::game::UpdateQueues::dispose();
}

/**
 * C entry point for halo::game::UpdateQueues::revert; forwards to the C++ implementation.
 * blam-cc: no arguments
 *
 * @address 0x472980
 */
void update_queues_revert(void)
{
    halo::game::UpdateQueues::revert();
}

/**
 * C entry point for halo::game::UpdateQueues::run_catchup_ticks; forwards to the C++ implementation.
 * register convention: a tick count in BX (Ghidra's `unaff_BX`).
 * // blam-cc: BX -> tick_count
 * blam-cc: EAX -> machine_index, EDX -> tick_count, stack -> source, extra
 * blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record
 * blam-cc: BX -> tick_count
 *
 * @address 0x473310
 */
void update_run_catchup_ticks(int16_t tick_count)
{
    halo::game::UpdateQueues::run_catchup_ticks(tick_count);
}

/**
 * C entry point for halo::game::PlayerNetworkState::apply_remote_position_update; forwards to the C++ implementation.
 * register convention: EAX -> plr, EBX -> unit_obj.
 * // blam-cc: EAX -> plr, EBX -> unit_obj
 * blam-cc: EAX -> plr, EBX -> unit_obj
 *
 * @address 0x477350
 */
void apply_remote_player_position_update(player *plr, object *unit_obj)
{
    halo::game::PlayerNetworkState(plr).apply_remote_position_update(unit_obj);
}

/**
 * C entry point for halo::game::PlayerNetworkState::apply_remote_vehicle_position_update; forwards to the C++ implementation.
 * register convention: EAX -> plr, EBX -> unit_obj (matching apply_remote_player_position_update,
 * this batch; not independently re-disassembled here given the strong structural parallel).
 * // blam-cc: EAX -> plr, EBX -> unit_obj
 * blam-cc: EAX -> plr, EBX -> unit_obj
 *
 * @address 0x477490
 */
void apply_remote_player_vehicle_position_update(player *plr, object *unit_obj)
{
    halo::game::PlayerNetworkState(plr).apply_remote_vehicle_position_update(unit_obj);
}

/**
 * C entry point for halo::game::PlayerNetworkState::apply_first_position_update; forwards to the C++ implementation.
 * register convention: ESI -> plr, EDI -> field0.
 * // blam-cc: ESI -> plr, EDI -> field0
 * blam-cc: ECX -> player_handle
 * blam-cc: EAX -> plr, EBX -> unit_obj
 * blam-cc: EAX -> plr, EBX -> unit_obj
 * blam-cc: ESI -> plr, EDI -> field0
 *
 * @address 0x476cf0
 */
void player_apply_first_position_update(uint32_t field0, player *plr)
{
    halo::game::PlayerNetworkState(plr).apply_first_position_update(field0);
}

}
