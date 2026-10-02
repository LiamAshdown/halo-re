// unit_get_aiming_vector  (Ghidra: FUN_005696f0)
// address 0x5696f0, size 45 bytes, name confidence 0.4, rewrite confidence 0.7
// evidence: types/units.h unit_data.aiming_vector (0x23c, "the current aim; 0x5696f0 returns it
// and unit_release_thrown_grenade launches along it").
// blam-cc: in_ECX -> unit_index, in_EAX -> out.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

void unit_get_aiming_vector(uint32_t unit_index, real_vector3d *out) // blam-cc: in_ECX, in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    *out = unit->aiming_vector;
    return;
}

#if 0
Original Ghidra decompilation (0x5696f0):

void FUN_005696f0(void)

{
  int iVar1;
  undefined4 *in_EAX;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  *in_EAX = *(undefined4 *)(iVar1 + 0x23c);
  in_EAX[1] = *(undefined4 *)(iVar1 + 0x240);
  in_EAX[2] = *(undefined4 *)(iVar1 + 0x244);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
