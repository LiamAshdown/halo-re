// ai_search_flood_fill_group  (Ghidra: ai_search_flood_fill_group, renamed)
// address 0x43ca40, size 285 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (verified against objdump)
// evidence: types/ai.h ai_search_obstacle_list.count(+0x02)/obstacles(+0x08, stride 0x14)
// and ai_search_obstacle.position(+0x08)/radius(+0x10). phase-4 summary "flood-fills the set
// of point-graph indices reachable from a starting point through pairwise proximity links."
// register convention: DX -> start_index; stack -> list, radius, out_bitmask.
//   // blam-cc: EDX -> start_index, stack -> list, radius, out_bitmask
//
// VERIFIED against disassembly 0x43ca40..0x43cb5c (2026-09-30): frame, bitmask clear, worklist, radius sum order and the
//   <= compare match (the 128-entry int16 worklist is the [esp+0x10] area).
// UNSURE: the `asStack_10100[32760]` stack array Ghidra shows in the original is never read
// or written anywhere in the decompiled body and is omitted here as dead/unused stack space
// (a 128-entry `int16_t` worklist, `local_100`, is the one actually used).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: EDX -> start_index, stack -> list, radius, out_bitmask
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void ai_search_flood_fill_group(ai_search_obstacle_list *list, float radius, uint32_t *out_bitmask, int16_t start_index)
{
    int16_t worklist[128];
    int16_t worklist_count;
    int32_t words = (list->count + 0x1f) >> 5;
    int32_t i;

    for (i = 0; i < words; i = i + 1) {
        out_bitmask[i] = 0;
    }

    if (start_index != -1) {
        worklist[0] = start_index;
        worklist_count = 1;
        out_bitmask[start_index >> 5] |= 1u << (start_index & 0x1f);

        do {
            int16_t j;
            worklist_count = worklist_count - 1;
            {
                ai_search_obstacle *cur = &list->obstacles[worklist[worklist_count]];

                for (j = 0; j < list->count; j = j + 1) {
                    uint32_t bit = 1u << (j & 0x1f);
                    if ((out_bitmask[j >> 5] & bit) == 0) {
                        float combined_radius = (radius + list->obstacles[j].radius) + (radius + cur->radius); // 0x43caec..0x43cb08 order
                        float dx = list->obstacles[j].position.x - cur->position.x;
                        float dy = list->obstacles[j].position.y - cur->position.y;
                        if (dy * dy + dx * dx <= combined_radius * combined_radius) {
                            out_bitmask[j >> 5] |= bit;
                            worklist[worklist_count] = j;
                            worklist_count = worklist_count + 1;
                        }
                    }
                }
            }
        } while (0 < worklist_count);
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043ca40 @ 0x43ca40) ----
void FUN_0043ca40(int param_1,float param_2,undefined4 *param_3)

{
  int iVar1;
  uint *puVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  short in_DX;
  short sVar9;
  short sVar10;
  undefined4 *puVar11;
  short asStack_10100 [32760];
  short local_100 [128];

  puVar11 = param_3;
  for (uVar7 = *(short *)(param_1 + 2) + 0x1f >> 5 & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
    *(undefined1 *)puVar11 = 0;
    puVar11 = (undefined4 *)((int)puVar11 + 1);
  }
  if (in_DX != -1) {
    sVar10 = 1;
    param_3[(int)in_DX >> 5] = param_3[(int)in_DX >> 5] | 1 << ((byte)in_DX & 0x1f);
    do {
      sVar10 = sVar10 + -1;
      sVar9 = 0;
      iVar8 = param_1 + 8 + local_100[sVar10] * 0x14;
      if (0 < *(short *)(param_1 + 2)) {
        do {
          iVar6 = (int)sVar9;
          uVar7 = 1 << ((byte)sVar9 & 0x1f);
          puVar2 = param_3 + (iVar6 >> 5);
          if (((*puVar2 & uVar7) == 0) &&
             (iVar1 = param_1 + 8 + iVar6 * 0x14,
             fVar3 = param_2 + *(float *)(iVar8 + 0x10) +
                     param_2 + *(float *)(param_1 + 0x18 + iVar6 * 0x14),
             fVar4 = *(float *)(iVar1 + 8) - *(float *)(iVar8 + 8),
             fVar5 = *(float *)(iVar1 + 0xc) - *(float *)(iVar8 + 0xc),
             fVar5 * fVar5 + fVar4 * fVar4 <= fVar3 * fVar3)) {
            *puVar2 = *puVar2 | uVar7;
            local_100[sVar10] = sVar9;
            sVar10 = sVar10 + 1;
          }
          sVar9 = sVar9 + 1;
        } while (sVar9 < *(short *)(param_1 + 2));
      }
    } while (0 < sVar10);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
