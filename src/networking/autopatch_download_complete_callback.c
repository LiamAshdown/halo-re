// autopatch_download_complete_callback  (Ghidra: autopatch_download_complete_callback, already named)
// address 0x576ad0, size 165 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: strides autopatch_download_slots[] (types/networking.h) by 5 dwords (0x14 bytes),
// matching autopatch_download_slot exactly; sets state to k_autopatch_download_error (5) on
// error, or allocates and NUL-terminates the payload and sets k_autopatch_download_ready (4) on
// success -- unless a close was already requested (the byte immediately after local_file, offset
// +0x11, which autopatch_download_pool_shutdown also sets), in which case the data is left alone.
// register convention: the four __cdecl stack parameters Ghidra recognized.

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


// Completion callback for an asynchronous download/read: on success, copies the received data
// into a heap buffer (unless the slot's close-requested flag is set) and marks the matching pool
// slot ready or errored.
uint32_t autopatch_download_complete_callback(int32_t request_id, int32_t error, uint8_t *data, uint32_t size)
{
    int32_t i;

    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id == request_id) {
            if (error != 0) {
                autopatch_download_slots[i].state = k_autopatch_download_error;
                return 1;
            }
            if (((uint8_t *)&autopatch_download_slots[i].local_file)[1] == 0) {
                uint8_t *buffer = (uint8_t *)GlobalAlloc(0, size + 1);
                uint32_t j;

                autopatch_download_slots[i].data = buffer;
                for (j = 0; j < size; j++) {
                    buffer[j] = data[j];
                }
                buffer[size] = 0;
                autopatch_download_slots[i].size = size + 1;
            }
            autopatch_download_slots[i].state = k_autopatch_download_ready;
            return 1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x576ad0):

undefined4
autopatch_download_complete_callback(int param_1,int param_2,undefined4 *param_3,uint param_4)

{
  int *piVar1;
  undefined4 *puVar2;
  int iVar3;
  uint uVar4;

  piVar1 = &DAT_006ef93c;
  iVar3 = 0;
  do {
    if (*piVar1 == param_1) {
      if (iVar3 != -1) {
        if (param_2 != 0) {
          (&DAT_006ef940)[iVar3 * 5] = 5;
          return 1;
        }
        if (*(char *)(&DAT_006ef94c + iVar3 * 5) == '\0') {
          puVar2 = GlobalAlloc(0,param_4 + 1);
          (&DAT_006ef944)[iVar3 * 5] = puVar2;
          for (uVar4 = param_4 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
            *puVar2 = *param_3;
            param_3 = param_3 + 1;
            puVar2 = puVar2 + 1;
          }
          for (uVar4 = param_4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *(undefined1 *)puVar2 = *(undefined1 *)param_3;
            param_3 = (undefined4 *)((int)param_3 + 1);
            puVar2 = (undefined4 *)((int)puVar2 + 1);
          }
          *(undefined1 *)((&DAT_006ef944)[iVar3 * 5] + param_4) = 0;
          (&DAT_006ef948)[iVar3 * 5] = param_4 + 1;
        }
        (&DAT_006ef940)[iVar3 * 5] = 4;
      }
      return 1;
    }
    piVar1 = piVar1 + 5;
    iVar3 = iVar3 + 1;
  } while ((int)piVar1 < 0x6ef964);
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
