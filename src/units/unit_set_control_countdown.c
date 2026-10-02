// unit_set_control_countdown  (Ghidra: unit_set_control_countdown)
// address 0x563b20, size 42 bytes
// name confidence: 0.3 (renamed from phase2's "unit_set_desired_aim_velocity"; the destination
//   fields are unit_data.unknown_210/unknown_214, not any aiming field)   rewrite confidence: 0.5
// evidence: types/units.h unit_data.unknown_210 ("set by 0x563b20, counted down by unit_update"),
//   .unknown_214 ("set by 0x563b20; ORed into control_flags and tested for bit 0x800 by
//   unit_update").
// register convention: unit index in EAX, the two stored values on the stack.
//   // blam-cc: in_EAX -> unit_index, param_1 -> countdown, param_2 -> extra_control_flags

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

void unit_set_control_countdown(uint32_t unit_index, int32_t countdown, uint32_t extra_control_flags) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    unit->persistent_control_ticks = countdown;
    unit->persistent_control_flags = extra_control_flags;
}

#if 0
Original Ghidra decompilation (0x563b20):

void FUN_00563b20(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  *(undefined4 *)(iVar1 + 0x210) = param_1;
  *(undefined4 *)(iVar1 + 0x214) = param_2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
