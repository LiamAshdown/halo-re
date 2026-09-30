// multiplayer_host_session_start  (Ghidra: multiplayer_host_session_start, already named)
// address 0x49d210, size 201 bytes, callers=0 in this build
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: matches the given name; functions.md: "Establishes a hosted multiplayer session by
// creating the host game engine, applying the active game variant, and opening the network
// session, cleaning up any partial state if a step fails." Reuses network_server/network_client
// (types/networking.h) and the already-established network_game_server_host_create/_dispose,
// game_engine_apply_current_custom_variant and network_session_create signatures.
// register convention: none (void).
// UNSURE: the four dword writes into the freshly created host record
// (DAT_0071c2d4[0x272..0x275] and the byte at +0x9d5) touch fields past
// types/networking.h's own documented offsets for network_server_globals; preserved as raw index
// writes rather than named fields. game_engine_sync_variant_defaults, FUN_00463b20 and network_client_globals_dispose are foreign with
// unresolved signatures (kept void/no-arg per existing UNSURE precedent elsewhere in the module).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_game.h"
#include "fn_networking.h"
#include "fn_interface.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, TYPES-GAP
extern int32_t game_variant_history_current;      // 0x00687b18, TYPES-GAP
extern int16_t network_game_mode;                  // 0x00719720
extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_host_handoff_requested;                    // 0x0071c2de
extern uint8_t network_server_host_valid;                    // 0x0071c2dd


extern uint8_t game_engine_ensure_variant_history_has_entry(void); // foreign, UNSURE shape


extern network_client_globals *network_session_create(void); // 0x4d8a80
extern uint8_t network_game_server_host_create(void); // 0x4ddd40

extern void network_game_server_host_dispose(network_server_globals *server); // 0x4deda0

// Tears down any prior host state, creates a new hosted game engine, applies the active custom
// game variant, and opens the network session; on any failure, disposes whatever was partially
// created and restores the torn-down state.
uint8_t multiplayer_host_session_start(void)
{
    uint8_t ok = 1;

    network_client_globals_dispose();
    network_game_setup_teardown();
    network_disconnect_timeout_flag = 1;

    if (network_server == (network_server_globals *)0) {
        game_engine_ensure_variant_history_has_entry();
        ok = network_game_server_host_create();
        if (ok == 1) {
            int32_t *raw = (int32_t *)network_server;

            raw[0x272] = 0;
            raw[0x273] = 0;
            raw[0x274] = 0;
            raw[0x275] = 0;
            *((uint8_t *)raw + 0x9d5) = 1;
            game_variant_history_current = -1;
            game_engine_apply_current_custom_variant();
            game_engine_sync_variant_defaults();
            network_game_mode = 2;
        }
        if (ok == 0) {
            goto fail;
        }
    }

    if (network_client == (network_client_globals *)0) {
        network_client = network_session_create();
        ok = (network_client != (network_client_globals *)0);
        if (ok) {
            network_host_handoff_requested = 0;
        }
    }
    if (ok != 0) {
        return ok;
    }

fail:
    if (network_server != (network_server_globals *)0) {
        network_game_server_host_dispose(network_server);
        network_server = (network_server_globals *)0;
        network_server_host_valid = 0;
    }
    network_client_globals_dispose();
    network_disconnect_timeout_flag = 0;
    network_game_setup_teardown();
    return 0;
}

#if 0
Original Ghidra decompilation (0x49d210):

char multiplayer_host_session_start(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;

  cVar3 = '\x01';
  FUN_004dde70();
  FUN_00495520();
  DAT_0071c2dc = 1;
  if (DAT_0071c2d4 == (int *)0x0) {
    FUN_00463b20();
    iVar2 = network_game_server_host_create();
    piVar1 = DAT_0071c2d4;
    cVar3 = (char)iVar2;
    if (cVar3 == '\x01') {
      DAT_0071c2d4[0x272] = 0;
      piVar1[0x273] = 0;
      piVar1[0x274] = 0;
      piVar1[0x275] = 0;
      *(undefined1 *)((int)piVar1 + 0x9d5) = 1;
      DAT_00687b18 = 0xffffffff;
      game_engine_apply_current_custom_variant();
      FUN_0045fc80();
      DAT_00719720 = 2;
    }
    if (cVar3 == '\0') goto LAB_0049d2a1;
  }
  if (DAT_0071c2d8 == 0) {
    DAT_0071c2d8 = network_session_create();
    cVar3 = DAT_0071c2d8 != 0;
    if ((bool)cVar3) {
      DAT_0071c2de = 0;
    }
  }
  if (cVar3 != '\0') {
    return cVar3;
  }
LAB_0049d2a1:
  if (DAT_0071c2d4 != (int *)0x0) {
    network_game_server_host_dispose(DAT_0071c2d4);
    DAT_0071c2d4 = (int *)0x0;
    DAT_0071c2dd = 0;
  }
  FUN_004dde70();
  DAT_0071c2dc = 0;
  FUN_00495520();
  return '\0';
}
#endif
