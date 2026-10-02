// ai_search_partition_into_groups  (Ghidra: ai_search_partition_into_groups, renamed)
// address 0x43cb60, size 159 bytes
// name confidence: 0.45  rewrite confidence: 0.85 (verified against objdump)
// evidence: types/ai.h ai_search_obstacle_list.count(+0x02)/obstacles(+0x08, stride 0x14) and
// ai_search_obstacle.link(+0x02); phase-4 summary "partitions the point graph into connected
// groups by repeatedly flood-filling from each ungrouped point." Calls
// ai_search_flood_fill_group (ai_search_flood_fill_group, this rewrite), which is called here with no
// visible arguments; the group bitmask it writes into is Ghidra's `local_10[4]`.
//
// UNSURE: unknown_00 (+0x00) is used here as a running "next group id" counter, and
// obstacle.link (documented as "the paired obstacle entry, or -1" elsewhere) is repurposed
// here as a temporary "which group is this point in" tag; neither use is stated in the
// header. ai_search_flood_fill_group is called with none of its own four parameters visible, forwarding
// this function's own `radius` (unaff-style) implicitly; called explicitly below with the
// values this function has in scope.
//
// register convention: ESI -> list (the only `unaff_` register Ghidra's decompile shows).
//   // blam-cc: ESI -> list, stack -> radius (inherited from the caller, not independently
//   //   recoverable at this call site)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void ai_search_flood_fill_group(ai_search_obstacle_list *list, float radius, uint32_t *out_bitmask,
                                       int16_t start_index); // 0x43ca40

// VERIFIED against disassembly 0x43cb60..0x43cbff (2026-09-30): link reset, group id increment before the flood fill,
//   the 4-dword group bitmask and the link assignment match.
// blam-cc: ESI -> list
void ai_search_partition_into_groups(ai_search_obstacle_list *list, float radius)
{
    uint32_t group_bitmask[4];
    int16_t i;

    list->group_count = 0;
    for (i = 0; i < list->count; i = i + 1) {
        list->obstacles[i].link = -1;
    }

    for (i = 0; i < list->count; i = i + 1) {
        if (list->obstacles[i].link == -1) {
            int16_t group_id = list->group_count;
            int16_t j;

            list->group_count = group_id + 1;
            ai_search_flood_fill_group(list, radius, group_bitmask, i);

            for (j = 0; j < list->count; j = j + 1) {
                if ((group_bitmask[j >> 5] & (1u << (j & 0x1f))) != 0) {
                    list->obstacles[j].link = group_id;
                }
            }
        }
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043cb60 @ 0x43cb60) ----
void FUN_0043cb60(void)

{
  short sVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  short *unaff_ESI;
  uint auStack_1010 [1018];
  uint local_10 [4];

  sVar2 = 0;
  *unaff_ESI = 0;
  if (0 < unaff_ESI[1]) {
    do {
      iVar3 = (int)sVar2;
      sVar2 = sVar2 + 1;
      unaff_ESI[iVar3 * 10 + 5] = -1;
    } while (sVar2 < unaff_ESI[1]);
  }
  sVar2 = 0;
  if (0 < unaff_ESI[1]) {
    do {
      if (unaff_ESI[sVar2 * 10 + 5] == -1) {
        sVar1 = *unaff_ESI;
        *unaff_ESI = sVar1 + 1;
        FUN_0043ca40();
        sVar4 = 0;
        if (0 < unaff_ESI[1]) {
          do {
            if ((local_10[(int)sVar4 >> 5] & 1 << ((byte)sVar4 & 0x1f)) != 0) {
              unaff_ESI[sVar4 * 10 + 5] = sVar1;
            }
            sVar4 = sVar4 + 1;
          } while (sVar4 < unaff_ESI[1]);
        }
      }
      sVar2 = sVar2 + 1;
    } while (sVar2 < unaff_ESI[1]);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
