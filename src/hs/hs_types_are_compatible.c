// hs_types_are_compatible  (Ghidra: hs_types_are_compatible, already named)
// address 0x48ac90, size 125 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: types/hs.h hs_type (object family 0x25..0x2a, object-name family 0x2b..0x30) and
//   hs_type_conversion_procedures (0x0068bc10, [destination][source]); this module's
//   hs_type_mask_is_subset (0x48ac60).
// REWRITTEN (objdump 0x48ac90..0x48ad0c; the draft took (source, dest) while every caller -- postprocess,
//   hs_parse_nonprimitive, hs_parse_set -- passes (dest, source), so script postprocessing failed with "type is
//   inconsistent with usage" and fell back to recompiling from source; it also biased a name source used as an
//   object by 0x2b on both sides). CX = the expected (dest) type, AX = the actual (source) type. Passthrough or
//   equal types are compatible. An object dest takes an object source by type mask (hs_type_mask_is_subset(AX
//   source - 0x25, CX dest - 0x25)) or an object-name source (source - 0x2b, dest - 0x25); an object-name dest
//   takes only an object-name source (both - 0x2b); any other dest needs a conversion procedure
//   hs_type_conversion_procedures[dest][source] (0x0068bc10).
// blam-cc: CX -> dest_type, AX -> source_type

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char hs_type_mask_is_subset(int16_t subtype_index, int16_t supertype_index);
    // this module, 0x48ac60
extern int32_t (*hs_type_conversion_procedures[k_hs_type_count][k_hs_type_count])(int32_t value);
    // 0x0068bc10, indexed [destination_type][source_type]

// A value of `source_type` can be used where `dest_type` is expected if: they are the same type,
// or source_type is "passthrough"; or (for ordinary value types) the conversion table has a
// procedure for [dest_type][source_type]; or (for the object and object-name families) one
// type's bitmask is a subset of the other's, via hs_type_mask_is_subset.
char hs_types_are_compatible(hs_type_t dest_type, hs_type_t source_type)
{
    if (source_type == _hs_type_passthrough || source_type == dest_type) {
        return 1;
    }
    if (0x24 < dest_type && dest_type < 0x2b) {
        if (0x24 < source_type && source_type < 0x2b) {
            return hs_type_mask_is_subset(source_type - 0x25, dest_type - 0x25);
        }
        if (0x2a < source_type && source_type < 0x31) {
            return hs_type_mask_is_subset(source_type - 0x2b, dest_type - 0x25);
        }
        return 0;
    }
    if (0x2a < dest_type && dest_type < 0x31) {
        if (source_type < 0x2b || 0x30 < source_type) {
            return 0;
        }
        return hs_type_mask_is_subset(source_type - 0x2b, dest_type - 0x2b);
    }
    return hs_type_conversion_procedures[dest_type][source_type] != 0;
}

#if 0
Original Ghidra decompilation (0x48ac90):

undefined1 hs_types_are_compatible(void)

{
  undefined1 uVar1;
  short in_AX;
  short in_CX;

  if ((in_AX == 3) || (in_AX == in_CX)) {
    return true;
  }
  if ((in_CX < 0x25) || (0x2a < in_CX)) {
    if ((in_CX < 0x2b) || (0x30 < in_CX)) {
      return *(int *)(&DAT_0068bc10 + (in_CX * 0x31 + (int)in_AX) * 4) != 0;
    }
  }
  else if ((0x24 < in_AX) && (in_AX < 0x2b)) {
    uVar1 = hs_type_mask_is_subset();
    return uVar1;
  }
  if ((0x2a < in_AX) && (in_AX < 0x31)) {
    uVar1 = hs_type_mask_is_subset();
    return uVar1;
  }
  return false;
}
#endif
