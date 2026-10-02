// object_update_functions_clone_4f93b0   (renamed by the phase-4 review pass; Ghidra/PDB name was `object_delete_to_network`,
//    which this file's own header shows does not describe the code)
// address 0x4f93b0, size 314 bytes
// name confidence: 0.1 (inherited Ghidra name; out/phase4/objects_types_notes.md: "0x4f93b0,
// 0x4f94e0, 0x4f9540: byte-identical copies of the per-node periodic_function_evaluate loop;
// the inherited names object_delete_to_network, object_delete and object_reconnect_to_map do
// not match a function-value recompute.")
// rewrite confidence: 1.0 (FRAGMENT: 0x4f93b0 lies inside object_update_functions 0x4f92f0..0x4f9690, whose C covers it; only jumps reach here)
//
// This is not a real, independently-callable function: it has zero recorded callers, and every
// input arrives as an unresolved "unaff_"/"in_stack_" value with no call site anywhere to check
// a real parameter list against. Its body is byte-identical to the per-function evaluation loop
// in object_update_functions (0x4f92f0, this batch) -- a compiler-cloned duplicate, not
// distinct engine logic (it does not recompute anything "to network" despite the inherited
// name). See object_update_functions.c for the rewrite of that logic; nothing here is
// translated independently to avoid a second, unverifiable copy.

#if 0
Original Ghidra decompilation (0x4f93b0):

void object_delete_to_network(void)

{
  float fVar1;
  short sVar2;
  int iVar3;
  byte unaff_BL;
  int unaff_EBP;
  int iVar4;
  uint *unaff_ESI;
  int unaff_EDI;
  float10 fVar5;
  float10 in_ST1;
  float fStack00000010;
  float in_stack_00000014;
  short in_stack_00000018;
  int in_stack_0000001c;

  while( true ) {
    fVar5 = (float10)periodic_function_evaluate((double)(in_ST1 * (float10)in_stack_00000014));
    sVar2 = (short)unaff_ESI[3];
    fStack00000010 = (float)fVar5;
    if (sVar2 != 0) {
      if (sVar2 < 5) {
        fVar1 = *(float *)(unaff_EDI + 0x120 + sVar2 * 4);
      }
      else {
        fVar1 = *(float *)(unaff_EDI + 0x120 + sVar2 * 4);
      }
      fStack00000010 = fVar1 * fStack00000010;
    }
    if ((*unaff_ESI & 1) != 0) {
      fStack00000010 = 1.0 - fStack00000010;
    }
    if ((float)unaff_ESI[5] != 0.0) {
      fVar5 = (float10)periodic_function_evaluate((double)(in_stack_00000014 * (float)unaff_ESI[4]))
      ;
      fVar5 = (fVar5 - (float10)0.5) * (float10)(float)unaff_ESI[5];
      fStack00000010 = (float)(fVar5 + fVar5 + (float10)fStack00000010);
    }
    fVar1 = fStack00000010;
    if (((float)unaff_ESI[6] != 0.0) && (fStack00000010 = 1.0, fVar1 <= (float)unaff_ESI[6])) {
      fStack00000010 = 0.0;
    }
    if (1 < (short)unaff_ESI[7]) {
      FUN_00623e40((double)((float)(int)(short)unaff_ESI[7] * fStack00000010));
    }
    if (0.0 < (float)unaff_ESI[0x4f]) {
      FUN_00628cca();
    }
    fVar5 = (float10)FUN_004ccac0();
    fStack00000010 = (float)fVar5;
    if (0.0 < (float)unaff_ESI[0xe]) {
      fStack00000010 = fStack00000010 * (float)unaff_ESI[0xe];
    }
    if (*(short *)((int)unaff_ESI + 0x26) == 2) {
      fStack00000010 =
           ((float)unaff_ESI[0xb] - (float)unaff_ESI[10]) * fStack00000010 + (float)unaff_ESI[10];
      if (fStack00000010 <= (float)unaff_ESI[10] + 0.0001) {
        unaff_BL = (byte)(*unaff_ESI >> 2) & 1;
      }
    }
    else {
      if (fStack00000010 <= (float)unaff_ESI[10] + 0.0001) {
        fStack00000010 = (float)unaff_ESI[10];
        unaff_BL = (byte)(*unaff_ESI >> 2) & 1;
      }
      if ((float)unaff_ESI[0xb] < fStack00000010) {
        fStack00000010 = (float)unaff_ESI[0xb];
      }
      if (*(short *)((int)unaff_ESI + 0x26) == 1) {
        fStack00000010 = (fStack00000010 - (float)unaff_ESI[10]) * (float)unaff_ESI[0x4e];
      }
    }
    if ((*(short *)((int)unaff_ESI + 0x36) != -1) &&
       ((*(byte *)(unaff_EDI + 0x123) &
        (byte)(1 << ((byte)*(short *)((int)unaff_ESI + 0x36) & 0x1f))) == 0)) {
      unaff_BL = 0;
    }
    if ((*unaff_ESI & 2) != 0) {
      fVar5 = (float10)FUN_00628cca();
      fStack00000010 = (float)fVar5;
    }
    *(float *)(unaff_EDI + 0x134 + unaff_EBP * 4) = fStack00000010;
    if (unaff_BL == 0) {
      *(byte *)(unaff_EDI + 0x123) =
           *(byte *)(unaff_EDI + 0x123) & ~('\x01' << ((byte)unaff_EBP & 0x1f));
    }
    else {
      *(byte *)(unaff_EDI + 0x123) =
           *(byte *)(unaff_EDI + 0x123) | '\x01' << ((byte)unaff_EBP & 0x1f);
    }
    in_stack_00000018 = in_stack_00000018 + 1;
    unaff_EBP = (int)in_stack_00000018;
    if (*(int *)(in_stack_0000001c + 0x158) <= unaff_EBP) break;
    iVar3 = *(int *)(in_stack_0000001c + 0x15c);
    iVar4 = unaff_EBP * 0x168;
    in_ST1 = (float10)*(float *)(iVar4 + 0x144 + iVar3);
    sVar2 = *(short *)(iVar4 + 8 + iVar3);
    unaff_ESI = (uint *)(iVar4 + iVar3);
    unaff_BL = 1;
    if (sVar2 != 0) {
      if (sVar2 < 5) {
        fVar1 = *(float *)(unaff_EDI + 0x120 + sVar2 * 4);
      }
      else {
        fVar1 = *(float *)(unaff_EDI + 0x120 + sVar2 * 4);
      }
      if ((float10)0.0 < (float10)fVar1) {
        in_ST1 = in_ST1 / (float10)fVar1;
      }
    }
  }
  return;
}
#endif

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
