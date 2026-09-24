// object_set_position_network_clone_4f8cb0   (renamed by the phase-4 review pass; Ghidra/PDB name was `object_placement_data_new`,
//    which this file's own header shows does not describe the code)
// address 0x4f8cb0, size 176 bytes
// name confidence: 0.2 (inherited Ghidra name; functions.md: "A variant of the network
//   position update loop that clamps the same derived blend weights; appears to duplicate
//   object_set_position_network's logic")
// rewrite confidence: n/a -- not independently rewritten
//
// Zero recorded callers, and the only "parameter" Ghidra can identify is unaff_EDI, live at
// function entry with no call site anywhere to check a real signature against (confirmed via
// objdump -d -M intel bin/halo.exe: the function's first instructions read only file-local
// float constants, never a stack slot or a fresh register load). Its body is the same
// clamp-to-[0,1] / threshold-table / color_interpolate sequence as
// object_set_position_network (0x4f8bd0, this batch), just re-emitted with a different loop
// entry point (a mid-loop goto target as its "start"). Treated the same way this module treats
// its other zero-caller compiler duplicates (see object_reset.c,
// objects_initialize_for_new_map_mod_processed_bsps.c,
// objects_update__object_in_player_pvs_nop1.c): the real, parameterized logic lives in
// object_set_position_network.c and is not re-translated here.

#if 0
Original Ghidra decompilation (0x4f8cb0):

void object_placement_data_new(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  float *pfVar5;
  int iVar6;
  float *unaff_EDI;
  float *pfVar7;
  float10 fVar8;
  int in_stack_00000010;
  int in_stack_00000014;
  float *in_stack_00000018;
  float fStack0000001c;
  int in_stack_00000020;
  int in_stack_00000024;

  do {
    fVar1 = 0.0;
    pfVar7 = unaff_EDI;
LAB_004f8cd1:
    pfVar7[0xc] = fVar1;
    if (0.0 <= pfVar7[1]) {
      if (pfVar7[1] <= 1.0) {
        fVar1 = pfVar7[1];
      }
      else {
        fVar1 = 1.0;
      }
    }
    else {
      fVar1 = 0.0;
    }
    pfVar7[0xd] = fVar1;
    if (0.0 <= pfVar7[2]) {
      if (pfVar7[2] <= 1.0) {
        fVar1 = pfVar7[2];
      }
      else {
        fVar1 = 1.0;
      }
    }
    else {
      fVar1 = 0.0;
    }
    pfVar7[0xe] = fVar1;
    in_stack_00000010 = in_stack_00000010 + 1;
    in_stack_00000014 = in_stack_00000014 + 0x2c;
    pfVar5 = in_stack_00000018 + 3;
    unaff_EDI = pfVar7 + 3;
    in_stack_00000020 = in_stack_00000020 + -1;
    if (in_stack_00000020 == 0) {
      return;
    }
    *unaff_EDI = *pfVar5;
    pfVar7[4] = in_stack_00000018[4];
    pfVar7[5] = in_stack_00000018[5];
    if (in_stack_00000010 < *(int *)(in_stack_00000024 + 0x164)) {
      fStack0000001c = (float)in_stack_00000010;
      iVar6 = *(int *)(in_stack_00000024 + 0x168) + in_stack_00000014;
      fVar8 = (float10)FUN_00628cca();
      iVar2 = *(int *)(iVar6 + 0x20);
      sVar4 = 0;
      if (0 < iVar2) {
        iVar3 = 0;
        do {
          fVar1 = *(float *)(iVar3 * 0x1c + *(int *)(iVar6 + 0x24));
          if ((float)fVar8 < fVar1 != ((float)fVar8 == fVar1)) {
            fVar8 = (float10)FUN_00628cca();
            color_interpolate(unaff_EDI,1,(float)fVar8);
            break;
          }
          sVar4 = sVar4 + 1;
          iVar3 = (int)sVar4;
        } while (iVar3 < iVar2);
      }
    }
    in_stack_00000018 = pfVar5;
  } while (*unaff_EDI < 0.0);
  pfVar7 = unaff_EDI;
  if (*unaff_EDI <= 1.0) {
    fVar1 = *unaff_EDI;
  }
  else {
    fVar1 = 1.0;
  }
  goto LAB_004f8cd1;
}
#endif
