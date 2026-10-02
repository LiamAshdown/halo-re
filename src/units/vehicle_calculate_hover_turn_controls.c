// vehicle_calculate_hover_turn_controls  (NOT A FUNCTION: an address inside vehicle_calculate_wing_flex_controls)
// address 0x5738b0, size 267 bytes
// VERIFIED against disassembly 0x5734d0..0x573ede (2026-09-30): 0x5734d0 (`push ebp; mov ebp,esp`) to 0x573edd (the only `ret`)
//   is ONE 2574-byte function (a hovering vehicle's per-tick physics: forward/turn controls, lift toward the target, thruster
//   effects). Ghidra split it at 0x5738b0 and 0x5739a0, which is why this range shows unaffected registers, x87 values with no
//   source and offsets from the caller's frame (ebp - 4 .. ebp - 0x58). Nothing calls or jumps to 0x5738b0 from outside the
//   function. This file has no body of its own and must never be hooked; the whole function still needs to be rewritten
//   once, as a single unit, in vehicle_calculate_wing_flex_controls.c (whose current C is sized 992 bytes, i.e. does not cover the whole range, and is marked
//   low confidence).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Not callable: see the header. Kept only so the address stays listed in the symbol tables.
void vehicle_calculate_hover_turn_controls(void)
{
}

#if 0
Original Ghidra decompilation (0x5738b0):

