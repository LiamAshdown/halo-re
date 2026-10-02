// hud_text_message_queue_init  (Ghidra: hud_text_message_queue_init, already named)
// address 0x4a3ce0, size 96 bytes, callers=0 in this build (types_notes.md: "definitive source
// for the growable_array header at 0x006b37e8"; kept despite zero callers)
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: matches the given name; types/interface.h: "global 0x006b37e8: growable_array
// hud_text_message_queue  element size 0x14" and "global 0x0071922c: int32_t
// hud_text_message_time_base". Same QueryPerformanceCounter-to-milliseconds conversion as
// ui_network_wait_timeout_start.c (this session).
// register convention: none (void).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern growable_array hud_text_message_queue; // 0x006b37e8, element size 0x14 (hud_text_message)
extern int64_t performance_frequency;          // 0x006ac8f8/0x006ac8fc
extern int32_t hud_text_message_time_base;      // 0x0071922c


// Initializes the empty HUD text-message queue and stamps the current time (in milliseconds) as
// its base for later message expiry calculations.
uint32_t hud_text_message_queue_init(void)
{
    large_integer counter;

    hud_text_message_queue.element_size = 0x14;
    hud_text_message_queue.count = 0;
    hud_text_message_queue.data = (void *)0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    hud_text_message_time_base = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a3ce0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 hud_text_message_queue_init(void)

{
  undefined8 uVar1;
  LARGE_INTEGER local_8;

  _DAT_006b37e8 = 0x14;
  DAT_006b37ec = 0;
  DAT_006b37f0 = 0;
  QueryPerformanceCounter(&local_8);
  uVar1 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  DAT_0071922c = __alldiv(uVar1,DAT_006ac8f8,DAT_006ac8fc);
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
