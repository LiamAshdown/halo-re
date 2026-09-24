// object_recalculate_bounding_radius_clone_4f8a70   (renamed by the phase-4 review pass; Ghidra/PDB name was `object_reset`,
//    which this file's own header shows does not describe the code)
// address 0x4f8a70, size 148 bytes
// name confidence: 0.1 (inherited Ghidra name; out/phase4/objects_types_notes.md: "0x4f84e2,
// 0x4f8834, 0x4f8a70: compiler-cloned variants of object_recalculate_bounding_radius 0x4f8310;
// all three end with the same write to object+0xac scaled by object+0xb0, and all three have
// zero recorded callers. Their inherited names (objects_initialize_for_new_map_mod_processed_bsps,
// objects_update__object_in_player_pvs_nop1, object_reset) do not match.")
// rewrite confidence: n/a -- not independently rewritten
//
// This is not a real, independently-callable function: it has zero recorded callers, and every
// input arrives as an unresolved "unaff_"/"in_" register with no call site to check the
// convention against. Its body is the same sequence as the simple (no animation graph) branch
// of object_recalculate_bounding_radius (0x4f8310, this batch): build a rotation matrix from
// up/forward via a cross product, copy position, matrix4x3_transform_point, then write
// bounding_radius scaled by scale. See object_recalculate_bounding_radius.c for the full,
// correctly-parameterized rewrite of that logic; nothing here is translated independently to
// avoid a second, unverifiable copy of the same code under a name (object_reset) that does not
// describe it.

#if 0
Original Ghidra decompilation (0x4f8a70):

void object_reset(void)

{
  float fVar1;
  int in_EAX;
  int in_ECX;
  undefined4 in_EDX;
  int unaff_EBX;
  int unaff_ESI;
  int unaff_EDI;

  *(undefined4 *)(in_EAX + 4) = in_EDX;
  *(undefined4 *)(in_EAX + 8) = *(undefined4 *)(in_ECX + 8);
  *(undefined4 *)(unaff_ESI + 0x1c) = *(undefined4 *)(unaff_EBX + 0x80);
  *(undefined4 *)(unaff_ESI + 0x20) = *(undefined4 *)(unaff_EBX + 0x84);
  *(undefined4 *)(unaff_ESI + 0x24) = *(undefined4 *)(unaff_EBX + 0x88);
  vector3d_cross_product(unaff_ESI + 0x1c);
  *(undefined4 *)(unaff_ESI + 0x28) = *(undefined4 *)(unaff_EBX + 0x5c);
  *(undefined4 *)(unaff_ESI + 0x2c) = *(undefined4 *)(unaff_EBX + 0x60);
  *(undefined4 *)(unaff_ESI + 0x30) = *(undefined4 *)(unaff_EBX + 100);
  matrix4x3_transform_point();
  fVar1 = *(float *)(unaff_EDI + 4);
  *(float *)(unaff_EBX + 0xac) = fVar1;
  if (0.0 < *(float *)(unaff_EBX + 0xb0)) {
    *(float *)(unaff_EBX + 0xac) = fVar1 * *(float *)(unaff_EBX + 0xb0);
    return;
  }
  return;
}
#endif
