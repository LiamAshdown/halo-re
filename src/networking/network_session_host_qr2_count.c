// network_session_host_qr2_count  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x5780c0, size 88 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5780c0..0x578117: the player / team count callback (key type, user data): 0
//   outside a game; the game engine's +0xa8 hook when it has one; otherwise players -> the active player count, teams
//   -> 2 with teams (else 0), anything else 0.
// blam-cc: cdecl (a qr2 count callback)

#include "tags.h"
#include <string.h>
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *current_game_engine; // 0x006f1d20 (game_engine_definition *; +0x9c/+0xa0/+0xa4/+0xa8 the query hooks)
extern void qr2_buffer_add(void *buffer, const char *value); // 0x615590 qr2_buffer_add
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640 qr2_buffer_add_int
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id); // 0x615560 qr2_keybuffer_add
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t players_active_count(void); // 0x45c6a0

int32_t network_session_host_qr2_count(int32_t key_type, void *user_data)
{
    int32_t (*hook)(int32_t);

    (void)user_data;
    if (current_game_engine == 0) {
        return 0;
    }
    hook = *(int32_t (**)(int32_t))((uint8_t *)current_game_engine + 0xa8);
    if (hook != 0) {
        return hook(key_type);
    }
    if (key_type == 1) {
        return players_active_count();
    }
    if (key_type == 2 && game_engine_teams_enabled_flag != 0) {
        return 2;
    }
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
