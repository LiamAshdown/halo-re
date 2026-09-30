// ui_network_wait_timeout_check  (Ghidra: FUN_0049c7b0, unnamed; named per functions.md)
// address 0x49c7b0, size 86 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: functions.md: "Checks whether the elapsed time since a saved timestamp exceeds a
// threshold and, if so, raises a timeout flag used to abandon a pending network/UI wait state."
// types/interface.h already names all three globals touched here: "global 0x006927c4: float
// ui_saved_color[3]" is unrelated -- the real match is "global 0x006927c4: int32_t
// ui_network_wait_start_time  -1 when no wait is running" plus "0x00718fcc:
// ui_network_wait_timed_out" and "0x00718fcd: ui_network_wait_active".
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint8_t ui_network_wait_active;      // 0x00718fcd
extern int32_t ui_network_wait_start_time;  // 0x006927c4, -1 when no wait is running
extern uint8_t ui_network_wait_timed_out;   // 0x00718fcc

extern int32_t time_query_performance_counter_ms(void); // 0x449210, current time in milliseconds (QPC-based)

// If a network wait is not currently active, clears the start time and timeout flag. Otherwise,
// once 10 seconds have elapsed since the wait started, raises the timeout flag; either way the
// active flag is cleared before returning.
void ui_network_wait_timeout_check(void)
{
    if (ui_network_wait_active == 0) {
        ui_network_wait_start_time = -1;
        ui_network_wait_timed_out = 0;
    } else if (ui_network_wait_start_time != -1 && ui_network_wait_timed_out == 0) {
        int32_t now = time_query_performance_counter_ms();

        ui_network_wait_active = 0;
        if ((uint32_t)(now - ui_network_wait_start_time) > 9999) {
            ui_network_wait_timed_out = 1;
            return;
        }
    }
    ui_network_wait_active = 0;
}

#if 0
Original Ghidra decompilation (0x49c7b0):

void FUN_0049c7b0(void)

{
  int iVar1;

  if (DAT_00718fcd == '\0') {
    DAT_006927c4 = -1;
    DAT_00718fcc = '\0';
  }
  else if ((DAT_006927c4 != -1) && (DAT_00718fcc == '\0')) {
    iVar1 = FUN_00449210();
    DAT_00718fcd = 0;
    if (9999 < (uint)(iVar1 - DAT_006927c4)) {
      DAT_00718fcc = 1;
      return;
    }
  }
  DAT_00718fcd = 0;
  return;
}
#endif
