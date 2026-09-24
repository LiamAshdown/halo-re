// network_channel_list_remove  (Ghidra: network_channel_list_remove, already named)
// address 0x441b00, size 175 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "network_channel_list (0x114)" section:
// "network_channel_list_add uses +0x00 as a count, +0x04.. as a 64-entry dedup array and
// +0x10c as the write cursor." This is add's mirror image: it finds `entry` in `list->entries`
// by pointer identity, removes its socket_key from the dedup array (shifting later entries
// down), then swap-removes the entries slot by moving the last live entry into the freed one.
// register convention: entry pointer in EAX (decompiled as an explicit param_1), list pointer
// in ECX (in_ECX), matching network_channel_list_add.c's convention for the same struct.
// UNSURE: none beyond the shared network_channel_list layout notes above.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: entry pointer in EAX (param_1), list pointer in ECX (in_ECX)
// Returns 0 on success, -19 (0xffffffed) if the list is empty (last_index < 0) or `entry` is
// not present in list->entries.
int32_t network_channel_list_remove(network_receive_queue *entry, network_channel_list *list)
{
    int32_t found_index;
    uint32_t dedup_index;
    uint32_t i;

    found_index = 0;
    if (list->last_index < 0) {
        return 0xffffffed;
    }
    while (list->entries[found_index] != entry) {
        found_index = found_index + 1;
        if (list->last_index < found_index) {
            return 0xffffffed;
        }
    }
    if (list->fd_count != 0) {
        dedup_index = 0;
        while (list->fd_array[dedup_index] != (uint32_t)entry->socket_key) {
            dedup_index = dedup_index + 1;
            if (list->fd_count <= dedup_index) {
                goto done_dedup;
            }
        }
        if (dedup_index < list->fd_count - 1) {
            for (i = dedup_index; i < list->fd_count - 1; i++) {
                list->fd_array[i] = list->fd_array[i + 1];
            }
        }
        list->fd_count = list->fd_count - 1;
    }
done_dedup:
    entry->flags = entry->flags & 0xf7;
    list->entries[found_index] = list->entries[list->last_index];
    list->entries[list->last_index] = 0;
    list->last_index = list->last_index - 1;
    return 0;
}

#if 0
Original Ghidra decompilation (0x441b00):

undefined4 network_channel_list_remove(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  uint *in_ECX;
  int *piVar3;
  uint *puVar4;
  int iVar5;

  iVar5 = 0;
  uVar1 = 0xffffffed;
  if (-1 < (int)in_ECX[0x43]) {
    piVar3 = (int *)in_ECX[0x41];
    while (*piVar3 != param_1) {
      iVar5 = iVar5 + 1;
      piVar3 = piVar3 + 1;
      if ((int)in_ECX[0x43] < iVar5) {
        return uVar1;
      }
    }
    uVar2 = 0;
    if (*in_ECX != 0) {
      puVar4 = in_ECX;
LAB_00441b47:
      puVar4 = puVar4 + 1;
      if (*puVar4 != *(uint *)(param_1 + 8)) goto code_r0x00441b4b;
      if (uVar2 < *in_ECX - 1) {
        puVar4 = in_ECX + uVar2 + 1;
        do {
          *puVar4 = puVar4[1];
          uVar2 = uVar2 + 1;
          puVar4 = puVar4 + 1;
        } while (uVar2 < *in_ECX - 1);
      }
      *in_ECX = *in_ECX - 1;
    }
LAB_00441b72:
    *(byte *)(param_1 + 0xc) = *(byte *)(param_1 + 0xc) & 0xf7;
    *(undefined4 *)(in_ECX[0x41] + iVar5 * 4) = *(undefined4 *)(in_ECX[0x41] + in_ECX[0x43] * 4);
    *(undefined4 *)(in_ECX[0x41] + in_ECX[0x43] * 4) = 0;
    in_ECX[0x43] = in_ECX[0x43] - 1;
    uVar1 = 0;
  }
  return uVar1;
code_r0x00441b4b:
  uVar2 = uVar2 + 1;
  if (*in_ECX <= uVar2) goto LAB_00441b72;
  goto LAB_00441b47;
}
#endif
