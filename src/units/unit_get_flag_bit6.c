// unit_get_flag_bit6  (Ghidra: FUN_00569bc0)
// address 0x569bc0, size 34 bytes, name confidence 0.25, rewrite confidence 0.6
// functions.md: "Returns a single flag bit (bit 6) from the unit's 0x204 flags dword."
// evidence: types/units.h unit_data.flags (0x204); bit 0x40 is not named by any function in
// this module's enum list.
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint32_t unit_get_flag_bit6(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return (unit->flags >> 6) & 1;
}

#if 0
Original Ghidra decompilation (0x569bc0):

uint FUN_00569bc0(void)

{
  uint in_EAX;

  return *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x204) >>
         6 & 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
