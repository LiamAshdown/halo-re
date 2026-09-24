// vehicle_calculate_hover_lift_toward_target  (Ghidra: unit_start_user_animation -- per
//   out/phase4/units_types_notes.md this pre-existing name is WRONG: it is the same
//   hovering-vehicle physics as 0x5738b0, entered when a target direction is already known; the
//   real unit_start_user_animation is 0x5702a0, this batch)
// address 0x5739a0, size 1315 bytes
// name confidence: 0.3 (units_types_notes.md's correction; the replacement name is new)
// rewrite confidence: 0.05 -- identical situation to vehicle_calculate_hover_turn_controls.c
//   (0x5738b0, this batch): every operand is an untraceable "unaff_" register or an
//   out-of-frame stack access, the unmistakable signature of a mid-function tail Ghidra split
//   off incorrectly. No faithful rewrite is possible without the real caller.
// evidence: same as vehicle_calculate_hover_turn_controls.c; the tail from the "0x4cc & 8" flag
//   check onward is byte-for-byte identical to that function (per functions.md: "Nearly
//   identical tail logic to FUN_005738b0 ... entered when a target direction is already
//   known"); callee vehicle_create_hover_thruster_midpoint_effects (0x574bc0, this batch).
// register convention: UNRESOLVED.
//   // blam-cc: UNSURE -- see header
// UNSURE: everything; see file header.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern void vehicle_create_hover_thruster_midpoint_effects(uint32_t unit_index); // 0x574bc0, this batch

// UNSURE: placeholder only; see file header.
void vehicle_calculate_hover_lift_toward_target(uint32_t unit_index)
{
    vehicle_create_hover_thruster_midpoint_effects(unit_index);
}

#if 0
Original Ghidra decompilation (0x5739a0):

