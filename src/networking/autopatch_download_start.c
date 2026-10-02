// autopatch_download_start  (Ghidra: autopatch_download_start, already named)
// address 0x576e60, size 160 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary; finds a free slot (request_id == -1),
// marks it active and starts either an HTTP-ish download (ghttpGetEx, when local_file == 0) or
// a local file read (ghttpSaveEx, when local_file != 0), storing the caller-visible request id
// and resetting the slot to free again on failure.
// register convention: the URL/path as the recognized stack parameter (param_1), a local-file
// flag in EDX (in_EDX, unresolved register read).
// blam-cc: stack -> path, EDX -> local_file
// UNSURE: ghttpGetEx's and ghttpSaveEx's full argument lists (foreign, GameSpy/CRT-shaped);
// transcribed with the arguments Ghidra recovered plus &LAB_00576a70 and this module's own
// completion callback.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern autopatch_download_slot autopatch_download_slots[2]; // 0x006ef93c

extern int32_t ghttpGetEx(void *path, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6,
                             int32_t a7, void *progress_callback, void *complete_callback, int32_t a8); // foreign, UNSURE
extern int32_t ghttpSaveEx(void *url, void *filename, void *headers, void *post, int32_t throttle, int32_t blocking,
                             void *progress_callback, void *complete_callback, void *param); // 0x61bef0 ghttpSaveEx
extern void autopatch_download_progress_callback(int32_t request, int32_t state, const char *buffer,
    int32_t buffer_length, int32_t bytes_received, int32_t total_size, void *param); // 0x576a70
extern uint32_t autopatch_download_complete_callback(int32_t request_id, int32_t error, uint8_t *data, uint32_t size); // 0x576ad0, this module

// blam-cc: stack -> path, EDX -> local_file
// Starts an asynchronous download (or local file read, when local_file is set) into a free pool
// slot, using autopatch_download_complete_callback for completion, and returns the slot index (or
// -1 if the pool is full or the start call itself failed).
int32_t autopatch_download_start(void *path, int32_t local_file)
{
    int32_t slot_index;
    int32_t request_id;

    slot_index = -1;
    for (int32_t i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id == -1) {
            slot_index = i;
            break;
        }
    }
    if (slot_index == -1) {
        return -1;
    }

    autopatch_download_slots[slot_index].state = k_autopatch_download_active;
    if (local_file == 0) {
        request_id = ghttpGetEx(path, 0, 0, 0, 0, 0, 0, (void *)autopatch_download_progress_callback,
                                   (void *)autopatch_download_complete_callback, 0);
        autopatch_download_slots[slot_index].local_file = 0;
    } else {
        // FIXED 2026-09-28 (0x576eb3): EDX is the file to save into, passed on to ghttpSaveEx with the same
        // progress / completion callbacks as the download.
        request_id = ghttpSaveEx(path, (void *)local_file, 0, 0, 0, 0, (void *)autopatch_download_progress_callback,
                                  (void *)autopatch_download_complete_callback, 0);
        autopatch_download_slots[slot_index].local_file = 1;
    }
    autopatch_download_slots[slot_index].request_id = request_id;
    if (request_id == -1) {
        autopatch_download_slots[slot_index].request_id = 0;
        autopatch_download_slots[slot_index].state = 0;
        autopatch_download_slots[slot_index].data = 0;
        autopatch_download_slots[slot_index].size = 0;
        autopatch_download_slots[slot_index].local_file = 0;
        autopatch_download_slots[slot_index].request_id = -1;
        return -1;
    }
    return slot_index;
}

#if 0
Original Ghidra decompilation (0x576e60):

int autopatch_download_start(undefined4 param_1)

{
  int *piVar1;
  int iVar2;
  int in_EDX;
  int iVar3;

  iVar2 = 0;
  piVar1 = &DAT_006ef93c;
  do {
    iVar3 = iVar2;
    if (*piVar1 == -1) break;
    piVar1 = piVar1 + 5;
    iVar2 = iVar2 + 1;
    iVar3 = -1;
  } while ((int)piVar1 < 0x6ef964);
  if (iVar3 != -1) {
    piVar1 = &DAT_006ef93c + iVar3 * 5;
    (&DAT_006ef940)[iVar3 * 5] = 1;
    if (in_EDX == 0) {
      iVar2 = FUN_0061bd80(param_1,0,0,0,0,0,0,&LAB_00576a70,autopatch_download_complete_callback,0)
      ;
      *(undefined1 *)(&DAT_006ef94c + iVar3 * 5) = 0;
    }
    else {
      iVar2 = FUN_0061bef0(param_1);
      *(undefined1 *)(&DAT_006ef94c + iVar3 * 5) = 1;
    }
    *piVar1 = iVar2;
    if (iVar2 == -1) {
      *piVar1 = 0;
      (&DAT_006ef940)[iVar3 * 5] = 0;
      (&DAT_006ef944)[iVar3 * 5] = 0;
      (&DAT_006ef948)[iVar3 * 5] = 0;
      (&DAT_006ef94c)[iVar3 * 5] = 0;
      *piVar1 = -1;
      return -1;
    }
  }
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
