// ui_game_data_input_4a6e90  (not a Ghidra function; game_data_input_function_table[30])
// address 0x4a6e90, size 105 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b90 (index 30); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6e90.
// WRITTEN 2026-09-28 from objdump 0x4a6e90..0x4a6ef8: with a server (+8) or client (+0xb14) game, grows the widget
//   text to 0x10 bytes and formats its dword +0x15c with L"%d" (0x006607a0, 7 characters), terminated at [7].
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

void ui_game_data_input_4a6e90(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    uint16_t *text;

    if (game == 0) {
        return;
    }
    text = (uint16_t *)heap_reallocate(widget->text, 0x10, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        string_format_wide_va_bounded(7, text, (const uint16_t *)L"%d", *(int32_t *)(game + 0x15c));
        text[7] = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
