// network_channel_reliable_pool_ensure_capacity  (Ghidra: FUN_004dcc30; named per this rewrite)
// address 0x4dcc30, size 367 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Finds (or grows and allocates) a reusable pair
// of fixed-size scratch buffers in the channel's reliable-message buffer pool, sized to hold at
// least the requested header/body byte counts." Fully recovered cdecl parameters; every offset
// (+0xa78, +0xa7c, and the reliable-slot fields +0x00/+0x04/+0x08/+0x0c/+0x10/+0x14/+0x18/+0x1c)
// matches types/networking.h's network_channel.reliable_count/reliable and
// network_channel_reliable_slot exactly. types/networking.h's own comment on this function
// ("the argument order in 0x4dcdb0 is the opposite of the capacity order in 0x4dcc30, which is
// why the capacities are cross-named") confirms param_2 = needed body capacity, param_3 = needed
// header capacity.
// FIXED against objdump -d -M intel bin/halo.exe (0x4dcc30..0x4dcda7): Ghidra types this
// function void, but it is not -- the early-return path (`jne 0x4dcda3` at 0x4dcc7a) jumps
// straight to the epilogue with EAX still holding the matched slot's loop index, and the
// grow path's last `mov eax,[ebp+0xa78]` at 0x4dcd94 loads the OLD reliable_count (the index of
// the first newly-allocated slot) right before the epilogue. Both paths leave a valid slot index
// in EAX, so this rewrite returns int32_t; the caller (network_channel_reliable_pool_store.c)
// uses exactly this index.
// The `if (iVar1 != -1) return;` / `break` pair right after a match is found is unreachable dead
// code (the loop counter is never -1 at that point) and is kept verbatim rather than simplified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern void *GlobalAlloc(uint32_t flags, uint32_t bytes);
extern void *GlobalFree(void *memory);

// Scans the pool for a free (pending == 0) slot already large enough for both requested
// capacities; if one exists, returns its index without doing anything else. Otherwise grows the
// pool by 10 slots (reallocating and copying the existing array), initializes each new slot with
// max(requested, 100)-byte header/body allocations, and returns the index of the first new slot.
int32_t network_channel_reliable_pool_ensure_capacity(network_channel *channel, int32_t body_capacity_needed,
    int32_t header_capacity_needed)
{
    int32_t i;
    network_channel_reliable_slot *slot;
    network_channel_reliable_slot *new_pool;
    int32_t old_count;
    int32_t header_cap;
    int32_t body_cap;

    i = 0;
    if (channel->reliable_count > 0) {
        slot = channel->reliable;
        do {
            if (slot->pending == 0 && body_capacity_needed <= slot->body_capacity &&
                header_capacity_needed <= slot->header_capacity) {
                if (i != -1) {
                    return i;
                }
                break; // unreachable, kept verbatim -- see header note
            }
            i = i + 1;
            slot = slot + 1;
        } while (i < channel->reliable_count);
    }

    new_pool = (network_channel_reliable_slot *)GlobalAlloc(0, (channel->reliable_count + 10) * 0x20);
    if (channel->reliable_count > 0) {
        memcpy(new_pool, channel->reliable, (uint32_t)channel->reliable_count * 0x20);
        GlobalFree(channel->reliable);
    }
    old_count = channel->reliable_count;
    channel->reliable = new_pool;
    for (i = old_count; i < channel->reliable_count + 10; i++) {
        slot = &channel->reliable[i];
        slot->pending = 0;
        slot->body_bits = 0;
        slot->header_bits = 0;
        slot->priority = -1;
        header_cap = header_capacity_needed;
        if (header_capacity_needed < 100) {
            header_cap = 100;
        }
        slot->header_capacity = header_cap;
        body_cap = body_capacity_needed;
        if (body_capacity_needed < 100) {
            body_cap = 100;
        }
        slot->body_capacity = body_cap;
        slot->body = (uint8_t *)GlobalAlloc(0, slot->body_capacity);
        slot->header = (uint8_t *)GlobalAlloc(0, slot->header_capacity);
    }
    channel->reliable_count = channel->reliable_count + 10;
    return old_count;
}

#if 0
Original Ghidra decompilation (0x4dcc30):

void FUN_004dcc30(int param_1,int param_2,int param_3)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  HGLOBAL pvVar4;
  char *pcVar5;
  undefined4 *puVar6;
  int iVar7;
  undefined4 *puVar8;

  iVar1 = 0;
  if (0 < *(int *)(param_1 + 0xa78)) {
    pcVar5 = *(char **)(param_1 + 0xa7c);
    do {
      if (((*pcVar5 == '\0') && (param_2 <= *(int *)(pcVar5 + 0xc))) &&
         (param_3 <= *(int *)(pcVar5 + 8))) {
        if (iVar1 != -1) {
          return;
        }
        break;
      }
      iVar1 = iVar1 + 1;
      pcVar5 = pcVar5 + 0x20;
    } while (iVar1 < *(int *)(param_1 + 0xa78));
  }
  puVar2 = GlobalAlloc(0,(*(int *)(param_1 + 0xa78) + 10) * 0x20);
  if (0 < (int)*(uint *)(param_1 + 0xa78)) {
    puVar6 = *(undefined4 **)(param_1 + 0xa7c);
    puVar8 = puVar2;
    for (iVar1 = (*(uint *)(param_1 + 0xa78) & 0x7ffffff) << 3; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar8 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar8 = puVar8 + 1;
    }
    for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
      *(undefined1 *)puVar8 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
      puVar8 = (undefined4 *)((int)puVar8 + 1);
    }
    GlobalFree(*(HGLOBAL *)(param_1 + 0xa7c));
  }
  iVar1 = *(int *)(param_1 + 0xa78);
  *(undefined4 **)(param_1 + 0xa7c) = puVar2;
  if (iVar1 < iVar1 + 10) {
    iVar7 = iVar1 << 5;
    do {
      *(undefined1 *)(iVar7 + *(int *)(param_1 + 0xa7c)) = 0;
      *(undefined4 *)(iVar7 + 0x14 + *(int *)(param_1 + 0xa7c)) = 0;
      *(undefined4 *)(iVar7 + 0x10 + *(int *)(param_1 + 0xa7c)) = 0;
      *(undefined4 *)(iVar7 + 4 + *(int *)(param_1 + 0xa7c)) = 0xffffffff;
      iVar3 = param_3;
      if (param_3 < 100) {
        iVar3 = 100;
      }
      *(int *)(iVar7 + 8 + *(int *)(param_1 + 0xa7c)) = iVar3;
      iVar3 = param_2;
      if (param_2 < 100) {
        iVar3 = 100;
      }
      *(int *)(iVar7 + 0xc + *(int *)(param_1 + 0xa7c)) = iVar3;
      pvVar4 = GlobalAlloc(0,*(SIZE_T *)(iVar7 + 0xc + *(int *)(param_1 + 0xa7c)));
      *(HGLOBAL *)(iVar7 + 0x1c + *(int *)(param_1 + 0xa7c)) = pvVar4;
      pvVar4 = GlobalAlloc(0,*(SIZE_T *)(iVar7 + 8 + *(int *)(param_1 + 0xa7c)));
      *(HGLOBAL *)(iVar7 + 0x18 + *(int *)(param_1 + 0xa7c)) = pvVar4;
      iVar1 = iVar1 + 1;
      iVar7 = iVar7 + 0x20;
    } while (iVar1 < *(int *)(param_1 + 0xa78) + 10);
  }
  *(int *)(param_1 + 0xa78) = *(int *)(param_1 + 0xa78) + 10;
  return;
}
#endif
