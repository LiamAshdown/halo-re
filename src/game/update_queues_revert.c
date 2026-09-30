// update_queues_revert  (not a Ghidra function; the static value of game_state_revert_proc)
// address 0x472980, size 277 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: game_state_revert_proc (0x0069e7b0) holds 0x472980 in the image; game_state_load_checkpoint
//   0x538280 calls it before reading the saved state. Touches only the update server/client globals of
//   update_server_new 0x472c20 / update_client_new 0x472f40.
// objdump 0x472980..0x472a95:
//   - server initialized (0x006f1d88): tick (0x006f1d8c) = 0, the 32 record history (0x006f1d94) zeroed;
//   - client initialized (0x006f7e98): the 128 record history (0x006f7ed4) filled with 0xff, the header
//     dwords 0x006f7ea4..0x006f7ec0, 0x006f7ec8 and the base tick (0x006f7e9c) zeroed, 0x006f7ec4 and
//     0x006f7ea0 = -1; then for the ticks max(now - 0x80, 0) .. now - 1 (now = game_time +0x0c), the
//     consecutive records from the first get tick = that tick, player count 1 and +0x08..+0x207 zeroed;
//     the base tick and the server tick become now, 0x006f7ea0 now - 1;
//   - then with the server initialized update_server_dispose 0x472b70 runs and the first server queue's
//     +0x04 gets the server tick; otherwise with the client initialized it tail-jumps to
//     update_client_dispose 0x472fa0.
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <string.h>

extern game_time_globals *game_time;              // 0x006f1d6c
extern uint8_t update_server_initialized;         // 0x006f1d88
extern int32_t update_server_tick;                // 0x006f1d8c
extern data_array *update_server_queues;          // 0x006f1d90
extern update_record update_server_history[32];   // 0x006f1d94
extern uint8_t update_client_initialized;         // 0x006f7e98
extern int32_t update_client_base_tick;           // 0x006f7e9c
extern int32_t update_client_unknown_ea0;         // 0x006f7ea0
extern update_record update_client_history[128];  // 0x006f7ed4


void update_queues_revert(void)
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
        memset(header + 0x0c, 0, 0x20);           // 0x006f7ea4..0x006f7ec3
        *(int32_t *)(header + 0x30) = 0;          // 0x006f7ec8
        update_client_base_tick = 0;
        *(int32_t *)(header + 0x2c) = -1;         // 0x006f7ec4
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
        update_server_dispose();
        *(int32_t *)((uint8_t *)update_server_queues->data + 4) = update_server_tick;
    } else if (update_client_initialized) {
        update_client_dispose();
    }
}
