// update_queues_dispose  (Ghidra: update_queues_dispose, already named)
// address 0x472b00, size 109 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Frees and resets both the server-side and client-side
// network update-queue allocations"); types/game.h globals list.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *update_server_queues;      // 0x006f1d90
extern data_array *update_client_queues;      // 0x006f7ed0
extern uint8_t update_server_initialized;     // 0x006f1d88
extern int32_t update_server_tick;             // 0x006f1d8c
extern uint8_t update_client_initialized;      // 0x006f7e98
extern int32_t update_client_base_tick;         // 0x006f7e9c
extern int32_t update_client_unknown_ea0;        // 0x006f7ea0


// Frees update_server_queues and update_client_queues (each preceded by a 14-dword zero pass
// over their data_array headers, matching Ghidra literally) and resets every bookkeeping global.
void update_queues_dispose(void)
{
    if (update_server_queues != 0) {
        uint32_t *words = (uint32_t *)update_server_queues;
        int32_t i;

        for (i = 0; i < 14; i++) {
            words[i] = 0;
        }
        GlobalFree(update_server_queues);
        update_server_queues = 0;
    }
    update_server_initialized = 0;
    update_server_tick = 0;

    if (update_client_queues != 0) {
        uint32_t *words = (uint32_t *)update_client_queues;
        int32_t i;

        for (i = 0; i < 14; i++) {
            words[i] = 0;
        }
        GlobalFree(update_client_queues);
        update_client_queues = 0;
    }

    update_client_base_tick = 0;
    update_client_initialized = 0;
    update_client_unknown_ea0 = -1;
}

#if 0
Original Ghidra decompilation (0x472b00), from tools/pack.py 0x472b00:

void update_queues_dispose(void)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 *puVar3;

  puVar1 = DAT_006f1d90;
  if (DAT_006f1d90 != (undefined4 *)0x0) {
    puVar3 = DAT_006f1d90;
    for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    GlobalFree(puVar1);
    DAT_006f1d90 = (undefined4 *)0x0;
  }
  puVar1 = DAT_006f7ed0;
  DAT_006f1d88 = 0;
  DAT_006f1d8c = 0;
  if (DAT_006f7ed0 != (undefined4 *)0x0) {
    puVar3 = DAT_006f7ed0;
    for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    GlobalFree(puVar1);
    DAT_006f7ed0 = (undefined4 *)0x0;
  }
  DAT_006f7e9c = 0;
  DAT_006f7e98 = 0;
  DAT_006f7ea0 = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
