// custom_waypoint_get_position  (Ghidra: FUN_00462230; renamed per its summary)
// address 0x462230, size 33 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Fetches the stored 3D position of a single
// custom-waypoint slot into a caller-supplied Point3D-sized buffer"); types/game.h
// custom_waypoint::position (+0x00).
// register convention: output buffer in EAX (in_EAX); slot index in CX (in_CX).
//   // blam-cc: EAX -> out, CX -> slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

// blam-cc: EAX -> out, CX -> slot
void custom_waypoint_get_position(real_point3d *out, int16_t slot)
{
    out->x = custom_waypoints[slot].position.x;
    out->y = custom_waypoints[slot].position.y;
    out->z = custom_waypoints[slot].position.z;
}

#if 0
Original Ghidra decompilation (0x462230), from tools/pack.py 0x462230:

void FUN_00462230(void)

{
  undefined4 *in_EAX;
  short in_CX;
  int iVar1;

  iVar1 = (int)in_CX;
  *in_EAX = (&DAT_006f1888)[iVar1 * 8];
  in_EAX[1] = (&DAT_006f188c)[iVar1 * 8];
  in_EAX[2] = (&DAT_006f1890)[iVar1 * 8];
  return;
}
#endif
