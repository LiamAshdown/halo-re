// flag_cloth_stamp_region_split_flags
// address 0x4fb840, size 290 bytes, callers=2 (flag_cloth_mark_border_cells 0x4fb6d0,
//   flag_cloth_init_shape_constraints 0x4fb770, both in this same file group)
// name confidence: 0.45 (still FUN_004fb840 in Ghidra; functions.md: "Stamps a rectangular
//   region of the flag cloth grid with a quad-diagonal-split code later used when triangulating
//   the cloth mesh for rendering")
// rewrite confidence: 0.55 (arithmetic resolved directly from Ghidra's decompile, which already
//   had a clean explicit parameter list; only the calling convention -- which register carries
//   which argument -- needed disassembly to confirm, since both call sites show zero visible
//   arguments in Ghidra's output)
// evidence: types/objects.h flag (cell_split_codes 0x1534); types/tags.h Flag (width 0x0c,
//   height 0x0e); resolved against both call sites by disassembling 0x4fb6d0 and 0x4fb770
//   (objdump -d -M intel bin/halo.exe).
// register convention: EAX -> outer_start (Ghidra's in_AX), stack -> tag, entry, inner_start,
//   size, split_code (in that order, matching both call sites' push order).
// blam-cc: EAX -> outer_start, stack -> tag, entry, inner_start, size, split_code

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

void flag_cloth_stamp_region_split_flags(int16_t outer_start /*EAX*/, Flag *tag, flag *entry,
                                          int16_t inner_start, int16_t size, uint16_t split_code)
    // blam-cc: EAX -> outer_start, stack -> tag, entry, inner_start, size, split_code
{
    int16_t outer;

    for (outer = outer_start; outer < size + outer_start; outer++) {
        int16_t inner;

        for (inner = inner_start; inner < size + inner_start; inner++) {
            if (outer >= 0 && inner >= 0 &&
                outer < tag->width - 1 && inner < tag->height - 1) {
                int16_t a, b;
                uint16_t *cell;

                if (split_code == 4 || split_code == 5) {
                    a = outer - outer_start;
                } else {
                    a = (size - outer) - 1 + outer_start;
                }
                if (split_code == 4 || split_code == 2) {
                    b = inner - inner_start;
                } else {
                    b = (size - inner) - 1 + inner_start;
                }

                cell = &entry->cell_split_codes[(tag->height - 1) * outer + inner];
                if (a == b) {
                    *cell = split_code;
                } else {
                    *cell = (uint16_t)(a <= b);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fb840):

void FUN_004fb840(int param_1,int param_2,short param_3,short param_4,ushort param_5)

{
  ushort *puVar1;
  int iVar2;
  short sVar3;
  short in_AX;
  short sVar4;
  int iVar5;
  short sVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  int iVar10;

  iVar8 = (int)in_AX;
  if (iVar8 < param_4 + iVar8) {
    iVar9 = (int)param_3;
    iVar2 = iVar9;
    sVar3 = param_3;
    sVar4 = in_AX;
    iVar10 = iVar8;
    do {
      while (iVar2 < iVar9 + param_4) {
        if ((((-1 < sVar4) && (-1 < sVar3)) && (iVar10 < *(short *)(param_1 + 0xc) + -1)) &&
           (iVar5 = *(short *)(param_1 + 0xe) + -1, iVar2 < iVar5)) {
          if ((param_5 == 4) || (param_5 == 5)) {
            sVar7 = (short)iVar10 - in_AX;
          }
          else {
            sVar7 = (param_4 - (short)iVar10) + -1 + in_AX;
          }
          if ((param_5 == 4) || (param_5 == 2)) {
            sVar6 = (short)iVar2 - param_3;
          }
          else {
            sVar6 = (param_4 - (short)iVar2) + -1 + param_3;
          }
          puVar1 = (ushort *)(param_2 + 0x1534 + (iVar5 * iVar10 + iVar2) * 2);
          if (sVar7 == sVar6) {
            *puVar1 = param_5;
          }
          else {
            *puVar1 = (ushort)(sVar7 <= sVar6);
          }
        }
        iVar2 = (int)(short)(sVar3 + 1);
        sVar3 = sVar3 + 1;
      }
      sVar4 = sVar4 + 1;
      iVar10 = (int)sVar4;
      iVar2 = iVar9;
      sVar3 = param_3;
    } while (iVar10 < param_4 + iVar8);
  }
  return;
}
#endif
