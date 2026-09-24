// projectile_force_detonate  (Ghidra: missed_4c0ac0, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the projectile object_type_definition row)
// address 0x4c0ac0, size 71 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: named already in out/phase4/projectiles_types_notes.md ("+0x44
//   projectile_force_detonate -- missed" and the flags table: "0x4c0ac0 forces 1.0" against both
//   0x240 and 0x248, "0x4c0ac0 clears 0x08" against the flags dword). types/projectiles.h
//   projectile_data.detonation_timer (0x240) / .arming_timer (0x248) / .flags
//   (_projectile_attached_bit 0x08, "stuck to something"). Callee
//   object_snap_to_parent_marker_and_detach (0x4f6610, src/objects, single stack
//   object_index parameter).
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family.
// blam-cc: stack -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610

// The projectile row's +0x44 hook. Forces both timers to their completed value (1.0), clears
// _projectile_attached_bit (it can no longer combine as an attached fragment once it is being
// forced to detonate), and detaches it from whatever parent marker it was snapped to. Always
// reports success.
uint8_t projectile_force_detonate(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    proj->arming_timer = 1.0f;
    proj->detonation_timer = 1.0f;
    proj->flags &= ~(uint32_t)_projectile_attached_bit;
    object_snap_to_parent_marker_and_detach(object_index);

    return 1;
}

#if 0
Original Ghidra decompilation (0x4c0ac0):

undefined4 missed_4c0ac0(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  *(undefined4 *)(iVar1 + 0x248) = 0x3f800000;
  *(undefined4 *)(iVar1 + 0x240) = 0x3f800000;
  *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) & 0xfffffff7;
  FUN_004f6610(param_1);
  return 1;
}
#endif
