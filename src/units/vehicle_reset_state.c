// vehicle_reset_state  (Ghidra: FUN_00570b00; chosen Blam-style name)
// address 0x570b00, size 171 bytes
// name confidence: 0.3 (out/phase4/units_types_notes.md identifies this as "the vehicle row's
//   +0x50 column (the reset/initialise hook)", correcting the pre-existing phase2 proposal
//   "unit_reset_control_state" which mistook it for a generic unit helper)
// rewrite confidence: 0.6
// evidence: types/units.h vehicle_data (every zeroed field from 0x4cc to 0x520 matches the
//   struct exactly, and the note "0x570b00 zeroes 0x4cc, 0x4ce, 0x4d0..0x4d3, then every dword
//   from 0x4d4 to 0x4f8 and from 0x508 to 0x520" describes this function verbatim).
// reconciled: R24 vehicle_data unknown_508..unknown_51c -> real_vector3d accumulated_force (+0x508) / accumulated_torque (+0x514) (still six zero dword stores)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

// Clears the live part of a vehicle's vehicle_data extension (0x4cc..0x520) to zero, e.g. on
// possession change or respawn.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
void vehicle_reset_state(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);

    vehicle->flags = 0;
    vehicle->decay_ticks_remaining = 0;
    vehicle->airborne_ticks = 0;
    vehicle->unknown_4d1 = 0;
    vehicle->unknown_4d2 = 0;
    vehicle->landing_ticks = 0;
    vehicle->forward_velocity = 0.0f;
    vehicle->sideways_velocity = 0.0f;
    vehicle->turning_velocity = 0.0f;
    vehicle->wheel_rotation = 0.0f;
    vehicle->left_wheel_rotation = 0.0f;
    vehicle->right_wheel_rotation = 0.0f;
    vehicle->ground_lean = 0.0f;
    vehicle->ground_contact_fraction = 0.0f;
    // only the first two dwords of contact_point_traction[20] (per the notes file)
    *(uint32_t *)&vehicle->contact_point_traction[0] = 0;
    *(uint32_t *)&vehicle->contact_point_traction[4] = 0;
    vehicle->accumulated_force.i = 0.0f;
    vehicle->accumulated_force.j = 0.0f;
    vehicle->accumulated_force.k = 0.0f;
    vehicle->accumulated_torque.i = 0.0f;
    vehicle->accumulated_torque.j = 0.0f;
    vehicle->accumulated_torque.k = 0.0f;
    vehicle->active_marker_mask = 0;
}

#if 0
Original Ghidra decompilation (0x570b00):

void FUN_00570b00(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  *(undefined2 *)(iVar1 + 0x4cc) = 0;
  *(undefined2 *)(iVar1 + 0x4ce) = 0;
  *(undefined1 *)(iVar1 + 0x4d0) = 0;
  *(undefined1 *)(iVar1 + 0x4d1) = 0;
  *(undefined1 *)(iVar1 + 0x4d2) = 0;
  *(undefined1 *)(iVar1 + 0x4d3) = 0;
  *(undefined4 *)(iVar1 + 0x4d4) = 0;
  *(undefined4 *)(iVar1 + 0x4d8) = 0;
  *(undefined4 *)(iVar1 + 0x4dc) = 0;
  *(undefined4 *)(iVar1 + 0x4e0) = 0;
  *(undefined4 *)(iVar1 + 0x4e4) = 0;
  *(undefined4 *)(iVar1 + 0x4e8) = 0;
  *(undefined4 *)(iVar1 + 0x4f0) = 0;
  *(undefined4 *)(iVar1 + 0x4ec) = 0;
  *(undefined4 *)(iVar1 + 0x4f4) = 0;
  *(undefined4 *)(iVar1 + 0x4f8) = 0;
  *(undefined4 *)(iVar1 + 0x508) = 0;
  *(undefined4 *)(iVar1 + 0x50c) = 0;
  *(undefined4 *)(iVar1 + 0x510) = 0;
  *(undefined4 *)(iVar1 + 0x514) = 0;
  *(undefined4 *)(iVar1 + 0x518) = 0;
  *(undefined4 *)(iVar1 + 0x51c) = 0;
  *(undefined4 *)(iVar1 + 0x520) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
