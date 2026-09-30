// network_session_host_qr2_key_list  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x577fb0, size 261 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x577fb0..0x5780b4: the key list callback (key type, key buffer, user data):
//   server keys 1 3 4 10 19 5 0x33 11 and, in a game, 0x36 8 6 12 7 13 0x34 0x35; in a game, player keys 0x15 0x16
//   0x18 0x19 and team keys 0x1c 0x1d.
// blam-cc: cdecl (a qr2 key list callback)

#include "tags.h"
#include <string.h>
#include <wchar.h>
#include "math.h"
#include "memory.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_networking.h"

extern game_engine_definition *current_game_engine;
extern void qr2_buffer_add(void *buffer, const char *value); // 0x615590 qr2_buffer_add
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640 qr2_buffer_add_int
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id); // 0x615560 qr2_keybuffer_add

void network_session_host_qr2_key_list(int32_t key_type, void *keybuffer, void *user_data)
{
    int32_t i;

    (void)user_data;
    if (key_type == 0) {
        static const int32_t always[8] = { 1, 3, 4, 10, 19, 5, 0x33, 11 };
        static const int32_t in_game[8] = { 0x36, 8, 6, 12, 7, 13, 0x34, 0x35 };

        for (i = 0; i < 8; i++) {
            qr2_keybuffer_add(keybuffer, always[i]);
        }
        if (current_game_engine != 0) {
            for (i = 0; i < 8; i++) {
                qr2_keybuffer_add(keybuffer, in_game[i]);
            }
        }
    } else if (key_type == 1) {
        if (current_game_engine != 0) {
            qr2_keybuffer_add(keybuffer, 0x15);
            qr2_keybuffer_add(keybuffer, 0x16);
            qr2_keybuffer_add(keybuffer, 0x18);
            qr2_keybuffer_add(keybuffer, 0x19);
        }
    } else if (key_type == 2) {
        if (current_game_engine != 0) {
            qr2_keybuffer_add(keybuffer, 0x1c);
            qr2_keybuffer_add(keybuffer, 0x1d);
        }
    }
}
