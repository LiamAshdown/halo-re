// hash_table_grow_freelist  (orphan pass 4: FUN_004f0620, no Ghidra name)
// address 0x4f0620, size 170 bytes
// name confidence: 0.75 (sole caller is hash_table_set_or_remove 0x4f0530, which calls it
//   exactly when the freelist is empty and the effect is to allocate one more node block and
//   thread its 50 nodes onto the freelist)
// rewrite confidence: 0.7 (block/node layout confirmed against objdump: an 8-byte block header
//   {hash_node *nodes; hash_node_block *next;} followed by a 600-byte GlobalAlloc holding 50
//   hash_node's of 0x0c bytes each, matching types/objects.h hash_node_block)
// evidence: types/objects.h hash_table, hash_node, hash_node_block; global 0x0063a0b0
//   PTR_GlobalAlloc_0063a0b0 (IAT thunk, same one used elsewhere in this codebase, e.g.
//   src/cache/cache_file_load.c). out/phase4/objects_types_notes.md: "The 600-byte node pool
//   divided by the 0x3c-per-iteration unroll gives 50 nodes of 0x0c bytes per block."
// register convention: table pointer in ESI, no other arguments -- confirmed by objdump; this
//   matches the ESI table pointer live at hash_table_set_or_remove's call site (0x4f0574),
//   which moves EAX into ESI before calling.
// blam-cc: hash_table_grow_freelist(hash_table *table /*ESI*/)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"


void hash_table_grow_freelist(hash_table *table)
{
    hash_node_block *block;
    hash_node *nodes;
    int32_t i;

    block = (hash_node_block *)GlobalAlloc(0, sizeof(hash_node_block));
    nodes = (hash_node *)GlobalAlloc(0, 600);

    block->nodes = nodes;
    block->next = table->blocks;
    table->blocks = block;

    for (i = 0; i < 50; i++) {
        hash_node *node = &block->nodes[i];
        node->key = -1;
        node->value = -1;
        node->next = table->freelist;
        table->freelist = node;
    }
}

#if 0
Original Ghidra decompilation (0x4f0620):

void hash_table_grow_freelist(void)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  HGLOBAL pvVar4;
  undefined4 *puVar5;
  int iVar6;
  int unaff_ESI;

  piVar3 = GlobalAlloc(0,8);
  pvVar4 = GlobalAlloc(0,600);
  *piVar3 = (int)pvVar4;
  piVar3[1] = *(int *)(unaff_ESI + 0x14);
  *(int **)(unaff_ESI + 0x14) = piVar3;
  iVar6 = 0x24;
  do {
    iVar2 = *piVar3;
    iVar1 = iVar6 + -0x24;
    *(undefined4 *)(iVar2 + iVar1) = 0xffffffff;
    *(undefined4 *)(iVar2 + 4 + iVar1) = 0xffffffff;
    *(undefined4 *)(iVar2 + iVar1 + 8) = *(undefined4 *)(unaff_ESI + 0x10);
    *(int *)(unaff_ESI + 0x10) = iVar2 + iVar1;
    iVar1 = *piVar3;
    *(undefined4 *)(iVar6 + -0x18 + iVar1) = 0xffffffff;
    *(undefined4 *)(iVar6 + -0x14 + iVar1) = 0xffffffff;
    iVar1 = iVar6 + -0x18 + iVar1;
    *(undefined4 *)(iVar1 + 8) = *(undefined4 *)(unaff_ESI + 0x10);
    *(int *)(unaff_ESI + 0x10) = iVar1;
    iVar1 = *piVar3;
    *(undefined4 *)(iVar6 + -0xc + iVar1) = 0xffffffff;
    *(undefined4 *)(iVar6 + -8 + iVar1) = 0xffffffff;
    iVar1 = iVar6 + -0xc + iVar1;
    *(undefined4 *)(iVar1 + 8) = *(undefined4 *)(unaff_ESI + 0x10);
    *(int *)(unaff_ESI + 0x10) = iVar1;
    puVar5 = (undefined4 *)(*piVar3 + iVar6);
    *puVar5 = 0xffffffff;
    puVar5[1] = 0xffffffff;
    puVar5[2] = *(undefined4 *)(unaff_ESI + 0x10);
    *(undefined4 **)(unaff_ESI + 0x10) = puVar5;
    puVar5 = (undefined4 *)(iVar6 + 0xc + *piVar3);
    *puVar5 = 0xffffffff;
    puVar5[1] = 0xffffffff;
    iVar6 = iVar6 + 0x3c;
    puVar5[2] = *(undefined4 *)(unaff_ESI + 0x10);
    *(undefined4 **)(unaff_ESI + 0x10) = puVar5;
  } while (iVar6 < 0x27c);
  return;
}
#endif
