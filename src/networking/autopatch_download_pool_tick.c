// autopatch_download_pool_tick  (Ghidra: autopatch_download_pool_tick, already named)
// address 0x576bc0, size 103 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: strides autopatch_download_slots[] exactly like autopatch_download_complete_callback;
// a slot whose close-requested byte (local_file+1, offset +0x11) is set gets its data freed
// (unless the local-file flag itself is set) and its underlying transfer canceled via
// ghttpCancelRequest, then is reset to the free state (-1).
// register convention: no register-passed arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern autopatch_download_slot autopatch_download_slots[2]; // 0x006ef93c

extern void ghttpThink(void); // foreign, UNSURE
extern void ghttpCancelRequest(int32_t request_id); // foreign, UNSURE

// Advances/cleans up the small asynchronous download slot table and returns the count of still
// (non-free) slots.
int32_t autopatch_download_pool_tick(void)
{
    int32_t active_count;
    int32_t i;

    active_count = 0;
    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id != -1) {
            active_count = active_count + 1;
        }
        if (((uint8_t *)&autopatch_download_slots[i].local_file)[1] != 0) {
            if (autopatch_download_slots[i].data != 0 && autopatch_download_slots[i].local_file == 0) {
                GlobalFree(autopatch_download_slots[i].data);
            }
            ghttpCancelRequest(autopatch_download_slots[i].request_id);
            autopatch_download_slots[i].request_id = 0;
            autopatch_download_slots[i].state = 0;
            autopatch_download_slots[i].data = 0;
            autopatch_download_slots[i].size = 0;
            autopatch_download_slots[i].local_file = 0;
            autopatch_download_slots[i].request_id = -1;
        }
    }
    ghttpThink();
    return active_count;
}

#if 0
Original Ghidra decompilation (0x576bc0):

int autopatch_download_pool_tick(void)

{
  int iVar1;
  int *piVar2;

  iVar1 = 0;
  piVar2 = &DAT_006ef93c;
  do {
    if (*piVar2 != -1) {
      iVar1 = iVar1 + 1;
    }
    if (*(char *)((int)piVar2 + 0x11) != '\0') {
      if (((HGLOBAL)piVar2[2] != (HGLOBAL)0x0) && ((char)piVar2[4] == '\0')) {
        GlobalFree((HGLOBAL)piVar2[2]);
      }
      FUN_0061c030(*piVar2);
      *piVar2 = 0;
      piVar2[1] = 0;
      piVar2[2] = 0;
      piVar2[3] = 0;
      piVar2[4] = 0;
      *piVar2 = -1;
    }
    piVar2 = piVar2 + 5;
  } while ((int)piVar2 < 0x6ef964);
  FUN_0061c020();
  return iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
