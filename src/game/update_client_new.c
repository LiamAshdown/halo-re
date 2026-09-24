// update_client_new  (Ghidra: update_client_new, already named)
// address 0x472f40, size 93 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Allocates and initializes the client-side network
// update-queue globals and its ring-buffer state"); types/game.h globals list
// (update_client_initialized/base_tick/unknown_ea0/queues/history at 0x006f7e98..0x006f7ed4);
// src/memory/data_new.c for the established EBX element-size convention.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

extern uint8_t update_client_initialized;      // 0x006f7e98
extern int32_t update_client_base_tick;         // 0x006f7e9c
extern int32_t update_client_unknown_ea0;        // 0x006f7ea0
extern data_array *update_client_queues;          // 0x006f7ed0
extern update_record update_client_history[128];  // 0x006f7ed4

extern data_array *data_new(int16_t element_size, char *name, int16_t maximum_count); // 0x4d0370, blam-cc: EBX -> element_size

// Zeroes the client update-queue globals, allocates the 16-entry update_client_queues array
// (0x64-byte elements), and fills the 128-deep history ring with -1 bytes (so every record starts
// "no tick"). Returns a nonzero success flag (Ghidra: 0xffffff01) on success, or the
// (zero) initialized flag on failure.
uint32_t update_client_new(void)
{
    memset(&update_client_initialized, 0, 0x1843c); // Ghidra's 0x610f dwords (0x610f*4 ==
        // this byte count) span update_client_initialized through the end of
        // update_client_history exactly (0x3c header bytes + 128*0x308 history bytes)

    update_client_queues = data_new(0x28, "update client queues", 16); // 0x472f55: mov ebx,0x28 (the draft copied the server's 0x64)
    if (update_client_queues != 0) {
        memset(update_client_history, 0xff, sizeof(update_client_history));
        update_client_unknown_ea0 = -1;
        update_client_base_tick = 0;
        update_client_initialized = 1;
        return 0xffffff01;
    }
    return update_client_initialized;
}

#if 0
Original Ghidra decompilation (0x472f40), from tools/pack.py 0x472f40:

uint update_client_new(void)

{
  int iVar1;
  undefined4 *puVar2;

  puVar2 = (undefined4 *)&DAT_006f7e98;
  for (iVar1 = 0x610f; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_006f7ed0 = data_new("update client queues",0x10);
  if (DAT_006f7ed0 != 0) {
    puVar2 = &DAT_006f7ed4;
    for (iVar1 = 0x6100; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0xffffffff;
      puVar2 = puVar2 + 1;
    }
    DAT_006f7ea0 = 0xffffffff;
    DAT_006f7e9c = 0;
    DAT_006f7e98 = 1;
    return 0xffffff01;
  }
  return (uint)DAT_006f7e98;
}
#endif
