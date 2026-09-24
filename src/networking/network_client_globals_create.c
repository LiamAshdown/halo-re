// network_client_globals_create  (Ghidra: FUN_004dde50; named per this rewrite)
// address 0x4dde50, size 31 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md: "Allocates the primary network-game globals
// structure (DAT_0071c2d8) via network_session_create and, if successful, clears the DAT_0071c2de flag."
// network_client (0x0071c2d8), network_session_create (0x4d8a80, already rewritten) and
// network_host_handoff_requested (0x0071c2de) all match established names in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_host_handoff_requested;  // 0x0071c2de

extern network_client_globals *network_session_create(void); // 0x4d8a80, outside this batch

int32_t network_client_globals_create(void)
{
    network_client = network_session_create();
    if (network_client != 0) {
        network_host_handoff_requested = 0;
    }
    return network_client != 0;
}

#if 0
Original Ghidra decompilation (0x4dde50):

bool FUN_004dde50(void)

{
  DAT_0071c2d8 = network_session_create();
  if (DAT_0071c2d8 != 0) {
    DAT_0071c2de = 0;
  }
  return DAT_0071c2d8 != 0;
}
#endif
