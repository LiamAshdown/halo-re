// unit_current_weapon_is_type  (Ghidra: unit_current_weapon_is_type, already named)
// address 0x561f80, size 86 bytes
// name confidence: 0.5 (already carries this name)   rewrite confidence: 0.95 (VERIFIED against objdump)
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), unit_data.weapons[4] (0x2f8);
//   types/objects.h object.definition_tag (0x000, the Object tag every object carries).
// register convention: unit index in ECX, comparison tag id in EDI (unaff_EDI, i.e. carried
//   over from the caller's register rather than pushed).
//   // blam-cc: in_ECX -> unit_index, unaff_EDI -> weapon_tag_id
// UNSURE: the return value's upper 24 bits are whatever garbage the original left in EAX
//   (CONCAT31); only AL is meaningful, matching the house simplification used throughout this
//   codebase (see src/memory/bit_stream_write_bit.c) -- reproduced here as a plain uint8_t.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint8_t unit_current_weapon_is_type(uint32_t unit_index, datum_index weapon_tag_id) // blam-cc: see file header
{
    if (unit_index == (uint32_t)-1 || weapon_tag_id == (datum_index)-1) {
        return 0;
    }

    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot = unit->current_weapon_index;

    if (slot == -1) {
        return 0;
    }

    datum_index weapon_index = unit->weapons[slot];
    if (weapon_index == (datum_index)-1) {
        return 0;
    }

    object *weapon = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
    return weapon->definition_tag == weapon_tag_id;
}

#if 0
Original Ghidra decompilation (0x561f80):

uint unit_current_weapon_is_type(void)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  uint uVar3;
  uint in_ECX;
  uint uVar4;
  int unaff_EDI;

  uVar3 = in_EAX & 0xffffff00;
  if ((in_ECX != 0xffffffff) && (unaff_EDI != -1)) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    sVar1 = *(short *)(iVar2 + 0x2f2);
    if ((sVar1 != -1) && (uVar4 = *(uint *)(iVar2 + 0x2f8 + sVar1 * 4), uVar4 != 0xffffffff)) {
      uVar4 = uVar4 & 0xffff;
      uVar3 = CONCAT31((int3)(uVar4 * 3 >> 8),
                       **(int **)(*(int *)(DAT_008603b0 + 0x34) + 8 + uVar4 * 0xc) == unaff_EDI);
    }
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
