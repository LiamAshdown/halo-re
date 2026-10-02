// update_server_new  (Ghidra: update_server_new, already named)
// address 0x472aa0, size 86 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Allocates and initializes the server-side network
// update-queue globals, then initializes the paired client-side queue"); types/game.h globals
// list (update_server_initialized/tick/queues/history at 0x006f1d88..0x006f1d94);
// src/memory/data_new.c for the established `data_new(element_size EBX, name, maximum_count)`
// convention -- the element size (0x64, k_update_server_queue... matches update_server_queue's
// own 0x64 size) is not shown by Ghidra's call and is set via EBX before it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t update_server_initialized;    // 0x006f1d88
extern int32_t update_server_tick;            // 0x006f1d8c
extern data_array *update_server_queues;      // 0x006f1d90
extern update_record update_server_history[32]; // 0x006f1d94

extern data_array *data_new(int16_t element_size, char *name, int16_t maximum_count); // 0x4d0370, blam-cc: EBX -> element_size
extern uint32_t update_client_new(void); // 0x472f40; only AL is meaningful

// Zeroes the server update-queue globals, allocates the 16-entry update_server_queues array
// (0x64-byte elements), zeroes the 32-deep history ring, and initializes the client-side queue
// too. Returns true (and marks itself initialized) only if both succeed.
uint8_t update_server_new(void)
{
    memset(&update_server_initialized, 0, 0x610c); // Ghidra's 0x1843 dwords == this exact byte
        // span, from update_server_initialized through the end of update_server_history

    update_server_queues = data_new(0x64, (char *)"update server queues", 16);
    if (update_server_queues != 0) {
        memset(update_server_history, 0, sizeof(update_server_history));
        if (update_client_new() != 0) {
            update_server_initialized = 1;
            return 1;
        }
    }
    return update_server_initialized;
}

#if 0
Original Ghidra decompilation (0x472aa0), from tools/pack.py 0x472aa0:

undefined1 update_server_new(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;

  puVar3 = (undefined4 *)&DAT_006f1d88;
  for (iVar2 = 0x1843; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  DAT_006f1d90 = data_new("update server queues",0x10);
  if (DAT_006f1d90 != 0) {
    puVar3 = &DAT_006f1d94;
    for (iVar2 = 0x1840; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    cVar1 = update_client_new();
    if (cVar1 != '\0') {
      DAT_006f1d88 = 1;
      return 1;
    }
  }
  return DAT_006f1d88;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
