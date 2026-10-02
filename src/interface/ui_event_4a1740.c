// ui_event_4a1740  (not a Ghidra function; ui_event_function_table[92])
// address 0x4a1740, size 80 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692940 (index 92); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1740.
// WRITTEN 2026-09-28 from objdump 0x4a1740..0x4a178f: for a widget with no items (+0x48) and a client: a client
//   state word (+0xeda) of 1 reads the performance counter (result unused); a state of 0 returns the host session
//   start result (0x49d210, which ignores the three pushed arguments); otherwise 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)
extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern uint8_t multiplayer_host_session_start(void); // 0x49d210

uint8_t ui_event_4a1740(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t *state;

    if (widget->item_count != 0 || network_client == 0) {
        return 0;
    }
    state = (int16_t *)(network_client + 0xeda);
    if (*state == 1) {
        time_query_performance_counter_ms();
    }
    if (*state != 0) {
        return 0;
    }
    return multiplayer_host_session_start();
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
