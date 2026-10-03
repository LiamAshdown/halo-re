// control_binding_table_update_a  (orphan pass 4: FUN_004f3890, no Ghidra name)
// address 0x4f3890, size 260 bytes
// name confidence: 0.4 (out/phase4/objects_types_notes.md and src/objects/README.md already
//   refer to this exact address as "object_control_binding_table_update_a" when describing the
//   object control binding tables; this pass keeps that established name, minus the "object_"
//   prefix, since the function has moved out of that module)
// rewrite confidence: 0.3 (the per-row count/id/value table writes match
//   control_binding_table_register_single.c's LAB_004f396d path exactly, and the outer loop
//   bounds are confirmed against objdump; the indirect calls through PTR_LAB_004f39bc are not
//   resolved -- neither their targets nor their calling convention -- so that half of the
//   function is transliterated as a raw function-pointer call, not a named dispatch)
// evidence: out/phase4/objects_types_notes.md, "Not objects-module code": "0x4f3680 / 0x4f3700
//   / 0x4f37d0 / 0x4f3890 / 0x4f39d0 / 0x4f3ad0 read the packed control words at 0x006f1cec and
//   0x006f1ce8, which belong to the input or game module." src/objects/README.md: "The object
//   control binding tables (0x008603e0..0x008603fc, 0x00860430/0x00860434, 0x00860480/
//   0x00860484) are parallel arrays with bases 4 to 8 bytes apart, not one struct. No type is
//   claimed; the addresses are recorded as globals only. ... object_control_binding_table_update_a
//   0x4f3890 and _update_b 0x4f39d0 index them through register arguments Ghidra lost, and both
//   fall through to jump tables (PTR_LAB_004f39bc, PTR_LAB_004f3abc)."
// register convention: takes no visible parameters (Ghidra shows none, and objdump's prologue
//   reads no live-in register before the first store); every input comes from the global
//   control-binding tables it walks.
// UNSURE (function-wide): the parallel min/max-threshold tables at 0x008603e0/0x008603e4/
//   0x008603e8/0x00860430/0x00860434 (compared against each other to pick which of two
//   candidate row offsets, iVar6 0 or 1, to write into) are transliterated as raw int32 reads
//   at their Ghidra-given byte offsets; their real meaning (device axis calibration bounds?) is
//   not established. The call through PTR_LAB_004f39bc (device-type-indexed function pointer
//   table at 0x004f39bc, 4 entries per out/phase4/objects_types_notes.md) is preserved as an
//   unresolved indirect call; its argument and return value are not recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t control_word_primary; // 0x006f1ce8
extern uint32_t control_binding_device_type; // 0x006f1cb8, 1..4

extern uint8_t g_control_binding_region_e0[0x3c0]; // base 0x008603e0, UNSURE: same region as the other control_binding_table_*.c files

