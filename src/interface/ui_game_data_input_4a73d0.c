// ui_game_data_input_4a73d0  (not a Ghidra function; game_data_input_function_table[42])
// address 0x4a73d0, size 47 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692bc0 (index 42); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a73d0.
// WRITTEN 2026-09-28 from objdump 0x4a73d0..0x4a73fe: counts the connected joysticks, which is only slot 0 here
//   (0x006b2ce8 != -1); two or more give scale 1.0, otherwise 0x3eaa7efa (about 1/3). So always 1/3 in retail.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t joystick_slot_devices[4]; // 0x006b2ce8, input.h

void ui_game_data_input_4a73d0(widget_instance *widget)
{
    int32_t count = joystick_slot_devices[0] != -1 ? 1 : 0;

    if (count >= 2) {
        widget->scale = 1.0f;
        return;
    }
    *(uint32_t *)&widget->scale = 0x3eaa7efa;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
