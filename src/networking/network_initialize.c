// network_initialize  (Ghidra: network_initialize, already named)
// address 0x4415c0, size 281 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("one-time networking subsystem
// startup: calls WSAStartup, determines the local IP address, and launches the background
// network processing thread"); shares network_disabled_flag (0x007196ec) and
// network_local_address (0x006869b0, already documented in networking_types_notes.md as
// "byte swapped before binding") with network_dispatch_initialize.c / network_channels_open.c.
// register convention: __cdecl, no arguments.
// UNSURE: DAT_006869b4, DAT_006869b8 and DAT_006f14c0 are not documented anywhere in
// networking_types_notes.md; named here from how they are used (resolved/override address,
// one-time-init flag, and an init timestamp respectively) but not independently confirmed.
// UNSURE: WSADATA is a Winsock structure with no Blam equivalent; the manual zero-fill loop
// is folded into an equivalent memset over a same-size opaque byte buffer (same technique as
// the strcpy/strcat foldings in the summary/connection stats log functions), since the loop's
// only effect is zeroing the whole 400-byte structure before WSAStartup fills it in.
// UNSURE: the hostent parsing (`**(hostent+0xc)`, i.e. h_addr_list[0] dereferenced) is a raw
// Winsock structure walk with no Blam type, same treatment as network_local_hostent_get.c.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern uint8_t network_disabled_flag;      // 0x007196ec, UNSURE: see network_dispatch_initialize.c
extern uint8_t network_winsock_initialized; // 0x006869b8, UNSURE
extern uint32_t network_local_address;     // 0x006869b0, byte swapped before binding
extern uint32_t network_resolved_local_address; // 0x006869b4, UNSURE
extern int32_t network_initialized_at_ms;  // 0x006f14c0, UNSURE

extern int network_local_hostent_get(void **out_hostent); // 0x441540, this module
extern int32_t time_query_performance_counter_ms(void); // foreign module, millisecond tick reader
extern uint32_t __stdcall autopatch_proxy_initialize(void *parameter); // 0x5771c0, the proxy thread

int16_t network_initialize(void)
{
    int16_t result;
    int32_t wsa_result;
    void *hostent;
    uint32_t thread_id;
    uint32_t raw_address;
    uint8_t wsa_data[400]; // sizeof(WSADATA); see UNSURE note above

    result = 0;
    hostent = 0;
    if (network_disabled_flag != 0) {
        network_winsock_initialized = 0;
        return 0;
    }
    if (network_winsock_initialized == 0) {
        memset(wsa_data, 0, sizeof(wsa_data));
        wsa_result = WSAStartup(2, (LPWSADATA)wsa_data);
        if ((int16_t)wsa_result == 0) {
            if (network_local_address == 0) {
                if (network_local_hostent_get(&hostent) == 0) {
                    return -0x10;
                }
                raw_address = *(uint32_t *)**(uint32_t **)((uint8_t *)hostent + 0xc);
                network_resolved_local_address =
                    (raw_address & 0xff0000 | raw_address >> 0x10) >> 8 |
                    (raw_address << 0x10 | raw_address & 0xff00) << 8;
            } else {
                network_resolved_local_address = network_local_address;
            }
        }
        CreateThread(0, 0x10400, autopatch_proxy_initialize, 0, 0, &thread_id); // FIXED 2026-09-28: 0x4416ad pushes
            // 0x5771c0 (autopatch_proxy_initialize); the C started join_game_server_browser_tick on the thread
        network_initialized_at_ms = time_query_performance_counter_ms();
        network_winsock_initialized = 1;
        result = (int16_t)wsa_result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4415c0):

short network_initialize(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  WORD *pWVar4;
  void *local_1ac [2];
  uint local_1a4;
  WSADATA local_198;

  sVar3 = 0;
  local_1ac[0] = (void *)0x0;
  if (DAT_007196ec != 0) {
    DAT_006869b8 = 0;
    return 0;
  }
  if (DAT_006869b8 == '\0') {
    local_198.wVersion = 0;
    pWVar4 = &local_198.wHighVersion;
    for (iVar2 = 99; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined4 *)pWVar4 = 0;
      pWVar4 = pWVar4 + 2;
    }
    *pWVar4 = 0;
    iVar2 = WSAStartup(2,&local_198);
    if ((short)iVar2 == 0) {
      if (DAT_006869b0 == 0) {
        iVar1 = network_local_hostent_get(local_1ac);
        if (iVar1 == 0) {
          return -0x10;
        }
        local_1a4 = *(uint *)**(undefined4 **)((int)local_1ac[0] + 0xc);
        DAT_006869b4 = (local_1a4 & 0xff0000 | local_1a4 >> 0x10) >> 8 |
                       (local_1a4 << 0x10 | local_1a4 & 0xff00) << 8;
      }
      else {
        DAT_006869b4 = DAT_006869b0;
      }
    }
    CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x10400,autopatch_proxy_initialize,(LPVOID)0x0,0,
                 (LPDWORD)local_1ac);
    DAT_006f14c0 = FUN_00449210();
    DAT_006869b8 = '\x01';
    sVar3 = (short)iVar2;
  }
  return sVar3;
}
#endif
