// hs_parse_tag_reference  (Ghidra: FUN_00486ce0; renamed per types/hs.h, which already
// documents this address by this name in its Scenario offsets table: "0x4b4 / 0x4b8
// references count / pointer, stride 0x28 -- hs_parse_tag_reference (0x486ce0)")
// address 0x486ce0, size 216 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: linear scan of Scenario::references (ScenarioReference, stride 0x28) comparing the
// token text against each entry's TagDependency.path_pointer (offset 0x1c) and, on a name
// match, additionally requiring TagDependency.tag_fourcc (offset 0x18) to equal
// hs_tag_group_for_type[node->type - 0x18] (types/hs.h documents the 0x18-biased base address
// this reaches; folded into an array index here) before storing TagDependency.tag_id
// (offset 0x24) into the node.
// register convention: __cdecl, node_index is the recognized single stack parameter.
// UNSURE: every return path in the original returns 1/true, including "no references at all"
// and "scanned every reference, none matched" -- there is no failure return here at all; only
// whether node->data.tag_reference gets written differs. Preserved exactly rather than adding
// an error path that is not actually present in the compiled code.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *hs_syntax_data;                          // 0x0087a474
extern Scenario *global_scenario;                            // 0x00746f8c
extern char *hs_compiled_source;                              // 0x006b14c0
extern uint32_t hs_tag_group_for_type[8];                     // 0x00657544, types 0x18..0x1f

// Looks up a parsed enum/keyword token by exact name in Scenario::references and stores its
// TagID if found and its tag group matches the group expected for the node's type. Always
// returns 1 (see UNSURE above); a non-match simply leaves the node's data untouched.
char hs_parse_tag_reference(datum_index node_index)
{
    hs_syntax_node *node;
    int32_t count;
    int32_t i;
    ScenarioReference *reference;
    char *token_text;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * hs_syntax_data->size);
    count = (int32_t)global_scenario->references.count;
    token_text = hs_compiled_source + node->source_offset;
    for (i = 0; i < count; i = i + 1) {
        reference = (ScenarioReference *)global_scenario->references.pointer + i;
        if ((strcmp((char *)reference->reference.path_pointer, token_text) == 0) &&
            (reference->reference.tag_fourcc == hs_tag_group_for_type[node->type - 0x18])) {
            node->data.tag_reference = *(datum_index *)&reference->reference.tag_id;
            return 1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x486ce0):

undefined4 FUN_00486ce0(uint param_1)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  byte *pbVar4;
  int iVar5;
  int iVar6;
  byte *pbVar7;
  bool bVar8;

  iVar1 = *(int *)(DAT_0087a474 + 0x34) + (param_1 & 0xffff) * 0x14;
  iVar6 = *(int *)(DAT_00746f8c + 0x4b4);
  param_1 = 0;
  if (iVar6 < 1) {
LAB_00486db2:
    return CONCAT31((int3)((uint)iVar6 >> 8),1);
  }
  iVar3 = 0;
  do {
    iVar3 = *(int *)(DAT_00746f8c + 0x4b8) + iVar3 * 0x28;
    pbVar4 = *(byte **)(iVar3 + 0x1c);
    pbVar7 = (byte *)(DAT_006b14c0 + *(int *)(iVar1 + 0xc));
    do {
      bVar2 = *pbVar4;
      bVar8 = bVar2 < *pbVar7;
      if (bVar2 != *pbVar7) {
LAB_00486d74:
        iVar5 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
        goto LAB_00486d79;
      }
      if (bVar2 == 0) break;
      bVar2 = pbVar4[1];
      bVar8 = bVar2 < pbVar7[1];
      if (bVar2 != pbVar7[1]) goto LAB_00486d74;
      pbVar4 = pbVar4 + 2;
      pbVar7 = pbVar7 + 2;
    } while (bVar2 != 0);
    iVar5 = 0;
LAB_00486d79:
    if ((iVar5 == 0) &&
       (*(int *)(iVar3 + 0x18) == *(int *)(&DAT_006574e4 + *(short *)(iVar1 + 4) * 4))) {
      *(undefined4 *)(iVar1 + 0x10) = *(undefined4 *)(iVar3 + 0x24);
      iVar6 = iVar1;
      goto LAB_00486db2;
    }
    param_1 = param_1 + 1;
    iVar3 = (int)(short)param_1;
    if (iVar6 <= iVar3) {
      return CONCAT31((int3)(char)(param_1 >> 8),1);
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
