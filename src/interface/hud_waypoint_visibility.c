// hud_waypoint_visibility  (Ghidra: FUN_004af540, renamed in the phase-4 review)
// address 0x4af540, size 151 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4af540..0x4af5d6 in the phase-4 review (the first rewrite
// had no register arguments). Casts a segment from the eye (ECX) to the target (EDX) with
// collision flags 0xc2ad, ignoring the unit of the local player; a hit that is not the target
// object itself (result type 3 object, result +0x38 object index) makes the waypoint occluded.
// register convention: AX local player index, ECX eye, EDX target; one stack argument; the
// result is in EAX (0 visible, 2 occluded), stored into bits 4..7 of hud_waypoint::type.
//   // blam-cc: local_player_index -> AX, eye -> ECX, target -> EDX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "projectiles.h"
#include "interface.h"

extern player_globals *local_player_globals;  // 0x0087a478
extern data_array *player_data;               // 0x0087a480

extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880

// blam-cc: local_player_index -> AX, eye -> ECX, target -> EDX
int16_t hud_waypoint_visibility(int16_t local_player_index, const real_point3d *eye, const real_point3d *target,
                                datum_index ignore_object)
{
    collision_result result;
    real_vector3d delta;
    datum_index unit_index = (datum_index)-1;

    if (local_player_index != -1 && local_player_index < 1 &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = ((player *)((uint8_t *)player_data->data +
                                 (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
    }
    delta.i = target->x - eye->x;
    delta.j = target->y - eye->y;
    delta.k = target->z - eye->z;
    if (collision_test_movement_segment(0xc2ad, (real_point3d *)eye, &delta, unit_index, &result) != 0) {
        if (result.type != 3 || result.object_index != ignore_object) {
            return 2;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4af540):

undefined4 FUN_004af540(int param_1)

{
  char cVar1;
  short local_50;
  int local_18;

  cVar1 = FUN_00505880(0xc2ad);
  if ((cVar1 != '\0') && ((local_50 != 3 || (local_18 != param_1)))) {
    return 2;
  }
  return 0;
}
#endif
