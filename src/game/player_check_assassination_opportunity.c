// player_check_assassination_opportunity  (Ghidra: player_check_assassination_opportunity,
// already named)
// address 0x478770, size 172 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// FIXED 2026-09-28: unit_data begins at object +0x1f4 (k_unit_data_offset) and its field offsets are absolute; the draft cast the object pointer itself, so unit fields landed 0x1f4 bytes low (e.g. flags at object +0x10).
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x478770
//   --stop-address=0x478820): types/units.h unit_data::aiming_vector (0x23c);
//   types/objects.h object::bounding_center/bounding_radius (0x0a0/0x0ac);
//   src/math/ray_intersects_sphere_test.c's established (EAX->origin, ECX->center,
//   EDX->direction, stack->radius) convention; src/devices/device_frontfacing.c's established
//   (ESI->device_index, EDI->forward) convention; unit_get_camera_position (already named,
//   src/game/chimera__spectate_fp_camera_position.c); player_set_pending_interaction_action
//   (this batch, 0x478e00).
// register convention: none -- both are genuine stack parameters (Ghidra's own
//   param_1/param_2).
// UNSURE: device_can_change_position's exact effect (a final gate on the candidate handle, elided beyond its
//   one EAX argument and AL return).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80
extern uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin,
    real_vector3d *direction, real radius); // 0x4ce6c0
extern uint8_t device_frontfacing(uint32_t device_index, real_vector3d *forward); // 0x44c130
extern uint8_t device_can_change_position(uint32_t candidate_object); // 0x44c0c0, not in this batch


// Checks whether `player_index`'s unit is positioned and facing correctly to assassinate
// `candidate_object`: the candidate's bounding sphere must contain the player's camera position
// along the unit's own aiming direction, the candidate device/unit must be front-facing toward
// that aim, and device_can_change_position must approve the candidate. If all three pass, queues the
// assassination interaction (priority 10).
void player_check_assassination_opportunity(uint32_t player_index, uint32_t candidate_object)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    object *unit = (object *)((object_header *)object_data->data)[p->unit & 0xffff].data;
    object *candidate = (object *)((object_header *)object_data->data)[candidate_object & 0xffff].data;
    real_point3d camera_position;

    unit_get_camera_position(p->unit, &camera_position);

    if (ray_intersects_sphere_test(&candidate->bounding_center, &camera_position,
            &((unit_data *)((uint8_t *)unit + k_unit_data_offset))->aiming_vector, candidate->bounding_radius)) {
        if (device_frontfacing(candidate_object, &((unit_data *)((uint8_t *)unit + k_unit_data_offset))->aiming_vector)) {
            if (device_can_change_position(candidate_object)) {
                player_set_pending_interaction_action(10, (int16_t)0xffff, player_index, candidate_object);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x478770), from tools/pack.py 0x478770:

void player_check_assassination_opportunity(undefined4 param_1,uint param_2)

{
  int iVar1;
  char cVar2;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  unit_get_camera_position();
  cVar2 = ray_intersects_sphere_test(*(undefined4 *)(iVar1 + 0xac));
  if (cVar2 != '\0') {
    cVar2 = device_frontfacing();
    if (cVar2 != '\0') {
      cVar2 = FUN_0044c0c0();
      if (cVar2 != '\0') {
        player_set_pending_interaction_action(10,0xffffffff);
      }
    }
  }
  return;
}
#endif
