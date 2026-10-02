// ai_group_bucket_find_or_add  (Ghidra: ai_group_bucket_find_or_add; named from out/phase2/results/ai_02.json)
// address 0x420de0, size 103 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase2/results/ai_02.json -- linear-searches a fixed stride-0x1c array (in_EAX)
//   of up to param_1 entries for a key (unaff_EBX), returning its index; if absent and the
//   live count (*unaff_EDI) is below param_1, appends a new zero/-1-initialized entry
//   (including a 0x7f7fffff = FLT_MAX sentinel field) and returns its index. Used by
//   actor_scan_allies_for_backup_request (0x420ec0) to group results per actor type.
// register convention: EAX -> buckets, EBX -> key, EDI -> count (in/out); param_1 (short) is
//   Ghidra's recognized stack parameter, the bucket capacity.
//   // blam-cc: EAX -> buckets, EBX -> key, EDI -> count, stack -> capacity
//
// The 0x1c-byte bucket record now lives in types/ai.h as ai_group_bucket_entry. Only the byte
// offsets the function itself touches are named (0x00, 0x04, 0x08 "key", 0x0c, 0x10, 0x14, 0x18);
// 0x02 and 0x12 are never written and left as padding.
// UNSURE: the original packs the new-entry return as CONCAT22(0xffff, index) -- the upper
// 16 bits are decompiler leftover, not a second value -- so the return type here is the
// plain 16-bit index/sentinel every call site actually needs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// ai_group_bucket_entry now lives in types/ai.h (folded from this file).

// VERIFIED against disassembly 0x420de0..0x420e46 (2026-09-30): search, capacity test and the seven field initialisers
//   (stride 0x1c) match; the original returns EAX = 0xffff0000|index for a new entry (only AX is meaningful).
// blam-cc: EAX -> buckets, EBX -> key, EDI -> count, stack -> capacity
// Finds or allocates a small fixed-capacity aggregation-bucket slot keyed by an id, used by the
// nearby-actor scanning helpers to group results per actor type. Returns the slot index, or -1
// if the key was not found and the bucket array is already full.
int16_t ai_group_bucket_find_or_add(ai_group_bucket_entry *buckets, int32_t key, int16_t *count, int16_t capacity)
{
    int16_t live_count;
    int16_t i;
    ai_group_bucket_entry *entry;

    live_count = *count;

    for (i = 0; i < live_count; i++) {
        if (buckets[i].key == key) {
            return i;
        }
    }

    if (live_count < capacity) {
        entry = &buckets[live_count];
        *count = live_count + 1;
        entry->prop_index = -1;
        entry->key = -1;
        entry->nearest_friend_actor_index = -1;
        entry->priority = 0;
        entry->prop = 0;
        entry->retreating_friend_count = 0;
        entry->nearest_friend_distance_squared = 3.4028235e+38f; // 0x7f7fffff
        return live_count;
    }

    return -1;
}

#if 0
Original Ghidra decompilation (0x420de0):

int FUN_00420de0(short param_1)

{
  short sVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  undefined2 *puVar4;
  int unaff_EBX;
  short *unaff_EDI;

  sVar1 = *unaff_EDI;
  iVar2 = -1;
  iVar3 = 0;
  if (0 < sVar1) {
    do {
      if (*(int *)(in_EAX + 8 + (short)iVar3 * 0x1c) == unaff_EBX) {
        iVar2 = iVar3;
        if ((short)iVar3 != -1) {
          return iVar3;
        }
        break;
      }
      iVar3 = iVar3 + 1;
    } while ((short)iVar3 < sVar1);
  }
  if (sVar1 < param_1) {
    puVar4 = (undefined2 *)(sVar1 * 0x1c + in_EAX);
    *unaff_EDI = sVar1 + 1;
    *(undefined4 *)(puVar4 + 2) = 0xffffffff;
    *(undefined4 *)(puVar4 + 4) = 0xffffffff;
    *(undefined4 *)(puVar4 + 0xc) = 0xffffffff;
    *puVar4 = 0;
    *(undefined4 *)(puVar4 + 6) = 0;
    puVar4[8] = 0;
    *(undefined4 *)(puVar4 + 10) = 0x7f7fffff;
    iVar2 = CONCAT22(0xffff,sVar1);
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
