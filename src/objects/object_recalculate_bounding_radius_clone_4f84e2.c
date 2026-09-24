// object_recalculate_bounding_radius_clone_4f84e2   (renamed by the phase-4 review pass; Ghidra/PDB name was `objects_initialize_for_new_map_mod_processed_bsps`,
//    which this file's own header shows does not describe the code)
// address 0x4f84e2, size 850 bytes
// name confidence: 0.1 (inherited Ghidra name; out/phase4/objects_types_notes.md: "0x4f84e2,
// 0x4f8834, 0x4f8a70: compiler-cloned variants of object_recalculate_bounding_radius 0x4f8310;
// all three end with the same write to object+0xac scaled by object+0xb0, and all three have
// zero recorded callers. Their inherited names ... do not match.")
// rewrite confidence: n/a -- not independently rewritten
//
// This is not a real, independently-callable function: it has zero recorded callers, and every
// input arrives as an unresolved "unaff_"/"in_" register (in_AX, in_ECX, unaff_EBX, unaff_EBP,
// unaff_ESI, unaff_EDI, plus several "extraout_" registers from callees Ghidra could not fully
// type) with no call site anywhere to check a real parameter list against. Its body is the same
// node-tree-walk / quaternion-and-matrix-composition sequence as the animated branch of
// object_recalculate_bounding_radius (0x4f8310, this batch) -- a compiler-cloned duplicate
// (identical control flow, EBP-relative instead of ESP-relative locals) rather than distinct
// engine logic. See object_recalculate_bounding_radius.c for the rewrite of that logic; nothing
// here is translated independently to avoid a second, unverifiable copy under a name
// ("...for_new_map_mod_processed_bsps") that does not describe it and that this pass found no
// evidence for.

#if 0
Original Ghidra decompilation (0x4f84e2):

void objects_initialize_for_new_map_mod_processed_bsps(void)

