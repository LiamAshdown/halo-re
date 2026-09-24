// object_update_functions_clone_4f9540   (renamed by the phase-4 review pass; Ghidra/PDB name was `object_reconnect_to_map`,
//    which this file's own header shows does not describe the code)
// address 0x4f9540, size 337 bytes
// name confidence: 0.1 (inherited Ghidra name; out/phase4/objects_types_notes.md: "0x4f93b0,
// 0x4f94e0, 0x4f9540: byte-identical copies of the per-node periodic_function_evaluate loop;
// the inherited names object_delete_to_network, object_delete and object_reconnect_to_map do
// not match a function-value recompute.")
// rewrite confidence: n/a -- not independently rewritten
//
// This is not a real, independently-callable function: it has zero recorded callers, and every
// input arrives as an unresolved "unaff_"/"in_stack_" value with no call site anywhere to check
// a real parameter list against. Its body is byte-identical to object_delete.c (this batch),
// which is itself byte-identical to the per-function evaluation loop in object_update_functions
// (0x4f92f0, this batch) -- a compiler-cloned duplicate, not distinct engine logic (nothing
// here reconnects an object to the map despite the inherited name). See
// object_update_functions.c for the rewrite of that logic; nothing here is translated
// independently to avoid a third, unverifiable copy.

#if 0
Original Ghidra decompilation (0x4f9540):

void object_reconnect_to_map(void)

{
  float fVar1;
  float fVar2;
  short sVar3;
  int iVar4;
  byte unaff_BL;
  int unaff_EBP;
  int iVar5;
  uint *unaff_ESI;
  int unaff_EDI;
  float10 fVar6;
  float fStack00000010;
  float in_stack_00000014;
  short in_stack_00000018;
  int in_stack_0000001c;

  while( true ) {
    fVar6 = (float10)FUN_004ccac0();
    fStack00000010 = (float)fVar6;
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
      fVar6 = (float10)FUN_00628cca();
      fStack00000010 = (float)fVar6;
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
    iVar4 = *(int *)(in_stack_0000001c + 0x15c);
    iVar5 = unaff_EBP * 0x168;
    fVar1 = *(float *)(iVar5 + 0x144 + iVar4);
    sVar3 = *(short *)(iVar5 + 8 + iVar4);
    unaff_ESI = (uint *)(iVar5 + iVar4);
    unaff_BL = 1;
    if (sVar3 != 0) {
      if (sVar3 < 5) {
        fVar2 = *(float *)(unaff_EDI + 0x120 + sVar3 * 4);
      }
      else {
        fVar2 = *(float *)(unaff_EDI + 0x120 + sVar3 * 4);
      }
      if (0.0 < fVar2) {
        fVar1 = fVar1 / fVar2;
      }
    }
    fVar6 = (float10)periodic_function_evaluate((double)(fVar1 * in_stack_00000014));
    sVar3 = (short)unaff_ESI[3];
    fStack00000010 = (float)fVar6;
    if (sVar3 != 0) {
      if (sVar3 < 5) {
        fVar1 = *(float *)(unaff_EDI + 0x120 + sVar3 * 4);
      }
      else {
        fVar1 = *(float *)(unaff_EDI + 0x120 + sVar3 * 4);
      }
      fStack00000010 = fVar1 * fStack00000010;
    }
    if ((*unaff_ESI & 1) != 0) {
      fStack00000010 = 1.0 - fStack00000010;
    }
    if ((float)unaff_ESI[5] != 0.0) {
      fVar6 = (float10)periodic_function_evaluate((double)(in_stack_00000014 * (float)unaff_ESI[4]))
      ;
      fVar6 = (fVar6 - (float10)0.5) * (float10)(float)unaff_ESI[5];
      fStack00000010 = (float)(fVar6 + fVar6 + (float10)fStack00000010);
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
  }
  return;
}
#endif
