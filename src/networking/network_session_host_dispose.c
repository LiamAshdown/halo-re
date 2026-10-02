// network_session_host_dispose  (Ghidra: FUN_005778f0; named per this rewrite)
// address 0x5778f0, size 74 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary: "Tears down the network channel/session
// object created by network_session_host_start, if one exists."
// register convention: no register-passed arguments.
// UNSURE: gcd_shutdown/qr2_shutdown are foreign (GameSpy-shaped) calls whose argument lists
// Ghidra elided entirely.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *network_session_host_object; // 0x00722a20
extern int32_t network_session_host_state; // 0x00722a18, UNSURE
extern int32_t network_console_connection_id; // 0x0069fdfc

extern void network_session_host_update(void); // 0x577940, this module
extern void gcd_shutdown(void); // foreign, UNSURE
extern void qr2_shutdown(void *object); // foreign, UNSURE

// Tears down the network channel/session object created by network_session_host_start, if one
// exists.
void network_session_host_dispose(void)
{
    if (network_session_host_object != 0) {
        if (network_session_host_state != 2) {
            network_session_host_state = 2;
        }
        network_session_host_update();
        network_console_connection_id = -1;
        gcd_shutdown();
        qr2_shutdown(network_session_host_object);
        network_session_host_object = 0;
    }
}

#if 0
Original Ghidra decompilation (0x5778f0):

void FUN_005778f0(void)

{
  if (DAT_00722a20 != 0) {
    if (DAT_00722a18 != 2) {
      DAT_00722a18 = 2;
    }
    FUN_00577940();
    DAT_0069fdfc = 0xffffffff;
    FUN_0061b760();
    FUN_00616c40(DAT_00722a20);
    DAT_00722a20 = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