void control_binding_table_update_a(void)
{
    uint32_t low_nibble = control_word_primary & 0xf;
    int32_t row_offset = 0;
    int32_t pair_index = 0;

    do {
        if (low_nibble == 0) {
            // 0x4f38b5..0x4f3910. 0x004f39bc is this function's own switch table, not a callback
            // table: for each of the row's two cells, every registered entry whose value byte
            // has the device type's bit set gets its flag byte (+0x14) set. Types 1..4 test bits
            // 1, 0, 3, 2 (table entries 0x4f38dd, 0x4f38d9, 0x4f38ea, 0x4f38e3). (Orphan pass 4
            // review: the draft called through the table as function pointers and returned.)
            int32_t sub;
            for (sub = 0; sub < 2; sub++) {
                uint8_t *cell = g_control_binding_region_e0 + row_offset + sub * 0x50;
                int32_t count = ((control_binding_half *)cell)->entry_count;              // 0x008603e0
                int32_t i;

                for (i = 0; i < count; i++) {
                    uint8_t value = ((control_binding_half *)cell)->entries[i].device_mask;     // 0x008603f6
                    uint8_t bit;

                    switch (control_binding_device_type) {
                    case 1: bit = (uint8_t)((value >> 1) & 1); break;
                    case 2: bit = (uint8_t)(value & 1); break;
                    case 3: bit = (uint8_t)((value >> 3) & 1); break;
                    case 4: bit = (uint8_t)((value >> 2) & 1); break;
                    default: bit = 0; break; // UNSURE: the original indexes the table unchecked
                    }
                    if (bit) {
                        ((control_binding_half *)cell)->entries[i].selected = 1;                // 0x008603f4
                    }
                }
            }
        } else {
            uint8_t *base = g_control_binding_region_e0 + row_offset;
            int32_t remaining = ((control_binding_half *)base)->limit; // DAT_008603e8 offset
            int32_t settled = 0;

            while (remaining > 0 && !settled) {
                uint32_t a = (uint32_t)((control_binding_half *)base)->selected_count;       // DAT_008603e4 offset
                uint32_t b = (uint32_t)((control_binding_half *)(g_control_binding_region_e0 + 0x50 + row_offset))->selected_count; // DAT_00860434 offset
                int32_t pick;
                int32_t has_pick = 0;

                settled = 1;
                if (b < a) {
                    if (b < (uint32_t)((control_binding_half *)(g_control_binding_region_e0 + 0x50 + row_offset))->entry_count) { // DAT_00860430
                        pick = 1; has_pick = 1;
                    } else if (a < (uint32_t)((control_binding_half *)base)->entry_count) { // DAT_008603e0
                        pick = 0; has_pick = 1;
                    }
                } else {
                    if (a < (uint32_t)((control_binding_half *)base)->entry_count) { // DAT_008603e0
                        pick = 0; has_pick = 1;
                    } else if (b < (uint32_t)((control_binding_half *)(g_control_binding_region_e0 + 0x50 + row_offset))->entry_count) { // DAT_00860430
                        pick = 1; has_pick = 1;
                    }
                }

                if (has_pick) {
                    int32_t idx = pick + pair_index;
                    // UNSURE: Ghidra resolves this specific count cell against DAT_008603e4
                    // (base+4), not DAT_008603e0 (base+0) as control_binding_table_update_b.c
                    // and control_binding_table_register_single.c use for what looks like the
                    // same role. Preserved exactly as decompiled rather than reconciled.
                    int32_t *count_cell = (int32_t *)(g_control_binding_region_e0 + 4 + idx * 0x50);
                    int32_t slot = *count_cell + idx * 10;
                    *count_cell = *count_cell + 1;
                    settled = 0;
                    *(uint8_t *)(g_control_binding_region_e0 + 0x14 + slot * 8) = 1; // DAT_008603f4 offset
                    remaining--;
                }
            }
        }

        row_offset += 0xa0;
        pair_index += 2;
    } while (row_offset <= 0x3bf);
}

#if 0
Original Ghidra decompilation (0x4f3890):

void FUN_004f3890(void)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  bool bVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined *puVar9;
  int local_c;
  int local_8;

  uVar5 = DAT_006f1ce8 & 0xf;
  local_8 = 0;
  iVar7 = 0;
  do {
    if (uVar5 == 0) {
      puVar9 = &DAT_008603f6 + iVar7;
      local_c = 0;
      do {
        if (0 < *(int *)(puVar9 + -0x16)) {
                    /* WARNING: Could not recover jumptable at 0x004f38d2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
          (*(code *)(&PTR_LAB_004f39bc)[DAT_006f1cb8 + -1])();
          return;
        }
        local_c = local_c + 1;
        puVar9 = puVar9 + 0x50;
      } while (local_c < 2);
    }
    else {
      iVar8 = *(int *)((int)&DAT_008603e8 + iVar7);
      bVar4 = false;
      while ((0 < iVar8 && (!bVar4))) {
        uVar1 = *(uint *)((int)&DAT_008603e4 + iVar7);
        uVar2 = *(uint *)(&DAT_00860434 + iVar7);
        bVar4 = true;
        if (uVar2 < uVar1) {
          if (uVar2 < *(uint *)((int)&DAT_00860430 + iVar7)) {
            iVar6 = 1;
            goto LAB_004f396d;
          }
          if (uVar1 < *(uint *)((int)&DAT_008603e0 + iVar7)) {
LAB_004f396b:
            iVar6 = 0;
LAB_004f396d:
            iVar6 = iVar6 + local_8;
            iVar3 = (&DAT_008603e4)[iVar6 * 0x14];
            (&DAT_008603e4)[iVar6 * 0x14] = iVar3 + 1;
            bVar4 = false;
            (&DAT_008603f4)[(iVar3 + iVar6 * 10) * 8] = 1;
            iVar8 = iVar8 + -1;
          }
        }
        else {
          if (uVar1 < *(uint *)((int)&DAT_008603e0 + iVar7)) goto LAB_004f396b;
          if (uVar2 < *(uint *)((int)&DAT_00860430 + iVar7)) {
            iVar6 = 1;
            goto LAB_004f396d;
          }
        }
      }
    }
    iVar7 = iVar7 + 0xa0;
    local_8 = local_8 + 2;
    if (0x3bf < iVar7) {
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
