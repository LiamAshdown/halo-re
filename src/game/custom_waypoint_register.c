// custom_waypoint_register  (Ghidra: FUN_00462260; renamed per its summary)
// address 0x462260, size 100 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Registers a new custom HUD waypoint marker at a world
// position (raised slightly above it) with an icon looked up by name and optional player/type
// filters"); types/game.h custom_waypoint (owner +0x18, icon +0x1c, active +0x0c, position
// +0x00, team +0x14, player +0x10).
// register convention: owner handle in EAX (in_EAX); slot index in CX (in_CX); the source
// position in EBX (unaff_EBX, a real_point3d*); height offset, player filter and team filter are
// this function's own stack parameters.
//   // blam-cc: EAX -> owner, CX -> slot, unaff_EBX -> position, stack -> height_offset,
//   //          player_filter, team_filter
// UNSURE: hud_waypoint_arrow_find's own inputs (presumably an icon name string) are entirely register-
// passed and not visible anywhere in this decompilation; called here with no arguments, matching
// Ghidra's own rendering.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern int16_t hud_waypoint_arrow_find(void); // 0x4af070, not in this batch; UNSURE: icon lookup by name

// blam-cc: EAX -> owner, CX -> slot, unaff_EBX -> position, stack -> height_offset,
//          player_filter, team_filter
void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position,
    float height_offset, datum_index player_filter, int16_t team_filter)
{
    custom_waypoint *w = &custom_waypoints[slot];

    w->owner = owner;
    w->icon = hud_waypoint_arrow_find();
    w->active = 1;
    w->position.x = position->x;
    w->position.y = position->y;
    w->position.z = position->z;
    w->team = team_filter;
    w->player = player_filter;
    w->position.z = height_offset + w->position.z + 0.63f;
}

#if 0
Original Ghidra decompilation (0x462260), from tools/pack.py 0x462260:

void FUN_00462260(float param_1,undefined4 param_2,undefined2 param_3)

{
  undefined2 uVar1;
  undefined4 in_EAX;
  short in_CX;
  undefined4 *unaff_EBX;
  int iVar2;

  iVar2 = (int)in_CX;
  (&DAT_006f18a0)[iVar2 * 8] = in_EAX;
  uVar1 = FUN_004af070();
  *(undefined2 *)(&DAT_006f18a4 + iVar2 * 8) = uVar1;
  *(undefined1 *)(&DAT_006f1894 + iVar2 * 8) = 1;
  (&DAT_006f1888)[iVar2 * 8] = *unaff_EBX;
  (&DAT_006f188c)[iVar2 * 8] = unaff_EBX[1];
  (&DAT_006f1890)[iVar2 * 8] = unaff_EBX[2];
  *(undefined2 *)(&DAT_006f189c + iVar2 * 8) = param_3;
  (&DAT_006f1898)[iVar2 * 8] = param_2;
  (&DAT_006f1890)[iVar2 * 8] = param_1 + (float)(&DAT_006f1890)[iVar2 * 8] + 0.63;
  return;
}
#endif
