// zone_light_table_initialize
// address 0x4ffd40, size 90 bytes
// name confidence: 0.75 (types/objects.h names and cites this exact address in its
//   object_zone_light_table struct comment: "built by zone_light_table_initialize")
// rewrite confidence: 0.85
// evidence: types/objects.h object_zone_light_table (valid 0x0000, membership[16][8] 0x0001,
//   weights[16][256] 0x0204); global 0x006b8d78 object_zone_light_table_pointer.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object_zone_light_table *object_zone_light_table_pointer; // 0x006b8d78

void zone_light_table_initialize(void)
{
    object_zone_light_table *table = object_zone_light_table_pointer;
    int32_t group, i;

    table->valid = 1;

    for (group = 0; group < 16; group++) {
        for (i = 0; i < 8; i++) {
            table->membership[group][i] = 0xffffffff;
        }
    }
    for (group = 0; group < 16; group++) {
        for (i = 0; i < 256; i++) {
            table->weights[group][i] = 1.0f;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ffd40):

void FUN_004ffd40(void)

{
  undefined1 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;

  puVar1 = DAT_006b8d78;
  *DAT_006b8d78 = 1;
  puVar3 = (undefined4 *)(puVar1 + 1);
  iVar4 = 0x10;
  puVar5 = (undefined4 *)(puVar1 + 0x204);
  do {
    *puVar3 = 0xffffffff;
    puVar3[1] = 0xffffffff;
    puVar3[2] = 0xffffffff;
    puVar3[3] = 0xffffffff;
    puVar3[4] = 0xffffffff;
    puVar3[5] = 0xffffffff;
    puVar3[6] = 0xffffffff;
    puVar3[7] = 0xffffffff;
    puVar3 = puVar3 + 8;
    iVar4 = iVar4 + -1;
    puVar6 = puVar5;
    for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar6 = 0x3f800000;
      puVar6 = puVar6 + 1;
    }
    puVar5 = puVar5 + 0x100;
  } while (iVar4 != 0);
  return;
}
#endif
