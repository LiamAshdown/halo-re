// hs_type_mask_is_subset  (Ghidra: hs_type_mask_is_subset, already named)
// address 0x48ac60, size 38 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/hs.h hs_object_type_masks (0x00657538, 6 entries, biased by _hs_type_object or
//   _hs_type_object_name -- the bias is applied by the caller, per hs_types_are_compatible's own
//   raw bytes cited in out/phase4/hs_types_notes.md).
// register convention: subtype index in AX (in_AX); supertype index in CX (in_CX). Both are
//   already-biased indices into hs_object_type_masks[6], not raw hs_type values.
//   // blam-cc: AX -> subtype_index, CX -> supertype_index

#include "tags.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t hs_object_type_masks[6]; // 0x00657538

// Returns 1 if every bit set in hs_object_type_masks[subtype_index] is also set in
// hs_object_type_masks[supertype_index] (i.e. subtype_index's object family is a subset of
// supertype_index's), used for polymorphic type compatibility such as 'object' encompassing
// 'unit'/'vehicle'.
char hs_type_mask_is_subset(int16_t subtype_index, int16_t supertype_index)
{
    uint16_t subtype_mask;

    subtype_mask = hs_object_type_masks[subtype_index];
    return (hs_object_type_masks[supertype_index] & subtype_mask) == subtype_mask;
}

#if 0
Original Ghidra decompilation (0x48ac60):

undefined4 hs_type_mask_is_subset(void)

{
  ushort uVar1;
  short in_AX;
  short in_CX;

  uVar1 = *(ushort *)(&DAT_00657538 + in_AX * 2);
  return CONCAT31((int3)(char)(uVar1 >> 8),
                  '\x01' - ((*(ushort *)(&DAT_00657538 + in_CX * 2) & uVar1) != uVar1));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
