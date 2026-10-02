// object_recalculate_bounding_radius_clone_4f8834   (renamed by the phase-4 review pass; Ghidra/PDB name was `objects_update__object_in_player_pvs_nop1`,
//    which this file's own header shows does not describe the code)
// address 0x4f8834, size 553 bytes
// name confidence: 0.1 (inherited Ghidra name; out/phase4/objects_types_notes.md: "0x4f84e2,
// 0x4f8834, 0x4f8a70: compiler-cloned variants of object_recalculate_bounding_radius 0x4f8310;
// all three end with the same write to object+0xac scaled by object+0xb0, and all three have
// zero recorded callers. Their inherited names ... do not match.")
// rewrite confidence: 1.0 (FRAGMENT: 0x4f8834 lies inside object_recalculate_bounding_radius 0x4f8310..0x4f8afa, whose C covers it; only jumps reach here)
//
// This is not a real, independently-callable function: it has zero recorded callers, and every
// input arrives as an unresolved "unaff_"/"extraout_" register with no call site anywhere to
// check a real parameter list against. Its body is the same node-tree-walk / quaternion-and-
// matrix-composition sequence as the animated branch of object_recalculate_bounding_radius
// (0x4f8310, this batch) -- a compiler-cloned duplicate (identical control flow, EBP-relative
// instead of ESP-relative locals, several calls to the shared matrix4x3_multiply_procedure
// pointer shown with no visible arguments at all) rather than distinct engine logic. See
// object_recalculate_bounding_radius.c for the rewrite of that logic; nothing here is
// translated independently to avoid a second, unverifiable copy under a name
// ("...object_in_player_pvs_nop1") that does not describe it and that this pass found no
// evidence for.

#if 0
Original Ghidra decompilation (0x4f8834):

void objects_update__object_in_player_pvs_nop1(void)

