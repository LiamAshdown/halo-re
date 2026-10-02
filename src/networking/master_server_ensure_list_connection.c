// master_server_ensure_list_connection  (Ghidra: FUN_004b66c0, still unnamed -> renamed)
// address 0x4b66c0, size 103 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("ensures the master-server list
// connection is established or being (re)connected, scheduling a retry if the connection
// attempt cannot start"); server_browser_query_elapsed_ms (0x007196c8) matches
// server_list_result_reset.c's own UNSURE-named global exactly (both set it to 9999 on an
// abandoned attempt); master_server_list_refresh_request is this module's own rewrite.
// register convention: __cdecl, no arguments.
// UNSURE: ServerBrowserState/ServerBrowserCount are foreign GameSpy library calls (connection-state query
// and connect-attempt trigger, respectively, going by their observed use); real names/full
// signatures not recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *master_server_query_engine; // 0x0071946c
extern uint32_t master_server_request_flags; // 0x0071969c
extern uint8_t DAT_00719488; // see master_server_list_refresh_request.c UNSURE
extern int32_t server_browser_query_elapsed_ms; // 0x007196c8

extern int32_t ServerBrowserState(void *engine); // foreign, GameSpy library; connection state
extern int32_t ServerBrowserCount(void *engine); // foreign, GameSpy library: result count // foreign, GameSpy library; start/poll connect attempt
extern void master_server_list_refresh_request(void); // 0x4b6660, this module

// blam-cc: __cdecl, no arguments
// If the master-server connection is already established or connecting (state 1 or 2),
// abandons any in-flight query (elapsed sentinel 9999) and clears the refresh flag. Otherwise,
// if a connect attempt cannot be started (result < 1), immediately requests a fresh refresh;
// if it can, marks the refresh flag and arms the "waiting on connection" request bit.
void master_server_ensure_list_connection(void)
{
    int32_t state;
    int32_t connect_result;

    if (master_server_query_engine != 0) {
        state = ServerBrowserState(master_server_query_engine);
        if (state != 2 && state != 1) {
            connect_result = ServerBrowserCount(master_server_query_engine);
            if (connect_result < 1) {
                master_server_list_refresh_request();
                return;
            }
            DAT_00719488 = 1;
            master_server_request_flags = master_server_request_flags | 0x10;
            return;
        }
        master_server_request_flags = master_server_request_flags | 4;
        server_browser_query_elapsed_ms = 9999;
        DAT_00719488 = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4b66c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004b66c0(void)

{
  int iVar1;

  if (DAT_0071946c != 0) {
    iVar1 = FUN_00616ff0(DAT_0071946c);
    if ((iVar1 != 2) && (iVar1 != 1)) {
      iVar1 = FUN_00617030(DAT_0071946c);
      if (iVar1 < 1) {
        master_server_list_refresh_request();
        return;
      }
      DAT_00719488 = 1;
      DAT_0071969c = DAT_0071969c | 0x10;
      return;
    }
    DAT_0071969c = DAT_0071969c | 4;
    _DAT_007196c8 = 9999;
    DAT_00719488 = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
