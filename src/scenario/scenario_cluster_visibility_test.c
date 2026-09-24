// scenario_cluster_visibility_test  (Ghidra: FUN_0053eb60, still unnamed there; renamed from
// out/phase2/results/scenario_00.json's "structure_bsp_cluster_visibility_test" to match the
// "scenario_cluster_*" family out/phase4/scenario_types_notes.md places this function in)
// address 0x53eb60, size 65 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/scenario_types_notes.md / out/phase2/results/scenario_00.json: "Tests bit
// ((param_1 * row_words) + (in_CX>>5)) of a bit-vector at global_structure_bsp+0x14c sized from
// the cluster count at +0x134, the classic NxN cluster-to-cluster visibility/portal bit-matrix
// layout." types/tags.h confirms global_structure_bsp->clusters.count (+0x134) and
// ->cluster_data.pointer (+0x14c, the TagDataOffset's own .pointer sub-field lands exactly here)
// match the two globals this function reads.
// register convention: CX -> column_cluster, stack -> row_cluster (int16). Returns a bool.
//   // blam-cc: CX -> column_cluster, stack -> row_cluster

#include "tags.h"
#include "scenario.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

// blam-cc: CX -> column_cluster, stack -> row_cluster
// Tests whether column_cluster is visible from row_cluster in the resident structure bsp's
// cluster-by-cluster PVS bit matrix (one row of ceil(clusters.count / 32) dwords per cluster).
uint8_t scenario_cluster_visibility_test(int16_t row_cluster, int16_t column_cluster)
{
    int32_t row_words = ((int32_t)global_structure_bsp->clusters.count + 0x1f) >> 5;
    uint32_t *pvs = (uint32_t *)global_structure_bsp->cluster_data.pointer;
    int32_t word_index = row_words * (int32_t)row_cluster + (column_cluster >> 5);

    return (pvs[word_index] & (1u << (column_cluster & 0x1f))) != 0;
}

#if 0
Original Ghidra decompilation (0x53eb60):

bool FUN_0053eb60(short param_1)

{
  short in_CX;

  return (*(uint *)(*(int *)(DAT_00746f9c + 0x14c) +
                   ((*(int *)(DAT_00746f9c + 0x134) + 0x1f >> 5) * (int)param_1 + ((int)in_CX >> 5))
                   * 4) & 1 << ((byte)in_CX & 0x1f)) != 0;
}

Raw disassembly (0x53eb60-0x53eba0):

  53eb60: mov    eax,ds:0x746f9c
  53eb65: mov    edx,DWORD PTR [eax+0x134]     ; clusters.count
  53eb6b: mov    eax,DWORD PTR [eax+0x14c]     ; cluster_data.pointer
  53eb71: push   esi
  53eb72: movsx  esi,WORD PTR [esp+0x8]        ; row_cluster (stack arg)
  53eb77: add    edx,0x1f
  53eb7a: sar    edx,0x5                       ; row_words
  53eb7d: imul   edx,esi
  53eb80: movsx  ecx,cx                        ; column_cluster
  53eb83: mov    esi,ecx
  53eb85: sar    esi,0x5
  53eb88: add    edx,esi                       ; word_index
  53eb8a: mov    eax,DWORD PTR [eax+edx*4]
  53eb8d: and    ecx,0x1f
  53eb90: mov    esi,0x1
  53eb95: shl    esi,cl
  53eb97: and    eax,esi
  53eb99: neg    eax
  53eb9b: sbb    eax,eax
  53eb9d: neg    eax
  53eb9f: pop    esi
  53eba0: ret
#endif