{
  float fVar1;
  float fVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  short sVar6;
  int extraout_ECX;
  int iVar7;
  int extraout_ECX_00;
  undefined4 *extraout_EDX;
  int unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  undefined4 *puVar8;
  float *unaff_EDI;
  float *pfVar9;
  float *pfVar10;
  undefined4 *puVar11;

  do {
    *(undefined4 *)(unaff_EBP + -0xac) = 0x3f800000;
    *(undefined4 *)(unaff_EBP + -0xa8) = 0x3f800000;
    *(undefined4 *)(unaff_EBP + -0xa4) = 0;
    *(undefined4 *)(unaff_EBP + -0xa0) = 0;
    *(undefined4 *)(unaff_EBP + -0x9c) = 0;
    *(undefined4 *)(unaff_EBP + -0x98) = 0x3f800000;
    *(undefined4 *)(unaff_EBP + -0x94) = 0;
    *(undefined4 *)(unaff_EBP + -0x90) = 0;
    *(undefined4 *)(unaff_EBP + -0x8c) = 0;
    *(undefined4 *)(unaff_EBP + -0x88) = 0x3f800000;
    (*(code *)PTR_matrix4x3_multiply_00696664)();
    if (unaff_EDI == (float *)0x0) {
      (*(code *)PTR_matrix4x3_multiply_00696664)();
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(undefined4 *)(unaff_EBP + -0x14),unaff_EBP + -0x154,
                 *(undefined4 *)(unaff_EBP + -0x14));
    }
    else {
      if (*unaff_EDI != 1.0) {
        fVar1 = *unaff_EDI;
        *(float **)(unaff_EBP + -8) = (float *)(unaff_EBP + -0x18c);
        *(float *)(unaff_EBP + -0x4c) = *(float *)(unaff_EBP + -0x4c) * fVar1;
        *(float *)(unaff_EBP + -0x48) = *(float *)(unaff_EBP + -0x48) * *unaff_EDI;
        fVar1 = *(float *)(unaff_EBP + -0x44);
        fVar2 = *unaff_EDI;
        pfVar9 = (float *)(unaff_EBP + -0x18c);
        for (iVar7 = 0xd; iVar7 != 0; iVar7 = iVar7 + -1) {
          *pfVar9 = *unaff_EDI;
          unaff_EDI = unaff_EDI + 1;
          pfVar9 = pfVar9 + 1;
        }
        *(float *)(unaff_EBP + -0x44) = fVar1 * fVar2;
        unaff_ESI = *(int *)(unaff_EBP + 8);
        *(undefined4 *)(unaff_EBP + -0x18c) = 0x3f800000;
        unaff_EDI = (float *)(unaff_EBP + -0x18c);
      }
      if ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(unaff_EBX + 0x11c) & 0xffff) * 0xc) + 0x10) & 0x1000) != 0)
      {
        pfVar9 = (float *)(unaff_EBP + -0x18c);
        if (unaff_EDI != pfVar9) {
          pfVar10 = pfVar9;
          for (iVar7 = 0xd; iVar7 != 0; iVar7 = iVar7 + -1) {
            *pfVar10 = *unaff_EDI;
            unaff_EDI = unaff_EDI + 1;
            pfVar10 = pfVar10 + 1;
          }
          unaff_ESI = *(int *)(unaff_EBP + 8);
          *(float **)(unaff_EBP + -8) = pfVar9;
          unaff_EDI = pfVar9;
        }
        unaff_EDI[4] = -unaff_EDI[4];
        unaff_EDI[5] = -unaff_EDI[5];
        unaff_EDI[6] = -unaff_EDI[6];
      }
      (*(code *)PTR_matrix4x3_multiply_00696664)();
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(undefined4 *)(unaff_EBP + -0x14),unaff_EBP + -0x11c,
                 *(undefined4 *)(unaff_EBP + -0x14));
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(undefined4 *)(unaff_EBP + -0x14),unaff_EBP + -0x154,
                 *(undefined4 *)(unaff_EBP + -0x14));
    }
    while( true ) {
      while( true ) {
        iVar7 = *(int *)(unaff_EBP + -0xc);
        if (*(short *)(unaff_ESI + 0x20) != -1) {
          sVar6 = (short)iVar7;
          iVar7 = iVar7 + 1;
          *(short *)(unaff_EBP + -0x20c + sVar6 * 2) = *(short *)(unaff_ESI + 0x20);
          *(int *)(unaff_EBP + -0xc) = iVar7;
        }
        if (*(short *)(unaff_ESI + 0x22) != -1) {
          sVar6 = (short)iVar7;
          iVar7 = iVar7 + 1;
          *(short *)(unaff_EBP + -0x20c + sVar6 * 2) = *(short *)(unaff_ESI + 0x22);
          *(int *)(unaff_EBP + -0xc) = iVar7;
        }
        if (*(short *)(unaff_EBP + -0x20) == (short)iVar7) {
          iVar7 = *(int *)(unaff_EBP + -0x1c);
          matrix4x3_transform_point();
          fVar1 = *(float *)(iVar7 + 4);
          *(float *)(unaff_EBX + 0xac) = fVar1;
          if (0.0 < *(float *)(unaff_EBX + 0xb0)) {
            *(float *)(unaff_EBX + 0xac) = fVar1 * *(float *)(unaff_EBX + 0xb0);
            return;
          }
          return;
        }
        sVar6 = *(short *)(unaff_EBP + -0x20c + (short)*(int *)(unaff_EBP + -0x20) * 2);
        *(int *)(unaff_EBP + -0x20) = *(int *)(unaff_EBP + -0x20) + 1;
        unaff_ESI = sVar6 * 0x9c + *(int *)(*(int *)(unaff_EBP + -0x28) + 0xbc);
        *(int *)(unaff_EBP + 8) = unaff_ESI;
        if (sVar6 == 0) break;
        matrix4x3_from_quaternion();
        *extraout_EDX = *(undefined4 *)(extraout_ECX_00 + 0x1c);
        extraout_EDX[10] = *(undefined4 *)(extraout_ECX_00 + 0x10);
        extraout_EDX[0xb] = *(undefined4 *)(extraout_ECX_00 + 0x14);
        unaff_ESI = *(int *)(unaff_EBP + 8);
        extraout_EDX[0xc] = *(undefined4 *)(extraout_ECX_00 + 0x18);
        (*(code *)PTR_matrix4x3_multiply_00696664)();
      }
      matrix4x3_from_quaternion();
      uVar3 = *(undefined4 *)(extraout_ECX + 0x10);
      *(undefined4 *)(unaff_EBP + -0x154) = *(undefined4 *)(extraout_ECX + 0x1c);
      uVar4 = *(undefined4 *)(extraout_ECX + 0x14);
      *(undefined4 *)(unaff_EBP + -300) = uVar3;
      uVar3 = *(undefined4 *)(extraout_ECX + 0x18);
      *(undefined4 *)(unaff_EBP + -0x128) = uVar4;
      *(undefined4 *)(unaff_EBP + -0x124) = uVar3;
      if (*(char *)(unaff_EBP + -0xd) == '\0') break;
      puVar8 = (undefined4 *)(unaff_EBP + -0x154);
      puVar11 = *(undefined4 **)(unaff_EBP + -0x14);
      for (iVar7 = 0xd; iVar7 != 0; iVar7 = iVar7 + -1) {
        *puVar11 = *puVar8;
        puVar8 = puVar8 + 1;
        puVar11 = puVar11 + 1;
      }
      unaff_ESI = *(int *)(unaff_EBP + 8);
      unaff_EDI = *(float **)(unaff_EBP + -8);
    }
    uVar3 = *(undefined4 *)(unaff_EBX + 0x5c);
    uVar4 = *(undefined4 *)(unaff_EBX + 100);
    *(undefined4 *)(unaff_EBP + -0x48) = *(undefined4 *)(unaff_EBX + 0x60);
    *(undefined4 *)(unaff_EBP + -0x4c) = uVar3;
    *(undefined4 *)(unaff_EBP + -0x44) = uVar4;
    *(undefined4 *)(unaff_EBP + -0x74) = 0x3f800000;
    *(undefined4 *)(unaff_EBP + -0x70) = 0x3f800000;
    *(undefined4 *)(unaff_EBP + -0x6c) = 0;
    *(undefined4 *)(unaff_EBP + -0x68) = 0;
    *(undefined4 *)(unaff_EBP + -100) = 0;
    *(undefined4 *)(unaff_EBP + -0x60) = 0x3f800000;
    *(undefined4 *)(unaff_EBP + -0x5c) = 0;
    *(undefined4 *)(unaff_EBP + -0x58) = 0;
    *(undefined4 *)(unaff_EBP + -0x54) = 0;
    *(undefined4 *)(unaff_EBP + -0x50) = 0x3f800000;
    FUN_004cb970();
    if ((*(uint *)(unaff_EBX + 0x10) & 0x1000) != 0) {
      *(float *)(unaff_EBP + -0x10c) = -*(float *)(unaff_EBP + -0x10c);
      *(float *)(unaff_EBP + -0x108) = -*(float *)(unaff_EBP + -0x108);
      *(float *)(unaff_EBP + -0x104) = -*(float *)(unaff_EBP + -0x104);
    }
    uVar5 = *(uint *)(*(int *)(unaff_EBP + -0x1c) + 0x8c);
    if (uVar5 != 0xffffffff) {
      iVar7 = *(int *)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(float *)(unaff_EBP + -0x38) = -*(float *)(iVar7 + 0xc);
      *(float *)(unaff_EBP + -0x34) = -*(float *)(iVar7 + 0x10);
      fVar1 = *(float *)(iVar7 + 0x14);
      *(undefined4 *)(unaff_EBP + -0xbc) = *(undefined4 *)(unaff_EBP + -0x38);
      *(float *)(unaff_EBP + -0x30) = -fVar1;
      *(undefined4 *)(unaff_EBP + -0xb8) = *(undefined4 *)(unaff_EBP + -0x34);
      *(undefined4 *)(unaff_EBP + -0xb4) = *(undefined4 *)(unaff_EBP + -0x30);
      *(undefined4 *)(unaff_EBP + -0xe4) = 0x3f800000;
      *(undefined4 *)(unaff_EBP + -0xe0) = 0x3f800000;
      *(undefined4 *)(unaff_EBP + -0xdc) = 0;
      *(undefined4 *)(unaff_EBP + -0xd8) = 0;
      *(undefined4 *)(unaff_EBP + -0xd4) = 0;
      *(undefined4 *)(unaff_EBP + -0xd0) = 0x3f800000;
      *(undefined4 *)(unaff_EBP + -0xcc) = 0;
      *(undefined4 *)(unaff_EBP + -200) = 0;
      *(undefined4 *)(unaff_EBP + -0xc4) = 0;
      *(undefined4 *)(unaff_EBP + -0xc0) = 0x3f800000;
      (*(code *)PTR_matrix4x3_multiply_00696664)();
    }
    iVar7 = *(int *)(unaff_EBP + -0x1c);
    uVar3 = *(undefined4 *)(iVar7 + 0x18);
    uVar4 = *(undefined4 *)(iVar7 + 0x1c);
    *(undefined4 *)(unaff_EBP + -0x84) = *(undefined4 *)(iVar7 + 0x14);
    *(undefined4 *)(unaff_EBP + -0x80) = uVar3;
    *(undefined4 *)(unaff_EBP + -0x7c) = uVar4;
  } while( true );
}
#endif

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
