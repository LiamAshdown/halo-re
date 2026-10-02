// device_control_activate  (Ghidra: FUN_0044adf0; renamed per
// out/phase4/devices_types_notes.md's "Naming corrections worth carrying into phase 5" table)
// address 0x44adf0, size 62 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase2/results/devices_00.json (0x44adf0 entry: "only calls FUN_0044ae30 ...
//   when definition field +0x292 is zero"); types/tags.h DeviceControl.triggers_when (0x292,
//   DeviceTriggersWhen: touched_by_player = 0, destroyed = 1) -- devices_types_notes.md pins
//   this exact field: "FUN_0044adf0 gates on *(int16 *)(tag + 0x292) == 0, i.e.
//   DeviceControl.triggers_when == touched_by_player."
// register convention: object id in EAX.
//   // blam-cc: EAX = object_id
// UNSURE: the sole caller (unknown here; this function has no discovered callers in this
// batch's evidence) must also thread object_id through EBX untouched, since
// device_change_power_state (0x44ae30, this batch) reads its own object id from unaff_EBX
// rather than from any value this function passes explicitly -- Ghidra shows the call as a
// bare `device_change_power_state();`, consistent with EBX already holding the same id from
// a still-higher caller. This rewrite passes object_id explicitly, which reproduces that
// effect without needing the untouched-register assumption to hold inside this translation.
// UNSURE: device_change_power_state's ECX parameter is not set here either, so 0.0f is passed.
// The phase-4 review showed that parameter is NOT dead -- 0x44ae7a's `cmp eax,3; ja 0x44aebe`
// forwards it verbatim whenever DeviceControl.type falls outside 0..3 -- but this function's own
// 62 bytes touch neither ECX nor the FPU, so whatever the unseen caller left in ECX is what the
// original would have used. 0.0f is a stand-in for an unknowable inherited register, and the
// path that reads it only exists for malformed tag data.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void device_change_power_state(float fallback_value, uint32_t object_id); // 0x44ae30, this batch
    // blam-cc: ECX = fallback_value, EBX = object_id

// Guard for a device_control's activation: only forwards to device_change_power_state when the
// control's DeviceControl.triggers_when is touched_by_player. (triggers_when == destroyed is
// handled elsewhere, outside this module.)
void device_control_activate(uint32_t object_id) // blam-cc: EAX = object_id
{
    object *obj = ((object_header *)object_data->data)[object_id & 0xffff].data;
    DeviceControl *tag = (DeviceControl *)tag_instances[obj->definition_tag & 0xffff].data;

    if (tag->triggers_when == devicetriggerswhen_touched_by_player) {
        device_change_power_state(0.0f, object_id); // UNSURE: see header
    }
}

#if 0
Original Ghidra decompilation (0x44adf0):

void FUN_0044adf0(void)

{
  uint in_EAX;

  if (*(short *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc)
                          & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x292) == 0) {
    device_change_power_state();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
