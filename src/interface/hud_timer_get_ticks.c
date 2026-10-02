// hud_timer_get_ticks  (Ghidra: hud_counter_get_value, renamed in the phase-4 review)
// address 0x4adcc0, size 65 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4adcc0..0x4add00; the only caller is hud_timer_draw (0x4add10). The
// "counter" of the earlier name is the script countdown timer (hud_set_timer_time 0x4adbf0,
// pause_hud_timer 0x4adc60). Returns 0 while the timer is not active, -1 (all 32 bits) once it
// ran out (0xffff), the frozen tick count while paused, and otherwise the 16-bit
// timer_ticks - game_time + timer_start_time zero-extended (the upper half of EAX is 0).
// register convention: none; the result is in EAX, callers test AX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hud_messaging_globals *hud_messaging;  // 0x006b3a40
extern game_time_globals *game_time;          // 0x006f1d6c

// Ticks left on the HUD countdown timer (see the header for the special values).
uint32_t hud_timer_get_ticks(void)
{
    uint16_t ticks;

    if (hud_messaging->timer_active == 0) {
        return 0;
    }
    ticks = hud_messaging->timer_ticks;
    if (ticks == 0xffff) {
        return 0xffffffff;
    }
    if (hud_messaging->timer_paused != 0) {
        return ticks;
    }
    return (uint16_t)(ticks - (uint16_t)game_time->game_time + (uint16_t)hud_messaging->timer_start_time);
}

#if 0
Original Ghidra decompilation (0x4adcc0):

uint hud_counter_get_value(void)

{
  ushort uVar1;
  uint uVar2;

  uVar2 = 0;
  if (*(char *)(DAT_006b3a40 + 0x487) != '\0') {
    uVar1 = *(ushort *)(DAT_006b3a40 + 0x47c);
    uVar2 = (uint)uVar1;
    if (uVar1 == 0xffff) {
      return 0xffffffff;
    }
    if (*(char *)(DAT_006b3a40 + 0x486) == '\0') {
      uVar2 = (uint)(ushort)((uVar1 - *(short *)(DAT_006f1d6c + 0xc)) +
                            *(short *)(DAT_006b3a40 + 0x478));
    }
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