void unit_get_custom_animation_time(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  byte bVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined *puVar14;
  int iVar15;
  short sVar16;
  undefined4 *in_ECX;
  short sVar17;
  float *unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  float *unaff_EDI;
  float10 in_ST0;
  float10 fVar18;

  *(float *)(unaff_EBP + -8) = (float)((float10)*(float *)(unaff_EBP + -8) * in_ST0);
  *(float *)(unaff_EBP + -4) = (float)((float10)*(float *)(unaff_EBP + -4) * in_ST0);
  fVar2 = SQRT(*(float *)(unaff_EBP + -0x10) * *(float *)(unaff_EBP + -0x10) +
               *(float *)(unaff_EBP + -0xc) * *(float *)(unaff_EBP + -0xc));
  if (0.0001 <= ABS(fVar2)) {
    fVar2 = 1.0 / fVar2;
    *(float *)(unaff_EBP + -0x10) = *(float *)(unaff_EBP + -0x10) * fVar2;
    *(float *)(unaff_EBP + -0xc) = *(float *)(unaff_EBP + -0xc) * fVar2;
  }
  if (*(float *)(unaff_ESI + 0x88) <= 0.0) {
    fVar2 = *(float *)(unaff_ESI + 0x278) * 0.0015514038 + *(float *)(unaff_EBP + -0x50);
    fVar3 = *(float *)(unaff_ESI + 0x27c) * 0.0015514038;
  }
  else {
    uVar12 = *in_ECX;
    fVar2 = unaff_EBX[1];
    fVar3 = *unaff_EBX;
    fVar4 = *(float *)(unaff_EBP + -0xc);
    fVar5 = unaff_EBX[1];
    fVar6 = *(float *)(unaff_EBP + -0x10);
    fVar7 = *unaff_EBX;
    fVar8 = *(float *)(unaff_EBP + -0xc);
    *(undefined4 *)(unaff_EBP + -0xc) = in_ECX[1];
    fVar9 = *(float *)(unaff_ESI + 0x90);
    fVar10 = *(float *)(unaff_EBP + -0x10);
    *(undefined4 *)(unaff_EBP + -0x10) = uVar12;
    *(float *)(unaff_EBP + -0x18) = fVar10 * *(float *)(unaff_ESI + 0x8c) + fVar8 * fVar9;
    *(float *)(unaff_EBP + -0x14) =
         -(*(float *)(unaff_EBP + -8) * *(float *)(unaff_ESI + 0x8c) +
          *(float *)(unaff_EBP + -4) * *(float *)(unaff_ESI + 0x90));
    *(float *)(unaff_EBP + -0x10) =
         *(float *)(unaff_EBP + -0x10) -
         (*(float *)(unaff_EBP + -8) * fVar3 + *(float *)(unaff_EBP + -4) * fVar2);
    *(float *)(unaff_EBP + -0x10) =
         *(float *)(unaff_EBP + -0x10) - *(float *)(unaff_EBP + -0x18) * 15.0;
    *(float *)(unaff_EBP + -0xc) =
         (*(float *)(unaff_EBP + -0xc) - (fVar6 * fVar7 + fVar4 * fVar5)) -
         *(float *)(unaff_EBP + -0x14) * 15.0;
    fVar2 = *(float *)(unaff_EBP + -0x10) * *(float *)(unaff_ESI + 0x278);
    if (fVar2 == 0.0) {
      *(undefined4 *)(unaff_EBP + -4) = 0;
    }
    else if (0.0 <= fVar2) {
      *(undefined4 *)(unaff_EBP + -4) = 1;
    }
    else {
      *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
    }
    iVar1 = *(int *)(unaff_EBP + -4);
    fVar2 = *(float *)(unaff_EBP + -0xc) * *(float *)(unaff_ESI + 0x27c);
    if (fVar2 == 0.0) {
      *(undefined4 *)(unaff_EBP + -4) = 0;
    }
    else if (0.0 <= fVar2) {
      *(undefined4 *)(unaff_EBP + -4) = 1;
    }
    else {
      *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
    }
    *(float *)(unaff_EBP + -0x14) =
         ABS(*(float *)(unaff_EBP + -0xc)) * (float)*(int *)(unaff_EBP + -4);
    fVar2 = ABS(*(float *)(unaff_EBP + -0x10)) * (float)iVar1 + 1.0;
    if (0.3 <= fVar2) {
      if (2.5 < fVar2) {
        fVar2 = 2.5;
      }
    }
    else {
      fVar2 = 0.3;
    }
    fVar3 = *(float *)(unaff_ESI + 0x278);
    fVar4 = *(float *)(unaff_EBP + -0x14) + 1.0;
    if (0.3 <= fVar4) {
      if (2.5 < fVar4) {
        fVar4 = 2.5;
      }
    }
    else {
      fVar4 = 0.3;
    }
    *(float *)(unaff_EBP + -0x4c) =
         fVar4 * *(float *)(unaff_ESI + 0x27c) * 0.0015514038 + *(float *)(unaff_EBP + -0x4c);
    fVar4 = (1.0 - *(float *)(unaff_ESI + 0x88)) * 0.0038785094;
    *(float *)(unaff_EBP + -4) = fVar4;
    fVar2 = fVar4 * *(float *)(unaff_EBP + -0x10) +
            fVar2 * fVar3 * 0.0015514038 + *(float *)(unaff_EBP + -0x50);
    fVar3 = *(float *)(unaff_EBP + -4) * *(float *)(unaff_EBP + -0xc);
  }
  uVar12 = *(undefined4 *)PTR_DAT_00696714;
  uVar13 = *(undefined4 *)(PTR_DAT_00696714 + 4);
  *(undefined4 *)(unaff_EBP + -0x14) = *(undefined4 *)(PTR_DAT_00696714 + 8);
  fVar2 = fVar2 * *(float *)(*(int *)(unaff_EBP + -0x3c) + 0x54);
  *(undefined4 *)(unaff_EBP + -0x1c) = uVar12;
  *(undefined4 *)(unaff_EBP + -0x18) = uVar13;
  *(float *)(unaff_EBP + -0x1c) =
       *(float *)(unaff_EBP + -0x48) * fVar2 + *(float *)(unaff_EBP + -0x1c);
  *(float *)(unaff_EBP + -0x18) =
       *(float *)(unaff_EBP + -0x44) * fVar2 + *(float *)(unaff_EBP + -0x18);
  *(float *)(unaff_EBP + -0x14) =
       *(float *)(unaff_EBP + -0x40) * fVar2 + *(float *)(unaff_EBP + -0x14);
  fVar3 = -((fVar3 + *(float *)(unaff_EBP + -0x4c)) * *(float *)(*(int *)(unaff_EBP + -0x3c) + 0x50)
           );
  fVar2 = *unaff_EDI;
  *(float *)(unaff_EBP + -0x18) = fVar3 * unaff_EDI[1] + *(float *)(unaff_EBP + -0x18);
  *(float *)(unaff_EBP + -0x14) = fVar3 * unaff_EDI[2] + *(float *)(unaff_EBP + -0x14);
  *(float *)(unaff_EBP + -4) = 1.0 - *(float *)(unaff_ESI + 0x4ec);
  *(float *)(unaff_EBP + -0x34) =
       (fVar3 * fVar2 + *(float *)(unaff_EBP + -0x1c)) * *(float *)(unaff_EBP + -4) +
       *(float *)(unaff_EBP + -0x34);
  *(float *)(unaff_EBP + -0x30) =
       *(float *)(unaff_EBP + -0x18) * *(float *)(unaff_EBP + -4) + *(float *)(unaff_EBP + -0x30);
  *(float *)(unaff_EBP + -0x2c) =
       *(float *)(unaff_EBP + -0x14) * *(float *)(unaff_EBP + -4) + *(float *)(unaff_EBP + -0x2c);
  if ((*(byte *)(unaff_ESI + 0x4cc) & 8) != 0) {
    fVar2 = (*unaff_EDI * *(float *)(unaff_ESI + 0x68) +
            unaff_EDI[1] * *(float *)(unaff_ESI + 0x6c) +
            unaff_EDI[2] * *(float *)(unaff_ESI + 0x70)) /
            *(float *)(*(int *)(unaff_EBP + -0x58) + 0x2f8);
    if (0.0 <= fVar2) {
      if (1.0 < fVar2) {
        fVar2 = 1.0;
      }
    }
    else {
      fVar2 = 0.0;
    }
    *(float *)(unaff_EBP + -0x1c) = unaff_EBX[1] * unaff_EDI[2] - unaff_EDI[1] * unaff_EBX[2];
    fVar3 = *unaff_EDI;
    *(undefined4 *)(unaff_EBP + -0x48) = *(undefined4 *)(unaff_EBP + -0x1c);
    *(float *)(unaff_EBP + -0x18) = fVar3 * unaff_EBX[2] - *unaff_EBX * unaff_EDI[2];
    fVar3 = unaff_EDI[1];
    *(undefined4 *)(unaff_EBP + -0x44) = *(undefined4 *)(unaff_EBP + -0x18);
    puVar14 = PTR_DAT_00696720;
    *(float *)(unaff_EBP + -0x14) = fVar3 * *unaff_EBX - unaff_EBX[1] * *unaff_EDI;
    *(undefined4 *)(unaff_EBP + -0x40) = *(undefined4 *)(unaff_EBP + -0x14);
    if (0.0 < fVar2) {
      fVar3 = *(float *)(*(int *)(unaff_EBP + -0x3c) + 0x54) * *(float *)(unaff_ESI + 0x4ec) * fVar2
              * -0.005817764;
      *(float *)(unaff_EBP + -0x34) =
           *(float *)(unaff_EBP + -0x48) * fVar3 + *(float *)(unaff_EBP + -0x34);
      *(float *)(unaff_EBP + -0x30) =
           *(float *)(unaff_EBP + -0x44) * fVar3 + *(float *)(unaff_EBP + -0x30);
      *(float *)(unaff_EBP + -0x2c) =
           *(float *)(unaff_EBP + -0x40) * fVar3 + *(float *)(unaff_EBP + -0x2c);
      fVar2 = *(float *)(*(int *)(unaff_EBP + -0x3c) + 8) * *(float *)(unaff_ESI + 0x4ec) * fVar2 *
              0.004;
      *(float *)(unaff_EBP + -0x28) = fVar2 * *(float *)puVar14 + *(float *)(unaff_EBP + -0x28);
      *(float *)(unaff_EBP + -0x24) =
           fVar2 * *(float *)(puVar14 + 4) + *(float *)(unaff_EBP + -0x24);
      *(float *)(unaff_EBP + -0x20) =
           fVar2 * *(float *)(puVar14 + 8) + *(float *)(unaff_EBP + -0x20);
    }
    bVar11 = *(byte *)(unaff_ESI + 0x4d0);
    if (bVar11 != 0) {
      vector3d_cross_product(unaff_EBP + -0x48);
      fVar18 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 < fVar18) {
        *(uint *)(unaff_EBP + -0xc) = (uint)bVar11;
        fVar2 = 1.0 - (float)*(int *)(unaff_EBP + -0xc) * 0.033333335;
        if (0.0 <= fVar2) {
          if (1.0 < fVar2) {
            fVar2 = 1.0;
          }
        }
        else {
          fVar2 = 0.0;
        }
        fVar2 = (1.0 - *(float *)(unaff_ESI + 0x4ec)) * *(float *)(*(int *)(unaff_EBP + -0x3c) + 8)
                * fVar2;
        fVar3 = fVar2 * 0.002;
        *(float *)(unaff_EBP + -0x28) =
             *(float *)(unaff_EBP + -0x1c) * fVar3 + *(float *)(unaff_EBP + -0x28);
        *(float *)(unaff_EBP + -0x24) =
             *(float *)(unaff_EBP + -0x18) * fVar3 + *(float *)(unaff_EBP + -0x24);
        fVar2 = fVar2 * 0.001;
        *(float *)(unaff_EBP + -0x28) = fVar2 * *(float *)puVar14 + *(float *)(unaff_EBP + -0x28);
        *(float *)(unaff_EBP + -0x24) =
             fVar2 * *(float *)(puVar14 + 4) + *(float *)(unaff_EBP + -0x24);
        *(float *)(unaff_EBP + -0x20) =
             fVar2 * *(float *)(puVar14 + 8) +
             fVar3 * *(float *)(unaff_EBP + -0x14) + *(float *)(unaff_EBP + -0x20);
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
    fVar2 = *(float *)(unaff_ESI + 0x88);
  }
  else {
    fVar2 = 0.4;
  }
  iVar15 = *(int *)(iVar1 + 0x74);
  fVar3 = 0.0;
  sVar16 = 0;
  sVar17 = 0;
  *(undefined4 *)(unaff_EBP + -4) = 0;
  *(int *)(unaff_EBP + -0xc) = iVar15;
  if (0 < iVar15) {
    iVar1 = *(int *)(iVar1 + 0x78);
    iVar15 = 0;
    do {
      if ((*(short *)(iVar15 * 0x80 + 0x20 + iVar1) != -1) &&
         (sVar16 = sVar16 + 1, (*(byte *)(iVar15 * 0x130 + *(int *)(unaff_EBP + 0x14)) & 0x10) != 0)
         ) {
        *(int *)(unaff_EBP + -4) = *(int *)(unaff_EBP + -4) + 1;
      }
      sVar17 = sVar17 + 1;
      iVar15 = (int)sVar17;
    } while (iVar15 < *(int *)(unaff_EBP + -0xc));
    if (0 < sVar16) {
      *(int *)(unaff_EBP + -0xc) = (int)*(short *)(unaff_EBP + -4);
      iVar1 = *(int *)(unaff_EBP + -0xc);
      *(int *)(unaff_EBP + -0xc) = (int)sVar16;
      fVar3 = (float)iVar1 / (float)*(int *)(unaff_EBP + -0xc);
    }
  }
  fVar3 = fVar3 * fVar2;
  if (0.0 <= fVar3) {
    if (1.0 < fVar3) {
      fVar3 = 1.0;
    }
  }
  else {
    fVar3 = 0.0;
  }
  fVar2 = fVar3 - *(float *)(unaff_ESI + 0x4ec);
  if (fVar2 <= 0.1) {
    if (fVar2 < -0.1) {
      fVar3 = *(float *)(unaff_ESI + 0x4ec) - 0.1;
    }
  }
  else {
    fVar3 = *(float *)(unaff_ESI + 0x4ec) + 0.1;
  }
  uVar12 = *(undefined4 *)(unaff_EBP + 8);
  *(float *)(unaff_ESI + 0x4ec) = fVar3;
  vehicle_create_hover_thruster_midpoint_effects(uVar12);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
