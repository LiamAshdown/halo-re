// unit_sample_camera_shake_from_velocity  (Ghidra: FUN_0056bfc0)
// address 0x56bfc0, size 168 bytes, name confidence 0.1 (really a teleport to the aim point), rewrite confidence 0.95
// functions.md: "Samples a value derived from the unit's scaled velocity and, above a 0.95
// threshold, bumps an accumulator and triggers FUN_004f5350 (likely a camera or effect
// trigger)."
// evidence: this rewrite uses unit_get_aiming_vector's established field (unit_data.aiming_vector
// at 0x23c) since the offsets here (0x23c/0x240/0x244) match exactly.
// blam-cc: unaff_EBX -> unit_index.
// UNSURE: collision_test_movement_segment's output block (local_5c, 44 bytes) and its 4th written float
// (local_24, tested against 0.95) are not named; the accumulator (local_30) and the final
// object_set_position_and_relink call are reproduced with raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "physics.h"
#include "projectiles.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, ECX, EDI
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack (location may be 0)

// REWRITTEN from objdump 0x56bfc0..0x56c067: casts 25 along the aim from the camera (mask 0x22, ignoring the unit);
// on a floor (normal.k > 0.95) the unit is moved 0.25 above the hit point. A debug "teleport to aim" reached from
// game_engine_update_local_player_control; the draft relinked a NULL position.
void unit_sample_camera_shake_from_velocity(uint32_t unit_index) // blam-cc: EBX -> unit_index
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    real_point3d camera_position;
    real_vector3d delta;
    collision_result hit;

    unit_get_camera_position(unit_index, &camera_position);
    delta.i = ((unit_object *)obj)->unit.aiming_vector.i * 25.0f;
    delta.j = ((unit_object *)obj)->unit.aiming_vector.j * 25.0f;
    delta.k = ((unit_object *)obj)->unit.aiming_vector.k * 25.0f;
    if (collision_test_movement_segment(0x22, &camera_position, &delta, unit_index, &hit) &&
        hit.plane.normal.k > 0.95f) {
        real_point3d position = hit.point;

        position.z += 0.25f;
        object_set_position_and_relink(&position, unit_index, 0);
    }
}

#if 0
Original Ghidra decompilation (0x56bfc0):

void FUN_0056bfc0(void)

{
  int iVar1;
  char cVar2;
  uint unaff_EBX;
  float local_68;
  float local_64;
  float local_60;
  undefined1 local_5c [44];
  float local_30;
  float local_24;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  unit_get_camera_position();
  local_68 = *(float *)(iVar1 + 0x23c) * 25.0;
  local_64 = *(float *)(iVar1 + 0x240) * 25.0;
  local_60 = *(float *)(iVar1 + 0x244) * 25.0;
  cVar2 = FUN_00505880(0x22,local_5c,&local_68);
  if ((cVar2 != '\0') && (0.95 < local_24)) {
    local_30 = local_30 + 0.25;
    object_set_position_and_relink(0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
