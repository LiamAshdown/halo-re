// update_client_queue_get_slot  (Ghidra: update_client_queue_get_slot, already named)
// address 0x473500, size 86 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Returns a pointer to a slot in the 128-entry client
// update ring buffer, either allocating the next write slot (client) or looking up a slot by
// tick (server/single-player)"); types/game.h update_client_history (128 x update_record,
// 0x006f7ed4), update_client_write_cursor (0x006887b0).
// register convention: a tick value in EAX (Ghidra's `in_EAX`), used only on the
// server/single-player path.
//   // blam-cc: EAX -> tick

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode;              // 0x00719720
extern int32_t update_client_write_cursor;      // 0x006887b0
extern int32_t update_client_base_tick;          // 0x006f7e9c
extern update_record update_client_history[128]; // 0x006f7ed4

// blam-cc: EAX -> tick
// On a client or replay connection, allocates and returns the next write slot in the 128-deep
// ring (advancing the write cursor). Otherwise (server or single-player), returns the slot for
// `tick` if it falls within the current 128-tick window, or NULL if it does not.
update_record *update_client_queue_get_slot(int32_t tick)
{
    if (network_game_mode != 2 && network_game_mode != 0) {
        int32_t slot = update_client_write_cursor & 0x7f;

        update_client_write_cursor = update_client_write_cursor + 1;
        return &update_client_history[slot];
    }
    if (update_client_base_tick <= tick && tick < update_client_base_tick + 0x80) {
        return &update_client_history[tick & 0x7f];
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x473500), from tools/pack.py 0x473500:

undefined4 * update_client_queue_get_slot(void)

{
  uint in_EAX;
  uint uVar1;

  if ((DAT_00719720 != 2) && (DAT_00719720 != 0)) {
    uVar1 = DAT_006887b0 & 0x7f;
    DAT_006887b0 = DAT_006887b0 + 1;
    return &DAT_006f7ed4 + uVar1 * 0xc2;
  }
  if ((DAT_006f7e9c <= (int)in_EAX) && ((int)in_EAX < DAT_006f7e9c + 0x80)) {
    return &DAT_006f7ed4 + (in_EAX & 0x7f) * 0xc2;
  }
  return (undefined4 *)0x0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
