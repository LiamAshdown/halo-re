// network_channel_list_add  (Ghidra: network_channel_list_add, already named)
// address 0x441a40, size 184 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "network_channel_list (0x114)" section:
// "network_channel_list_add uses +0x00 as a count, +0x04.. as a 64-entry dedup array and
// +0x10c as the write cursor."
// register convention: entry pointer in EAX (in_EAX), list pointer in ECX (in_ECX).
// UNSURE: the decompiled `if ((entry->flags & 2) == 0) { X } else { X }` has byte-for-byte
// identical bodies in both branches; reproduced literally (as two identical blocks) rather
// than collapsed, since this rewrite does not assume that duplication is meaningless dead
// code versus a compiled remnant of genuinely different source.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: entry pointer in EAX (in_EAX), list pointer in ECX (in_ECX)
// Appends `entry` to the write-cursor slot of `list->entries`, then adds its socket_key to the
// 64-entry dedup array if not already present and there is room. Always advances last_index
// and marks the entry as "in a list" (flags bit3). Returns 0 on success, -20 (0xffffffec) if
// the list's write cursor has reached capacity or would overflow.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t network_channel_list_add(network_receive_queue *entry, network_channel_list *list)
{
    int32_t new_index;
    uint32_t fd_count_snapshot;
    uint32_t dedup_index;
    uint32_t i;

    if (list->capacity - 1 < list->last_index || (new_index = list->last_index + 1, new_index < 0)) {
        return 0xffffffec;
    }
    list->entries[new_index] = entry;
    fd_count_snapshot = list->fd_count;
    if ((entry->flags & 2) == 0) {
        dedup_index = 0;
        if (fd_count_snapshot != 0) {
            for (i = 0; i < list->fd_count; i++) {
                if (list->fd_array[i] == (uint32_t)list->entries[new_index]->socket_key) {
                    break;
                }
                dedup_index = dedup_index + 1;
            }
        }
    } else {
        dedup_index = 0;
        if (fd_count_snapshot != 0) {
            for (i = 0; i < list->fd_count; i++) {
                if (list->fd_array[i] == (uint32_t)list->entries[new_index]->socket_key) {
                    break;
                }
                dedup_index = dedup_index + 1;
            }
        }
    }
    if (dedup_index == fd_count_snapshot && fd_count_snapshot < 0x40) {
        list->fd_array[dedup_index] = (uint32_t)list->entries[new_index]->socket_key;
        list->fd_count = list->fd_count + 1;
    }
    list->last_index = list->last_index + 1;
    entry->flags = entry->flags | 8;
    return 0;
}

#if 0
Original Ghidra decompilation (0x441a40):

undefined4 network_channel_list_add(void)

{
  int iVar1;
  uint uVar2;
  int in_EAX;
  uint *in_ECX;
  uint uVar3;
  uint *puVar4;

  if (((int)(in_ECX[0x42] - 1) < (int)in_ECX[0x43]) || (iVar1 = in_ECX[0x43] + 1, iVar1 < 0)) {
    return 0xffffffec;
  }
  *(int *)(in_ECX[0x41] + iVar1 * 4) = in_EAX;
  uVar2 = *in_ECX;
  if ((*(byte *)(in_EAX + 0xc) & 2) == 0) {
    uVar3 = 0;
    if (uVar2 != 0) {
      puVar4 = in_ECX;
      do {
        puVar4 = puVar4 + 1;
        if (*puVar4 == *(uint *)(*(int *)(in_ECX[0x41] + iVar1 * 4) + 8)) break;
        uVar3 = uVar3 + 1;
      } while (uVar3 < *in_ECX);
    }
  }
  else {
    uVar3 = 0;
    if (uVar2 != 0) {
      puVar4 = in_ECX;
      do {
        puVar4 = puVar4 + 1;
        if (*puVar4 == *(uint *)(*(int *)(in_ECX[0x41] + iVar1 * 4) + 8)) break;
        uVar3 = uVar3 + 1;
      } while (uVar3 < *in_ECX);
    }
  }
  if ((uVar3 == uVar2) && (uVar2 < 0x40)) {
    in_ECX[uVar3 + 1] = *(uint *)(*(int *)(in_ECX[0x41] + iVar1 * 4) + 8);
    *in_ECX = *in_ECX + 1;
  }
  in_ECX[0x43] = in_ECX[0x43] + 1;
  *(byte *)(in_EAX + 0xc) = *(byte *)(in_EAX + 0xc) | 8;
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
