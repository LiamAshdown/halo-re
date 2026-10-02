// breakable_surfaces_reset  (was zone_light_table_initialize; renamed in reconciliation R79)
// address 0x4ffd40, size 90 bytes
// name confidence: 0.75 (types/objects.h names and cites this exact address in its
//   object_zone_light_table struct comment: "built by zone_light_table_initialize")
// rewrite confidence: 0.85
// evidence: types/objects.h object_zone_light_table (valid 0x0000, membership[16][8] 0x0001,
//   weights[16][256] 0x0204); global 0x006b8d78 object_zone_light_table_pointer.
// register convention: none (no parameters).
// reconciled: R79 0x006b8d78 is types/physics.h breakable_surface_globals (same 0x4204 layout): object_zone_light_table -> breakable_surface_globals, valid -> initialized, membership -> active (intact-surface bits), weights -> health. This is the breakable-surface reset, not a zone light table

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, types/physics.h breakable_surface_globals

void breakable_surfaces_reset(void)
{
    breakable_surface_globals *table = breakable_surface_state;
    int32_t group, i;

    table->initialized = 1;

    for (group = 0; group < 16; group++) {
        for (i = 0; i < 8; i++) {
            table->active[group][i] = 0xffffffff;
        }
    }
    for (group = 0; group < 16; group++) {
        for (i = 0; i < 256; i++) {
            table->health[group][i] = 1.0f;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
