// flag_cloth_mark_border_cells
// address 0x4fb6d0, size 159 bytes, called once from flag_new 0x4fb540
// name confidence: 0.5 (still FUN_004fb6d0 in Ghidra; functions.md: "Marks the border cells
//   along each row of the flag's cloth grid, used to set up edge constraints for the cloth
//   simulation")
// rewrite confidence: 0.5
// evidence: types/tags.h Flag (attached_edge_shape 0x08, height 0x0e, attachment_points
//   TagReflexive 0x54/0x58); resolved against flag_new's call site (objdump -d -M intel
//   bin/halo.exe, 0x4fb540..0x4fb6d0: `push esi; call 0x4fb6d0` with EDI still holding the Flag
//   tag pointer flag_new loaded earlier, unpushed).
// register convention: EDI -> tag, carried over unmodified from flag_new's own frame (this
//   function never reloads it); stack -> entry (the one value flag_new actually pushes).
// blam-cc: EDI -> tag, stack -> entry
// UNSURE: the per-attachment-point record read here (`*(int16_t *)(attachment_points.pointer +
//   i * 0x34)`) has no named type in this module; FlagAttachmentPoint is out of scope, kept as
//   a raw offset read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void flag_cloth_stamp_region_split_flags(int16_t outer_start, Flag *tag, flag *entry,
                                                 int16_t inner_start, int16_t size, uint16_t split_code); // this module, 0x4fb840

void flag_cloth_mark_border_cells(flag *entry, Flag *tag /*EDI*/)
    // blam-cc: EDI -> tag, stack -> entry
{
    if (tag->attached_edge_shape != 0 && (int32_t)tag->attachment_points.count > 0) {
        int16_t col = 0;
        int32_t point_index = 0;

        do {
            int16_t raw, clamped, half, split;

            if (tag->height <= col) {
                return;
            }

            raw = *(int16_t *)((uint8_t *)tag->attachment_points.pointer + point_index * 0x34);
            if (raw < 0) {
                clamped = 0;
            } else {
                int16_t remaining = tag->height - col;
                clamped = (remaining < raw) ? remaining : raw;
            }
            half = clamped & ~1;   // force even
            split = half / 2;

            flag_cloth_stamp_region_split_flags(0, tag, entry, col, split, 4);
            flag_cloth_stamp_region_split_flags(0, tag, entry, col + split, split, 5);

            col = col + half;
            point_index = point_index + 1;
        } while (point_index < (int32_t)tag->attachment_points.count);
    }
}

#if 0
Original Ghidra decompilation (0x4fb6d0):

void FUN_004fb6d0(void)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int unaff_EDI;

  iVar5 = 0;
  if ((*(short *)(unaff_EDI + 8) != 0) && (sVar2 = 0, 0 < *(int *)(unaff_EDI + 0x54))) {
    do {
      if (*(short *)(unaff_EDI + 0xe) <= (short)iVar5) {
        return;
      }
      sVar1 = *(short *)(sVar2 * 0x34 + *(int *)(unaff_EDI + 0x58));
      if (sVar1 < 0) {
        uVar3 = 0;
      }
      else {
        uVar4 = (int)*(short *)(unaff_EDI + 0xe) - (int)(short)iVar5;
        uVar3 = (int)sVar1;
        if ((int)uVar4 < (int)sVar1) {
          uVar3 = uVar4;
        }
      }
      FUN_004fb840();
      FUN_004fb840();
      iVar5 = iVar5 + (uVar3 & 0xfffffffe);
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(unaff_EDI + 0x54));
  }
  return;
}

Disassembly of the two calls (0x4fb840 args resolved from this), objdump -d -M intel:
  4fb734: push 0x4              ; split_code=4
  4fb736: push esi              ; size = split
  4fb737: push ebx              ; inner_start = col
  4fb738: push eax              ; entry ([esp+0x14])
  4fb739: push edi              ; tag
  4fb73a: xor eax,eax           ; outer_start = 0
  4fb73c: call 0x4fb840
  4fb741: mov ecx,[esp+0x28]    ; entry, reloaded
  4fb745: push 0x5              ; split_code=5
  4fb747: push esi              ; size = split (unchanged)
  4fb748: add esi,ebx           ; esi := col + split
  4fb74a: push esi              ; inner_start = col + split
  4fb74b: push ecx              ; entry
  4fb74c: push edi              ; tag
  4fb74d: xor eax,eax           ; outer_start = 0
  4fb74f: call 0x4fb840
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