{
  short *psVar1;
  float fVar2;
  float fVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  uint uVar6;
  short in_AX;
  int iVar7;
  int iVar8;
  short sVar9;
  int in_ECX;
  int extraout_ECX;
  int extraout_ECX_00;
  undefined4 *extraout_EDX;
  int unaff_EBX;
  int unaff_EBP;
  int *unaff_ESI;
  float *pfVar10;
  float *pfVar11;
  undefined4 *puVar12;
  int unaff_EDI;
  float *pfVar13;
  undefined4 *puVar14;

  do {
    *(undefined4 *)(unaff_EBP + -0xc) = *(undefined4 *)(unaff_EBX + 0x134 + in_ECX * 4);
    if (in_AX == 0) {
      if ((*(byte *)(in_ECX * 0x168 + *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x15c)) & 2) == 0) {
        *(int *)(unaff_EBP + -0x24) = *(short *)(unaff_EDI + 0x22) + -1;
        iVar8 = *(int *)(unaff_EBP + -0x24);
      }
      else {
        *(int *)(unaff_EBP + -0x24) = (int)*(short *)(unaff_EDI + 0x22);
        iVar8 = *(int *)(unaff_EBP + -0x24);
      }
      *(float *)(unaff_EBP + -0xc) = (float)iVar8 * *(float *)(unaff_EBP + -0xc);
      model_vertices_get_interpolated_frame
                (*(undefined4 *)(unaff_EBP + -0xc),*(undefined4 *)(unaff_EBP + -0x18));
    }
    else if (in_AX == 1) {
      FUN_004d51a0((uint)(*(int *)(DAT_006f1d6c + 0xc) + *(int *)(unaff_EBP + 8)) %
                   (uint)(int)*(short *)(unaff_EDI + 0x22),*(undefined4 *)(unaff_EBP + -0xc),
                   *(undefined4 *)(unaff_EBP + -0x18));
    }
    pfVar10 = *(float **)(unaff_EBP + -8);
    do {
      iVar8 = *unaff_ESI;
      iVar7 = *(int *)(unaff_EBP + -0x20) + 1;
      *(int *)(unaff_EBP + -0x20) = iVar7;
      iVar7 = (int)(short)iVar7;
      if (iVar8 <= iVar7) {
        iVar8 = *(int *)(unaff_EBP + -0x18);
        if (0.0 < *(float *)(unaff_EBX + 0xb0)) {
          *(float *)(iVar8 + 0x1c) = *(float *)(unaff_EBX + 0xb0) * *(float *)(iVar8 + 0x1c);
          *(float *)(iVar8 + 0x10) = *(float *)(unaff_EBX + 0xb0) * *(float *)(iVar8 + 0x10);
          *(float *)(iVar8 + 0x14) = *(float *)(unaff_EBX + 0xb0) * *(float *)(iVar8 + 0x14);
          *(float *)(iVar8 + 0x18) = *(float *)(unaff_EBX + 0xb0) * *(float *)(iVar8 + 0x18);
        }
        if (*(int *)(*(int *)(unaff_EBP + -0x1c) + 0x44) != -1) {
          FUN_004f4250(*(undefined4 *)(unaff_EBP + 8),iVar8);
        }
        if (0 < *(short *)(unaff_EBX + 0xd6)) {
          model_nodes_blend_transforms
                    (*(short *)(unaff_EBX + 0x1ea) + unaff_EBX,*(undefined2 *)(unaff_EBX + 0xd4),
                     *(short *)(unaff_EBX + 0xd6));
        }
        iVar8 = 0;
        *(undefined4 *)(unaff_EBP + -0xc) = 1;
        *(undefined2 *)(unaff_EBP + -0x20c) = 0;
        while( true ) {
          sVar9 = *(short *)(unaff_EBP + -0x20c + (short)iVar8 * 2);
          *(int *)(unaff_EBP + -0x20) = iVar8 + 1;
          iVar8 = sVar9 * 0x9c + *(int *)(*(int *)(unaff_EBP + -0x28) + 0xbc);
          *(int *)(unaff_EBP + 8) = iVar8;
          if (sVar9 == 0) {
            matrix4x3_from_quaternion();
            uVar4 = *(undefined4 *)(extraout_ECX + 0x10);
            *(undefined4 *)(unaff_EBP + -0x154) = *(undefined4 *)(extraout_ECX + 0x1c);
            uVar5 = *(undefined4 *)(extraout_ECX + 0x14);
            *(undefined4 *)(unaff_EBP + -300) = uVar4;
            uVar4 = *(undefined4 *)(extraout_ECX + 0x18);
            *(undefined4 *)(unaff_EBP + -0x128) = uVar5;
            *(undefined4 *)(unaff_EBP + -0x124) = uVar4;
            if (*(char *)(unaff_EBP + -0xd) == '\0') {
              uVar4 = *(undefined4 *)(unaff_EBX + 0x5c);
              uVar5 = *(undefined4 *)(unaff_EBX + 100);
              *(undefined4 *)(unaff_EBP + -0x48) = *(undefined4 *)(unaff_EBX + 0x60);
              *(undefined4 *)(unaff_EBP + -0x4c) = uVar4;
              *(undefined4 *)(unaff_EBP + -0x44) = uVar5;
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
              FUN_004cb970(unaff_EBP + -0x11c);
              if ((*(uint *)(unaff_EBX + 0x10) & 0x1000) != 0) {
                *(float *)(unaff_EBP + -0x10c) = -*(float *)(unaff_EBP + -0x10c);
                *(float *)(unaff_EBP + -0x108) = -*(float *)(unaff_EBP + -0x108);
                *(float *)(unaff_EBP + -0x104) = -*(float *)(unaff_EBP + -0x104);
              }
              uVar6 = *(uint *)(*(int *)(unaff_EBP + -0x1c) + 0x8c);
              if (uVar6 != 0xffffffff) {
                iVar7 = *(int *)((uVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
                *(float *)(unaff_EBP + -0x38) = -*(float *)(iVar7 + 0xc);
                *(float *)(unaff_EBP + -0x34) = -*(float *)(iVar7 + 0x10);
                fVar2 = *(float *)(iVar7 + 0x14);
                *(undefined4 *)(unaff_EBP + -0xbc) = *(undefined4 *)(unaff_EBP + -0x38);
                *(float *)(unaff_EBP + -0x30) = -fVar2;
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
                (*(code *)PTR_matrix4x3_multiply_00696664)
                          (unaff_EBP + -0x11c,unaff_EBP + -0xe4,unaff_EBP + -0x11c);
              }
              iVar7 = *(int *)(unaff_EBP + -0x1c);
              uVar4 = *(undefined4 *)(iVar7 + 0x18);
              uVar5 = *(undefined4 *)(iVar7 + 0x1c);
              *(undefined4 *)(unaff_EBP + -0x84) = *(undefined4 *)(iVar7 + 0x14);
              *(undefined4 *)(unaff_EBP + -0x80) = uVar4;
              *(undefined4 *)(unaff_EBP + -0x7c) = uVar5;
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
              (*(code *)PTR_matrix4x3_multiply_00696664)
                        (unaff_EBP + -0x11c,unaff_EBP + -0xac,unaff_EBP + -0x11c);
              if (pfVar10 == (float *)0x0) {
                (*(code *)PTR_matrix4x3_multiply_00696664)
                          (unaff_EBP + -0x74,unaff_EBP + -0x11c,*(undefined4 *)(unaff_EBP + -0x14));
                (*(code *)PTR_matrix4x3_multiply_00696664)
                          (*(undefined4 *)(unaff_EBP + -0x14),unaff_EBP + -0x154,
                           *(undefined4 *)(unaff_EBP + -0x14));
              }
              else {
                pfVar11 = pfVar10;
                if (*pfVar10 != 1.0) {
                  fVar2 = *pfVar10;
                  pfVar11 = (float *)(unaff_EBP + -0x18c);
                  *(float **)(unaff_EBP + -8) = pfVar11;
                  *(float *)(unaff_EBP + -0x4c) = *(float *)(unaff_EBP + -0x4c) * fVar2;
                  *(float *)(unaff_EBP + -0x48) = *(float *)(unaff_EBP + -0x48) * *pfVar10;
                  fVar2 = *(float *)(unaff_EBP + -0x44);
                  fVar3 = *pfVar10;
                  pfVar13 = (float *)(unaff_EBP + -0x18c);
                  for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
                    *pfVar13 = *pfVar10;
                    pfVar10 = pfVar10 + 1;
                    pfVar13 = pfVar13 + 1;
                  }
                  *(float *)(unaff_EBP + -0x44) = fVar2 * fVar3;
                  iVar8 = *(int *)(unaff_EBP + 8);
                  *(undefined4 *)(unaff_EBP + -0x18c) = 0x3f800000;
                }
                if ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                       (*(uint *)(unaff_EBX + 0x11c) & 0xffff) * 0xc) + 0x10) &
                    0x1000) != 0) {
                  pfVar10 = (float *)(unaff_EBP + -0x18c);
                  if (pfVar11 != pfVar10) {
                    pfVar13 = pfVar10;
                    for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
                      *pfVar13 = *pfVar11;
                      pfVar11 = pfVar11 + 1;
                      pfVar13 = pfVar13 + 1;
                    }
                    iVar8 = *(int *)(unaff_EBP + 8);
                    *(float **)(unaff_EBP + -8) = pfVar10;
                    pfVar11 = pfVar10;
                  }
                  pfVar11[4] = -pfVar11[4];
                  pfVar11[5] = -pfVar11[5];
                  pfVar11[6] = -pfVar11[6];
                }
                (*(code *)PTR_matrix4x3_multiply_00696664)
                          (pfVar11,unaff_EBP + -0x74,*(undefined4 *)(unaff_EBP + -0x14));
                (*(code *)PTR_matrix4x3_multiply_00696664)
                          (*(undefined4 *)(unaff_EBP + -0x14),unaff_EBP + -0x11c,
                           *(undefined4 *)(unaff_EBP + -0x14));
                (*(code *)PTR_matrix4x3_multiply_00696664)
                          (*(undefined4 *)(unaff_EBP + -0x14),unaff_EBP + -0x154,
                           *(undefined4 *)(unaff_EBP + -0x14));
                pfVar10 = pfVar11;
              }
            }
            else {
              puVar12 = (undefined4 *)(unaff_EBP + -0x154);
              puVar14 = *(undefined4 **)(unaff_EBP + -0x14);
              for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
                *puVar14 = *puVar12;
                puVar12 = puVar12 + 1;
                puVar14 = puVar14 + 1;
              }
              iVar8 = *(int *)(unaff_EBP + 8);
              pfVar10 = *(float **)(unaff_EBP + -8);
            }
          }
          else {
            matrix4x3_from_quaternion();
            *extraout_EDX = *(undefined4 *)(extraout_ECX_00 + 0x1c);
            extraout_EDX[10] = *(undefined4 *)(extraout_ECX_00 + 0x10);
            extraout_EDX[0xb] = *(undefined4 *)(extraout_ECX_00 + 0x14);
            iVar8 = *(int *)(unaff_EBP + 8);
            extraout_EDX[0xc] = *(undefined4 *)(extraout_ECX_00 + 0x18);
            (*(code *)PTR_matrix4x3_multiply_00696664)
                      (*(short *)(iVar8 + 0x24) * 0x34 + *(int *)(unaff_EBP + -0x14),extraout_EDX,
                       extraout_EDX);
          }
          iVar7 = *(int *)(unaff_EBP + -0xc);
          if (*(short *)(iVar8 + 0x20) != -1) {
            sVar9 = (short)iVar7;
            iVar7 = iVar7 + 1;
            *(short *)(unaff_EBP + -0x20c + sVar9 * 2) = *(short *)(iVar8 + 0x20);
            *(int *)(unaff_EBP + -0xc) = iVar7;
          }
          if (*(short *)(iVar8 + 0x22) != -1) {
            sVar9 = (short)iVar7;
            iVar7 = iVar7 + 1;
            *(short *)(unaff_EBP + -0x20c + sVar9 * 2) = *(short *)(iVar8 + 0x22);
            *(int *)(unaff_EBP + -0xc) = iVar7;
          }
          if (*(short *)(unaff_EBP + -0x20) == (short)iVar7) break;
          iVar8 = *(int *)(unaff_EBP + -0x20);
        }
        iVar8 = *(int *)(unaff_EBP + -0x1c);
        matrix4x3_transform_point(*(undefined4 *)(unaff_EBP + -0x14));
        fVar2 = *(float *)(iVar8 + 4);
        *(float *)(unaff_EBX + 0xac) = fVar2;
        if (0.0 < *(float *)(unaff_EBX + 0xb0)) {
          *(float *)(unaff_EBX + 0xac) = fVar2 * *(float *)(unaff_EBX + 0xb0);
          return;
        }
        return;
      }
      psVar1 = (short *)(unaff_ESI[1] + iVar7 * 0x14);
    } while ((*psVar1 == -1) ||
            (unaff_ESI = *(int **)(unaff_EBP + -0x2c),
            *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x158) <= (int)psVar1[1]));
    unaff_EDI = *psVar1 * 0xb4 + unaff_ESI[0x1e];
    in_ECX = (int)psVar1[1];
    in_AX = psVar1[2];
  } while( true );
}
#endif
