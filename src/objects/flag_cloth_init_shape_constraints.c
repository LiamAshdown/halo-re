// flag_cloth_init_shape_constraints
// address 0x4fb770, size 207 bytes, called once from flag_new 0x4fb540
// name confidence: 0.75 (named directly in out/phase4/objects_types_notes.md: "Flag ... proved
//   by flag_new 0x4fb540, flag_cloth_init_shape_constraints 0x4fb770")
// rewrite confidence: 0.5
// evidence: types/tags.h Flag (trailing_edge_shape 0x04, trailing_edge_shape_offset 0x06,
//   width 0x0c, height 0x0e); resolved against flag_new's call site the same way as
//   flag_cloth_mark_border_cells.c (EDI still holds the tag pointer, unpushed; the single
//   pushed stack argument is the runtime flag entry).
// register convention: EDI -> tag; stack -> entry.
// blam-cc: EDI -> tag, stack -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void flag_cloth_stamp_region_split_flags(int16_t outer_start, Flag *tag, flag *entry,
                                                 int16_t inner_start, int16_t size, uint16_t split_code); // this module, 0x4fb840

void flag_cloth_init_shape_constraints(flag *entry, Flag *tag /*EDI*/)
    // blam-cc: EDI -> tag, stack -> entry
{
    int16_t shape = (int16_t)tag->trailing_edge_shape;

    if (shape != 0) {
        int16_t si;
        int32_t edge_count;

        if (shape == 3 || shape == 4) {
            si = tag->height - 1;
        } else {
            si = tag->height / 2;
        }

        edge_count = tag->width + tag->trailing_edge_shape_offset - si - 1;
        if (edge_count < 0) {
            edge_count = 0;
        }

        if (shape == 3) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 3);
        } else if (shape == 4) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 2);
        } else if (shape == 1) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 2);
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, si, si, 3);
        } else if (shape == 2) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 3);
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, si, si, 2);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fb770):

void FUN_004fb770(void)

{
  short sVar1;
  int unaff_EDI;

  sVar1 = *(short *)(unaff_EDI + 4);
  if (sVar1 != 0) {
    if (sVar1 == 3) {
      FUN_004fb840();
      return;
    }
    if (sVar1 == 4) {
      FUN_004fb840();
      return;
    }
    if (sVar1 == 1) {
      FUN_004fb840();
      FUN_004fb840();
      return;
    }
    if (sVar1 == 2) {
      FUN_004fb840();
      FUN_004fb840();
    }
  }
  return;
}

Disassembly (0x4fb840 args resolved from this), objdump -d -M intel bin/halo.exe:
  entry: mov ax,[edi+4] (shape); mov ebp,[esp+8] (entry, the one stack arg)
  shape==3 or 4: si = height-1; else si = height>>1 (arithmetic shift)
  edge_count = max(width + trailing_edge_shape_offset - si - 1, 0)   ; setl/dec/and idiom
  shape==3: call(in_AX=edge_count, tag, entry, inner_start=0, size=si, code=3)
  shape==4: call(in_AX=edge_count, tag, entry, inner_start=0, size=si, code=2)
  shape==1: call(edge_count, tag, entry, 0, si, 2); call(edge_count, tag, entry, si, si, 3)
  shape==2: call(edge_count, tag, entry, 0, si, 3); call(edge_count, tag, entry, si, si, 2)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
