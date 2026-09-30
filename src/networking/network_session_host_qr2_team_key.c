// network_session_host_qr2_team_key  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x577f40, size 110 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x577f40..0x577fad: the team key callback (key, index, buffer, user data): the
//   game engine's +0xa4 hook answers first; key 0x1c is the team name ("Red" for 0, "Blue" for 1 -- as the binary has
//   it, index 1 is "Blue"); anything else is empty.
// blam-cc: cdecl (a qr2 team key callback)

#include "tags.h"
#include "fn_networking.h"
#include <string.h>
#include <wchar.h>

extern void *current_game_engine; // 0x006f1d20 (game_engine_definition *; +0x9c/+0xa0/+0xa4/+0xa8 the query hooks)
extern void qr2_buffer_add(void *buffer, const char *value); // 0x615590 qr2_buffer_add
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640 qr2_buffer_add_int
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id); // 0x615560 qr2_keybuffer_add

void network_session_host_qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    (void)user_data;
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, int32_t, void *) = *(uint8_t (**)(int32_t, int32_t, void *))((uint8_t *)current_game_engine + 0xa4);

        if (hook != 0 && hook(key_id, index, buffer) != 0) {
            return;
        }
    }
    if (key_id == 0x1c) {
        qr2_buffer_add(buffer, index == 1 ? "Blue" : "Red");
        return;
    }
    qr2_buffer_add(buffer, "");
}
