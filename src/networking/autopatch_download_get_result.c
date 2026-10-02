// autopatch_download_get_result  (Ghidra: autopatch_download_get_result, already named)
// address 0x576f00, size 61 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary; requires the slot index to be 0 or 1,
// state == k_autopatch_download_ready and the close-requested byte clear before handing back the
// data pointer and size.
// register convention: slot index in ECX (in_ECX, unresolved register read); the two out
// parameters are the recognized __cdecl stack parameters.
// blam-cc: ECX -> slot_index, stack -> out_data, out_size

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern autopatch_download_slot autopatch_download_slots[2]; // 0x006ef93c

// blam-cc: ECX -> slot_index, stack -> out_data, out_size
// Retrieves the completed data pointer and size from a finished download pool slot, if ready.
// Returns 1 (with *out_data/*out_size filled) on success, 0 otherwise.
uint8_t autopatch_download_get_result(void **out_data, int32_t *out_size, int32_t slot_index)
{
    if (slot_index >= 0 && slot_index < 2 &&
        autopatch_download_slots[slot_index].state == k_autopatch_download_ready &&
        ((uint8_t *)&autopatch_download_slots[slot_index].local_file)[1] == 0) {
        *out_data = autopatch_download_slots[slot_index].data;
        *out_size = autopatch_download_slots[slot_index].size;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x576f00):

uint autopatch_download_get_result(undefined4 *param_1,undefined4 *param_2)

{
  undefined4 uVar1;
  uint in_EAX;
  uint uVar2;
  int in_ECX;

  uVar2 = in_EAX & 0xffffff00;
  if ((((-1 < in_ECX) && (in_ECX < 2)) && ((&DAT_006ef940)[in_ECX * 5] == 4)) &&
     (*(char *)(&DAT_006ef94c + in_ECX * 5) == '\0')) {
    *param_1 = (&DAT_006ef944)[in_ECX * 5];
    uVar1 = (&DAT_006ef948)[in_ECX * 5];
    *param_2 = uVar1;
    uVar2 = CONCAT31((int3)((uint)uVar1 >> 8),1);
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
