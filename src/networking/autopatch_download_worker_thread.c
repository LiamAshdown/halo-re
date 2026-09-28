// autopatch_download_worker_thread  (Ghidra: autopatch_download_worker_thread, already named)
// address 0x576b80, size 59 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md summary; polls autopatch_download_pool_tick,
// sleeping 20ms while slots are active and 1000ms once the pool goes idle, until told to stop.
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t autopatch_download_active_count; // 0x007227c8, UNSURE: per types/networking.h's
                                                 // name, but used here as a 0/nonzero stop signal
                                                 // (autopatch_download_pool_shutdown sets it to 1
                                                 // to request the worker thread exit)

extern int32_t autopatch_download_pool_tick(void); // 0x576bc0, this module
extern void __stdcall Sleep(uint32_t milliseconds);

// Background worker thread that repeatedly polls the download pool until told to stop.
uint32_t autopatch_download_worker_thread(void)
{
    int32_t active_count = 1;

    do {
        Sleep(active_count < 1 ? 1000 : 0x14);
        active_count = autopatch_download_pool_tick();
    } while (autopatch_download_active_count == 0);
    autopatch_download_active_count = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x576b80):

undefined4 autopatch_download_worker_thread(void)

{
  int iVar1;
  DWORD dwMilliseconds;

  iVar1 = 1;
  do {
    if (iVar1 < 1) {
      dwMilliseconds = 1000;
    }
    else {
      dwMilliseconds = 0x14;
    }
    Sleep(dwMilliseconds);
    iVar1 = autopatch_download_pool_tick();
  } while (DAT_007227c8 == '\0');
  DAT_007227c8 = 0;
  return 0;
}
#endif
