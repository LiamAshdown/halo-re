// network_session_host_update  (Ghidra: FUN_00577940; named per this rewrite)
// address 0x577940, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary: "Periodic per-frame update for the
// network channel/session object: flushes it on timeout or close request, then pumps it."
// register convention: no register-passed arguments.
// UNSURE: FUN_00449210 (a tick/timer read), FUN_00616c00 and FUN_00616cb0 (foreign,
// GameSpy-shaped, argument lists elided).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *network_session_host_object;   // 0x00722a20
extern int32_t network_session_host_state;  // 0x00722a18
extern uint8_t network_session_host_closing; // 0x00722a1c, UNSURE
extern int32_t network_session_host_last_tick; // 0x00722a24, UNSURE

extern int32_t time_query_performance_counter_ms(void); // foreign/other module, UNSURE: a tick counter read
extern void FUN_00616c00(void *object); // foreign, UNSURE
extern void FUN_00616cb0(void *object); // foreign, UNSURE

// Periodic per-frame update for the network channel/session object: flushes it on timeout (1000+
// ticks since the last flush) or on a pending close request, then pumps it either way.
void network_session_host_update(void)
{
    if (network_session_host_object != 0) {
        if (network_session_host_state != 0) {
            int32_t now = time_query_performance_counter_ms();
            if (network_session_host_state == 2 || (uint32_t)(now - network_session_host_last_tick) > 999) {
                network_session_host_closing = (network_session_host_state == 2);
                FUN_00616c00(network_session_host_object);
                network_session_host_state = 0;
                network_session_host_last_tick = now;
            }
        }
        FUN_00616cb0(network_session_host_object);
        network_session_host_closing = 0;
    }
}

#if 0
Original Ghidra decompilation (0x577940):

void FUN_00577940(void)

{
  int iVar1;

  if (DAT_00722a20 != 0) {
    if ((DAT_00722a18 != 0) &&
       ((iVar1 = FUN_00449210(), DAT_00722a18 == 2 || (999 < (uint)(iVar1 - DAT_00722a24))))) {
      DAT_00722a1c = DAT_00722a18 == 2;
      FUN_00616c00(DAT_00722a20);
      DAT_00722a18 = 0;
      DAT_00722a24 = iVar1;
    }
    FUN_00616cb0(DAT_00722a20);
    DAT_00722a1c = 0;
  }
  return;
}
#endif
