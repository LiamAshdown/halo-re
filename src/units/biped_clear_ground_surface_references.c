// biped_clear_ground_surface_references  (Ghidra: no function created; the phase-4 types agent
//   carved a stub "missed_559f70" from the object_type_definition vtable evidence)
// VERIFIED against disassembly 0x559f70..0x559f9f (2026-09-30)
// address 0x559f70, size 47 bytes
// name confidence 0.35, rewrite confidence 0.7
// evidence: out/phase4/units_types_notes.md: "The biped row's +0x38, +0x50 and +0x54 columns are
//   0x559e40, 0x559f10 and 0x559f70". This is the +0x54 column, a much smaller sibling of
//   biped_reset_state.c (+0x50): rather than zeroing the whole biped_data extension it only
//   invalidates the three datum_index-shaped ground/look-at fields types/units.h already names
//   -- ground_surface_index (+0x4d8, "the supporting surface ...; -1 when airborne"),
//   unknown_4dc (+0x4dc, "the cached look-at result") and unknown_4f0 (+0x4f0, "the previous
//   value of unknown_4dc") -- to the standard datum_index invalid sentinel (-1 / 0xffffffff).
// register convention: object index in a single register argument (matches every other biped_*
//   per-object helper in this address range); blam-cc: object_index only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

// object_type_definition "biped" row, +0x54 column. Invalidates the cached ground-surface and
// look-at datum indices without touching the rest of biped_data.
void biped_clear_ground_surface_references(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    biped->ground_surface_index = (datum_index)-1;
    biped->cached_ground_surface_index = (datum_index)-1;
    biped->last_ground_surface_index = (datum_index)-1;
}

#if 0
Original Ghidra decompilation (0x559f70):

void missed_559f70(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  *(undefined4 *)(iVar1 + 0x4d8) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x4dc) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x4f0) = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
