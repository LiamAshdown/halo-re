// unit_get_grenade_count  (Ghidra: unit_get_grenade_count, already named)
// address 0x56e030, size 44 bytes, name confidence 0.5, rewrite confidence 0.8
// functions.md: "Returns the ammo count for a given grenade-type slot (index in CX) held by the
// unit."
// evidence: types/units.h unit_data.grenade_counts[2] (0x31e).
// blam-cc: in_EAX -> unit_index, in_CX -> grenade_type.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

int32_t unit_get_grenade_count(uint32_t unit_index, int16_t grenade_type) // blam-cc: in_EAX, in_CX
{
    if (grenade_type == -1) {
        return 0;
    }
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return unit->grenade_counts[grenade_type];
}

#if 0
Original Ghidra decompilation (0x56e030):

uint unit_get_grenade_count(void)

{
  uint in_EAX;
  short in_CX;

  if (in_CX != -1) {
    return CONCAT22((short)((in_EAX & 0xffff) * 3 >> 0x10),
                    (short)*(char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                             (in_EAX & 0xffff) * 0xc) + 0x31e + (int)in_CX));
  }
  return in_EAX & 0xffff0000;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
