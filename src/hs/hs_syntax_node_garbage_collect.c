// hs_syntax_node_garbage_collect  (Ghidra: hs_syntax_node_garbage_collect, already named)
// address 0x483310, size 128 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: walks every live datum in hs_syntax_data via the same *0x14 stride
// (datum_next/datum_delete pair, 0x4d0630/0x4d0510, already named in the memory module) and
// deletes every node whose flags byte does not carry hs_syntax_node_flags bit 3
// (_hs_syntax_node_garbage_collectable_bit, types/hs.h), which types/hs.h documents as
// "the only bit this function keeps".
// register convention: __cdecl, no parameters.
// UNSURE: the two nested loops are Ghidra's inlined unrolling of datum_next's own body
// (compare src/memory/datum_next.c); they are kept inline here, matching the compiled shape,
// rather than rewritten as a call to datum_next, to preserve the exact instruction-level
// control flow.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x004d0630
extern void datum_delete(data_array *array, datum_index handle); // 0x004d0510

extern data_array *hs_syntax_data; // 0x0087a474

// Deletes every hs_syntax_node not marked collectable (flags bit 3 clear) out of hs_syntax_data.
void hs_syntax_node_garbage_collect(void)
{
    data_array *nodes;
    datum_index current;
    hs_syntax_node *node;
    int32_t next_start;
    int16_t next_index;
    hs_syntax_node *scan;

    nodes = hs_syntax_data;
    current = datum_next(-1, nodes);
    for (;;) {
        for (;;) {
            if (current == k_datum_index_none) {
                return;
            }
            node = (hs_syntax_node *)((uint8_t *)nodes->data + (current & 0xffff) * nodes->size);
            if ((node->flags & _hs_syntax_node_garbage_collectable_bit) == 0) {
                datum_delete(nodes, current);
            }
            next_start = (int32_t)(current & 0xffff) + 1;
            current = k_datum_index_none;
            next_index = (int16_t)next_start;
            if (0 <= next_index && next_index < nodes->last_index) {
                break;
            }
        }
        scan = (hs_syntax_node *)((uint8_t *)nodes->data + (int32_t)next_index * nodes->size);
        for (;;) {
            if (scan->identifier != 0) {
                current = ((uint32_t)(uint16_t)scan->identifier << 16) | (uint16_t)next_index;
                break;
            }
            next_start = next_start + 1;
            scan = (hs_syntax_node *)((uint8_t *)scan + nodes->size);
            next_index = (int16_t)next_start;
            if (nodes->last_index <= next_index) {
                break;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x483310):

void __cdecl hs_syntax_node_garbage_collect(void)

{
  int iVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;

  iVar1 = DAT_0087a474;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      if ((*(byte *)(*(int *)(iVar1 + 0x34) + 6 + (uVar2 & 0xffff) * 0x14) & 8) == 0) {
        datum_delete();
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
    } while ((sVar4 < 0) || (*(short *)(iVar1 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar1 + 0x22));
    } while ((short)iVar5 < *(short *)(iVar1 + 0x2e));
  } while( true );
}
#endif
