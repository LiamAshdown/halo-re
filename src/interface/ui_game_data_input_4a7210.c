// ui_game_data_input_4a7210  (not a Ghidra function; game_data_input_function_table[34])
// address 0x4a7210, size 106 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692ba0 (index 34); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a7210.
// WRITTEN 2026-09-28 from objdump 0x4a7210..0x4a7279: with a server (+8) or client (+0xb14) game, grows the widget
//   text to 8 bytes and formats its int16 +0x1a0 with L"%d" (3 characters), terminated at [3].
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count

void ui_game_data_input_4a7210(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    uint16_t *text;

    if (game == 0) {
        return;
    }
    text = (uint16_t *)heap_reallocate(widget->text, 8, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        string_format_wide_va_bounded(3, text, (const uint16_t *)L"%d", (int32_t)*(int16_t *)(game + 0x1a0));
        text[3] = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