void unit_start_user_animation(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  byte bVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  float fVar7;
  undefined *puVar8;
  int iVar9;
  short sVar10;
  short sVar11;
  float *unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  float *unaff_EDI;
  float10 in_ST0;
  float10 fVar12;

  if (in_ST0 == (float10)0.0) {
    *(undefined4 *)(unaff_EBP + -4) = 0;
  }
  else if ((float10)0.0 <= in_ST0) {
    *(undefined4 *)(unaff_EBP + -4) = 1;
  }
  else {
    *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
  }
  iVar1 = *(int *)(unaff_EBP + -4);
  fVar3 = *(float *)(unaff_EBP + -0xc) * *(float *)(unaff_ESI + 0x27c);
  if (fVar3 == 0.0) {
    *(undefined4 *)(unaff_EBP + -4) = 0;
  }
  else if (0.0 <= fVar3) {
    *(undefined4 *)(unaff_EBP + -4) = 1;
  }
  else {
    *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
  }
  *(float *)(unaff_EBP + -0x14) =
       ABS(*(float *)(unaff_EBP + -0xc)) * (float)*(int *)(unaff_EBP + -4);
  fVar3 = ABS(*(float *)(unaff_EBP + -0x10)) * (float)iVar1 + 1.0;
  if (0.3 <= fVar3) {
    if (2.5 < fVar3) {
      fVar3 = 2.5;
    }
  }
  else {
    fVar3 = 0.3;
  }
  fVar2 = *(float *)(unaff_ESI + 0x278);
  fVar7 = *(float *)(unaff_EBP + -0x14) + 1.0;
  if (0.3 <= fVar7) {
    if (2.5 < fVar7) {
      fVar7 = 2.5;
    }
  }
  else {
    fVar7 = 0.3;
  }
  *(float *)(unaff_EBP + -0x4c) =
       fVar7 * *(float *)(unaff_ESI + 0x27c) * 0.0015514038 + *(float *)(unaff_EBP + -0x4c);
  fVar7 = (1.0 - *(float *)(unaff_ESI + 0x88)) * 0.0038785094;
  *(float *)(unaff_EBP + -4) = fVar7;
  uVar5 = *(undefined4 *)PTR_DAT_00696714;
  uVar6 = *(undefined4 *)(PTR_DAT_00696714 + 4);
  *(undefined4 *)(unaff_EBP + -0x14) = *(undefined4 *)(PTR_DAT_00696714 + 8);
  fVar3 = (fVar7 * *(float *)(unaff_EBP + -0x10) +
          fVar3 * fVar2 * 0.0015514038 + *(float *)(unaff_EBP + -0x50)) *
          *(float *)(*(int *)(unaff_EBP + -0x3c) + 0x54);
  *(undefined4 *)(unaff_EBP + -0x1c) = uVar5;
  *(undefined4 *)(unaff_EBP + -0x18) = uVar6;
  *(float *)(unaff_EBP + -0x1c) =
       *(float *)(unaff_EBP + -0x48) * fVar3 + *(float *)(unaff_EBP + -0x1c);
  *(float *)(unaff_EBP + -0x18) =
       *(float *)(unaff_EBP + -0x44) * fVar3 + *(float *)(unaff_EBP + -0x18);
  *(float *)(unaff_EBP + -0x14) =
       *(float *)(unaff_EBP + -0x40) * fVar3 + *(float *)(unaff_EBP + -0x14);
  fVar2 = -((*(float *)(unaff_EBP + -4) * *(float *)(unaff_EBP + -0xc) +
            *(float *)(unaff_EBP + -0x4c)) * *(float *)(*(int *)(unaff_EBP + -0x3c) + 0x50));
  fVar3 = *unaff_EDI;
  *(float *)(unaff_EBP + -0x18) = fVar2 * unaff_EDI[1] + *(float *)(unaff_EBP + -0x18);
  *(float *)(unaff_EBP + -0x14) = fVar2 * unaff_EDI[2] + *(float *)(unaff_EBP + -0x14);
  *(float *)(unaff_EBP + -4) = 1.0 - *(float *)(unaff_ESI + 0x4ec);
  *(float *)(unaff_EBP + -0x34) =
       (fVar2 * fVar3 + *(float *)(unaff_EBP + -0x1c)) * *(float *)(unaff_EBP + -4) +
       *(float *)(unaff_EBP + -0x34);
  *(float *)(unaff_EBP + -0x30) =
       *(float *)(unaff_EBP + -0x18) * *(float *)(unaff_EBP + -4) + *(float *)(unaff_EBP + -0x30);
  *(float *)(unaff_EBP + -0x2c) =
       *(float *)(unaff_EBP + -0x14) * *(float *)(unaff_EBP + -4) + *(float *)(unaff_EBP + -0x2c);
  if ((*(byte *)(unaff_ESI + 0x4cc) & 8) != 0) {
    fVar3 = (*unaff_EDI * *(float *)(unaff_ESI + 0x68) +
            unaff_EDI[1] * *(float *)(unaff_ESI + 0x6c) +
            unaff_EDI[2] * *(float *)(unaff_ESI + 0x70)) /
            *(float *)(*(int *)(unaff_EBP + -0x58) + 0x2f8);
    if (0.0 <= fVar3) {
      if (1.0 < fVar3) {
        fVar3 = 1.0;
      }
    }
    else {
      fVar3 = 0.0;
    }
    *(float *)(unaff_EBP + -0x1c) = unaff_EBX[1] * unaff_EDI[2] - unaff_EDI[1] * unaff_EBX[2];
    fVar2 = *unaff_EDI;
    *(undefined4 *)(unaff_EBP + -0x48) = *(undefined4 *)(unaff_EBP + -0x1c);
    *(float *)(unaff_EBP + -0x18) = fVar2 * unaff_EBX[2] - *unaff_EBX * unaff_EDI[2];
    fVar2 = unaff_EDI[1];
    *(undefined4 *)(unaff_EBP + -0x44) = *(undefined4 *)(unaff_EBP + -0x18);
    puVar8 = PTR_DAT_00696720;
    *(float *)(unaff_EBP + -0x14) = fVar2 * *unaff_EBX - unaff_EBX[1] * *unaff_EDI;
    *(undefined4 *)(unaff_EBP + -0x40) = *(undefined4 *)(unaff_EBP + -0x14);
    if (0.0 < fVar3) {
      fVar2 = *(float *)(*(int *)(unaff_EBP + -0x3c) + 0x54) * *(float *)(unaff_ESI + 0x4ec) * fVar3
              * -0.005817764;
      *(float *)(unaff_EBP + -0x34) =
           *(float *)(unaff_EBP + -0x48) * fVar2 + *(float *)(unaff_EBP + -0x34);
      *(float *)(unaff_EBP + -0x30) =
           *(float *)(unaff_EBP + -0x44) * fVar2 + *(float *)(unaff_EBP + -0x30);
      *(float *)(unaff_EBP + -0x2c) =
           *(float *)(unaff_EBP + -0x40) * fVar2 + *(float *)(unaff_EBP + -0x2c);
      fVar3 = *(float *)(*(int *)(unaff_EBP + -0x3c) + 8) * *(float *)(unaff_ESI + 0x4ec) * fVar3 *
              0.004;
      *(float *)(unaff_EBP + -0x28) = fVar3 * *(float *)puVar8 + *(float *)(unaff_EBP + -0x28);
      *(float *)(unaff_EBP + -0x24) = fVar3 * *(float *)(puVar8 + 4) + *(float *)(unaff_EBP + -0x24)
      ;
      *(float *)(unaff_EBP + -0x20) = fVar3 * *(float *)(puVar8 + 8) + *(float *)(unaff_EBP + -0x20)
      ;
    }
    bVar4 = *(byte *)(unaff_ESI + 0x4d0);
    if (bVar4 != 0) {
      vector3d_cross_product(unaff_EBP + -0x48);
      fVar12 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 < fVar12) {
        *(uint *)(unaff_EBP + -0xc) = (uint)bVar4;
        fVar3 = 1.0 - (float)*(int *)(unaff_EBP + -0xc) * 0.033333335;
        if (0.0 <= fVar3) {
          if (1.0 < fVar3) {
            fVar3 = 1.0;
          }
        }
        else {
          fVar3 = 0.0;
        }
        fVar3 = (1.0 - *(float *)(unaff_ESI + 0x4ec)) * *(float *)(*(int *)(unaff_EBP + -0x3c) + 8)
                * fVar3;
        fVar2 = fVar3 * 0.002;
        *(float *)(unaff_EBP + -0x28) =
             *(float *)(unaff_EBP + -0x1c) * fVar2 + *(float *)(unaff_EBP + -0x28);
        *(float *)(unaff_EBP + -0x24) =
             *(float *)(unaff_EBP + -0x18) * fVar2 + *(float *)(unaff_EBP + -0x24);
        fVar3 = fVar3 * 0.001;
        *(float *)(unaff_EBP + -0x28) = fVar3 * *(float *)puVar8 + *(float *)(unaff_EBP + -0x28);
        *(float *)(unaff_EBP + -0x24) =
             fVar3 * *(float *)(puVar8 + 4) + *(float *)(unaff_EBP + -0x24);
        *(float *)(unaff_EBP + -0x20) =
             fVar3 * *(float *)(puVar8 + 8) +
             fVar2 * *(float *)(unaff_EBP + -0x14) + *(float *)(unaff_EBP + -0x20);
      }
    }
  }
  iVar1 = *(int *)(unaff_EBP + -0x3c);
  *(float *)(unaff_EBP + -0x28) = *(float *)(unaff_EBP + -0x28) * *(float *)(unaff_EBP + -0x38);
  *(float *)(unaff_EBP + -0x24) = *(float *)(unaff_EBP + -0x24) * *(float *)(unaff_EBP + -0x38);
  *(float *)(unaff_EBP + -0x20) = *(float *)(unaff_EBP + -0x20) * *(float *)(unaff_EBP + -0x38);
  *(float *)(unaff_EBP + -0x34) = *(float *)(unaff_EBP + -0x34) * *(float *)(unaff_EBP + -0x38);
  *(float *)(unaff_EBP + -0x30) = *(float *)(unaff_EBP + -0x30) * *(float *)(unaff_EBP + -0x38);
  *(float *)(unaff_EBP + -0x2c) = *(float *)(unaff_EBP + -0x2c) * *(float *)(unaff_EBP + -0x38);
  FUN_00507840(*(undefined4 *)(unaff_EBP + 8),*(undefined4 *)(unaff_EBP + 0x10),
               *(undefined4 *)(unaff_EBP + 0x14),unaff_EBP + -0x28,unaff_EBP + -0x34);
  if (0.4 <= *(float *)(unaff_ESI + 0x88)) {
    fVar3 = *(float *)(unaff_ESI + 0x88);
  }
  else {
    fVar3 = 0.4;
  }
  iVar9 = *(int *)(iVar1 + 0x74);
  fVar2 = 0.0;
  sVar10 = 0;
  sVar11 = 0;
  *(undefined4 *)(unaff_EBP + -4) = 0;
  *(int *)(unaff_EBP + -0xc) = iVar9;
  if (0 < iVar9) {
    iVar1 = *(int *)(iVar1 + 0x78);
    iVar9 = 0;
    do {
      if ((*(short *)(iVar9 * 0x80 + 0x20 + iVar1) != -1) &&
         (sVar10 = sVar10 + 1, (*(byte *)(iVar9 * 0x130 + *(int *)(unaff_EBP + 0x14)) & 0x10) != 0))
      {
        *(int *)(unaff_EBP + -4) = *(int *)(unaff_EBP + -4) + 1;
      }
      sVar11 = sVar11 + 1;
      iVar9 = (int)sVar11;
    } while (iVar9 < *(int *)(unaff_EBP + -0xc));
    if (0 < sVar10) {
      *(int *)(unaff_EBP + -0xc) = (int)*(short *)(unaff_EBP + -4);
      iVar1 = *(int *)(unaff_EBP + -0xc);
      *(int *)(unaff_EBP + -0xc) = (int)sVar10;
      fVar2 = (float)iVar1 / (float)*(int *)(unaff_EBP + -0xc);
    }
  }
  fVar2 = fVar2 * fVar3;
  if (0.0 <= fVar2) {
    if (1.0 < fVar2) {
      fVar2 = 1.0;
    }
  }
  else {
    fVar2 = 0.0;
  }
  fVar3 = fVar2 - *(float *)(unaff_ESI + 0x4ec);
  if (fVar3 <= 0.1) {
    if (fVar3 < -0.1) {
      fVar2 = *(float *)(unaff_ESI + 0x4ec) - 0.1;
    }
  }
  else {
    fVar2 = *(float *)(unaff_ESI + 0x4ec) + 0.1;
  }
  uVar5 = *(undefined4 *)(unaff_EBP + 8);
  *(float *)(unaff_ESI + 0x4ec) = fVar2;
  vehicle_create_hover_thruster_midpoint_effects(uVar5);
  return;
}
#endif
