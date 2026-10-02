// unit_control_data_unpack  (Ghidra: FUN_00449fd0; renamed per types notes)
// address 0x449fd0, size 136 bytes
// name confidence: 0.8 (symbols/agent_phase4_cutscene.txt)   rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md "0x449fd0 (EBX = unit_control_data *, stack:
// cursor **, uint8 version) zeroes the 0x40 bytes, sets zoom_level to -1, then for each table
// 0..max(version,1)-1 copies size bytes from the cursor to offset (skipping the copy when
// offset is -1) and advances the cursor by size." types/cutscene.h documents the four
// unit_control_data_field_layout tables behind unit_control_data_version_layouts[4].
// register convention: EBX = control (unaff_EBX); stack (cursor, version) in Ghidra's own
// param_1/param_2 order. blam-cc: EBX -> control, stack -> (cursor, version).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern unit_control_data_field_layout *unit_control_data_version_layouts[4]; // 0x00686d88

// blam-cc: EBX -> control, stack -> (cursor, version)
// Zeroes control, sets its zoom_level to -1 (the v4-only field), then unpacks the stream at
// *cursor into it one field-layout table at a time (table 0 always applies; version selects
// how many more follow), copying each field's bytes to its destination offset -- or, when the
// table marks an offset -1, only skipping the cursor past it -- until a table's terminator
// record (size -1) is reached.
void unit_control_data_unpack(unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    int32_t i;
    int32_t table_count;
    int32_t table_index;

    for (i = 0; i < (int32_t)(sizeof(unit_control_data) / 4); i += 1) {
        ((uint32_t *)control)[i] = 0;
    }
    control->zoom_level = -1;

    table_count = (version < 2) ? 1 : (int32_t)version;
    for (table_index = 0; table_index < table_count; table_index += 1) {
        unit_control_data_field_layout *record = unit_control_data_version_layouts[table_index];

        while (record->size != -1) {
            if (record->offset != -1) {
                uint8_t *src = *cursor;
                uint8_t *dst = (uint8_t *)control + record->offset;
                int32_t size = record->size;
                int32_t dwords = size >> 2;
                int32_t bytes = size & 3;

                while (dwords != 0) {
                    *(uint32_t *)dst = *(uint32_t *)src;
                    src += 4;
                    dst += 4;
                    dwords -= 1;
                }
                while (bytes != 0) {
                    *dst = *src;
                    src += 1;
                    dst += 1;
                    bytes -= 1;
                }
            }
            *cursor += record->size;
            record += 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x449fd0):

void FUN_00449fd0(int *param_1,byte param_2)

{
  short sVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  undefined4 *unaff_EBX;
  undefined4 *puVar6;
  undefined4 *puVar7;

  puVar6 = unaff_EBX;
  for (iVar3 = 0x10; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  *(undefined2 *)(unaff_EBX + 2) = 0xffff;
  sVar1 = 0;
  while( true ) {
    uVar4 = (uint)param_2;
    if (param_2 < 2) {
      uVar4 = 1;
    }
    if ((int)uVar4 <= (int)sVar1) break;
    puVar2 = (uint *)((&PTR_PTR_00686d88)[sVar1] + 4);
    uVar4 = *(uint *)((&PTR_PTR_00686d88)[sVar1] + 4);
    while (uVar4 != 0xffffffff) {
      if (puVar2[1] != 0xffffffff) {
        uVar4 = *puVar2;
        puVar6 = (undefined4 *)*param_1;
        puVar7 = (undefined4 *)(puVar2[1] + (int)unaff_EBX);
        for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
          *puVar7 = *puVar6;
          puVar6 = puVar6 + 1;
          puVar7 = puVar7 + 1;
        }
        for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
          *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
          puVar6 = (undefined4 *)((int)puVar6 + 1);
          puVar7 = (undefined4 *)((int)puVar7 + 1);
        }
      }
      uVar4 = *puVar2;
      puVar2 = puVar2 + 3;
      *param_1 = *param_1 + uVar4;
      uVar4 = *puVar2;
    }
    sVar1 = sVar1 + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
