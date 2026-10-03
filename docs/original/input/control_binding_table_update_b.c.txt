// control_binding_table_update_b  (orphan pass 4: FUN_004f39d0, no Ghidra name)
// address 0x4f39d0, size 179 bytes
// name confidence: 0.4 (out/phase4/objects_types_notes.md and src/objects/README.md already
//   refer to this exact address as "object_control_binding_table_update_b"; kept minus the
//   "object_" prefix since the function has moved out of that module)
// rewrite confidence: 0.35 (loop bounds and the count/flag table writes are confirmed against
//   the decompilation and match the sibling functions in this file; the indirect call through
//   PTR_LAB_004f3abc is not resolved, same caveat as control_binding_table_update_a.c)
// evidence: src/objects/README.md: "object_control_binding_table_update_a 0x4f3890 and
//   _update_b 0x4f39d0 index them through register arguments Ghidra lost, and both fall through
//   to jump tables (PTR_LAB_004f39bc, PTR_LAB_004f3abc)." Same control-binding region as
//   control_binding_table_update_a.c / control_binding_table_register_single.c /
//   control_binding_table_query.c (this pass).
// register convention: takes no visible parameters, same as control_binding_table_update_a.
// UNSURE (function-wide): the outer loop runs for word_selector = 0 and 1, an inner loop of 6
//   rows (0xa0-byte stride) and a middle count-index that runs in lockstep at 0x14-element
//   (0x50-byte) stride -- transliterated literally; the call through PTR_LAB_004f3abc
//   (0x004f3abc, per src/objects/README.md) is preserved as an unresolved indirect call.

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
extern uint32_t control_word_secondary; // 0x006f1cec
extern uint32_t control_word_primary;   // 0x006f1ce8
extern uint32_t control_binding_device_type; // 0x006f1cb8, 1..4

extern uint8_t g_control_binding_region_e0[0x3c0]; // base 0x008603e0, UNSURE: same region as the sibling control_binding_table_*.c files

void control_binding_table_update_b(void)
{
    int32_t word_selector = 0;
    do {
        int32_t count_offset = word_selector * 10;
        int32_t row = 0;
        int32_t row_offset = word_selector * 0x50;

        do {
            int32_t count = *(int32_t *)(g_control_binding_region_e0 + row_offset); // DAT_008603e0 offset
            uint32_t word = (word_selector != 1) ? control_word_primary : control_word_secondary;

            if ((word & 0xf) == 0) {
                // 0x4f3a1f..0x4f3a63: 0x004f3abc is this function's own switch table (entries
                // 0x4f3a3b, 0x4f3a37, 0x4f3a48, 0x4f3a41, then 0x4f3a57 = skip), not a callback
                // table: every registered entry whose value byte has the device type's bit set
                // (types 1..4: bits 1, 0, 3, 2) gets its flag byte set, and the walk goes on to
                // the next row. (Orphan pass 4 review: the draft called through it and returned.)
                uint8_t *entries = g_control_binding_region_e0 + row_offset + 0x14; // 0x008603f4
                int32_t i;

                for (i = 0; i < count; i++) {
                    uint8_t value = entries[i * 8 + 2];                               // 0x008603f6
                    uint8_t bit;

                    switch (control_binding_device_type) {
                    case 1: bit = (uint8_t)((value >> 1) & 1); break;
                    case 2: bit = (uint8_t)(value & 1); break;
                    case 3: bit = (uint8_t)((value >> 3) & 1); break;
                    case 4: bit = (uint8_t)((value >> 2) & 1); break;
                    default: bit = 0; break; // entry 5 (0x4f3a57) skips; others unchecked, UNSURE
                    }
                    if (bit) {
                        entries[i * 8] = 1;
                    }
                }
            } else {
                int32_t remaining = *(int32_t *)(g_control_binding_region_e0 + row_offset + 8); // DAT_008603e8 offset
                int32_t i = 0;
                if (count > 0) {
                    do {
                        if (remaining < 1) break;
                        *(int32_t *)(g_control_binding_region_e0 + row_offset + 4) =
                            *(int32_t *)(g_control_binding_region_e0 + row_offset + 4) + 1; // DAT_008603e4 offset
                        {
                            int32_t slot = i + count_offset;
                            remaining--;
                            i++;
                            *(uint8_t *)(g_control_binding_region_e0 + 0x14 + slot * 8) = 1; // DAT_008603f4 offset
                        }
                    } while (i < count);
                }
            }

            row++;
            row_offset += 0xa0;
            count_offset += 0x14;
        } while (row < 6);

        word_selector++;
    } while (word_selector <= 1);
}

#if 0
Original Ghidra decompilation (0x4f39d0):

void FUN_004f39d0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_8;

  iVar7 = 0;
  do {
    iVar8 = iVar7 * 10;
    local_8 = 0;
    iVar6 = iVar7 * 0x50;
    do {
      iVar2 = *(int *)((int)&DAT_008603e0 + iVar6);
      uVar3 = DAT_006f1cec;
      if (iVar7 != 1) {
        uVar3 = DAT_006f1ce8;
      }
      if ((uVar3 & 0xf) == 0) {
        if (0 < iVar2) {
                    /* WARNING: Could not recover jumptable at 0x004f3a30. Too many branches */
                    /* WARNING: Treating indirect jump as call */
          (*(code *)(&PTR_LAB_004f3abc)[DAT_006f1cb8 + -1])();
          return;
        }
      }
      else {
        iVar5 = *(int *)((int)&DAT_008603e8 + iVar6);
        iVar4 = 0;
        if (0 < iVar2) {
          do {
            if (iVar5 < 1) break;
            *(int *)((int)&DAT_008603e4 + iVar6) = *(int *)((int)&DAT_008603e4 + iVar6) + 1;
            iVar1 = iVar4 + iVar8;
            iVar5 = iVar5 + -1;
            iVar4 = iVar4 + 1;
            (&DAT_008603f4)[iVar1 * 8] = 1;
          } while (iVar4 < iVar2);
        }
      }
      local_8 = local_8 + 1;
      iVar6 = iVar6 + 0xa0;
      iVar8 = iVar8 + 0x14;
    } while (local_8 < 6);
    iVar7 = iVar7 + 1;
    if (1 < iVar7) {
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
