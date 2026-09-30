// hud_waypoints_update_for_player  (Ghidra: FUN_004af370, renamed in the phase-4 review)
// address 0x4af370, size 462 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4af370..0x4af53d in the phase-4 review. For each of the
// four slots of the local player: an empty or freed slot gets its kind bits forced to 0xf;
// with a unit, the target position is resolved by kind (0 Scenario cutscene_flags +0x4e4,
// stride 0x5c, position +0x24; 1 object center of mass, a missing or hidden (+0x106 bit 2)
// object frees the slot; 2 custom_waypoints[i].position at 0x006f1888), raised by the slot
// vertical offset, and hud_waypoint_visibility (0x4af540) from the unit "head" marker (node
// transform position, object_marker +0x60) to it gives bits 4..7 of the kind word.
// The object kind passes its own object as the collision ignore object (EBP), the other kinds
// -1. Behaviour kept from the binary: a kind above 2 tests a stale position.
// register convention: plain cdecl, one stack argument (a short).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#include "fn_objects.h"

extern player_globals *local_player_globals;  // 0x0087a478
extern data_array *player_data;               // 0x0087a480
extern hud_waypoint_state *hud_waypoints;     // 0x006b3a44
extern Scenario *global_scenario; // 0x00746f8c
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
                                               uint32_t maximum_markers); // 0x4f6080
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index

extern int16_t hud_waypoint_visibility(int16_t local_player_index, const real_point3d *eye,
                                       const real_point3d *target, datum_index ignore_object); // 0x4af540, blam-cc: AX, ECX eye, EDX target

void hud_waypoints_update_for_player(int16_t local_player_index)
{
    static char head_marker[] = "head"; // 0x0066bfa0
    hud_waypoint *waypoint;
    datum_index unit_index;
    datum_index ignore_object;
    object_marker marker;
    real_point3d eye;
    real_point3d target;
    float radius;
    int32_t i;

    waypoint = hud_waypoints[local_player_index].waypoints;
    unit_index = (datum_index)-1;
    if (local_player_index != -1 && local_player_index < 1 &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = ((player *)((uint8_t *)player_data->data +
                                 (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
    }

    ignore_object = (datum_index)-1;
    for (i = 4; i != 0; i--, waypoint++) {
        uint16_t type_word;
        int16_t visibility;

        if (waypoint->arrow_index == -1 || waypoint->object_index == (datum_index)-1 ||
            (*(uint8_t *)&waypoint->type & 0xf) == 0xf) {
            *(uint8_t *)&waypoint->type |= 0xf;
            ignore_object = (datum_index)-1;
            continue;
        }
        if (unit_index == (datum_index)-1) {
            ignore_object = (datum_index)-1;
            continue;
        }
        object_get_node_local_transform(unit_index, head_marker, &marker, 1);
        eye = marker.node_transform.position; // object_marker +0x60

        type_word = (uint16_t)waypoint->type;
        switch ((int16_t)(type_word << 12) >> 12) {
        case 0: // scenario cutscene flag
            target = *(real_point3d *)&((ScenarioCutsceneFlag *)global_scenario->cutscene_flags.pointer +
                                        waypoint->object_index)->position;
            break;
        case 1: { // object
            object *target_object;

            ignore_object = waypoint->object_index;
            target_object = object_try_and_get(ignore_object, 0xffffffff);
            if (target_object == 0 || (*((uint8_t *)target_object + 0x106) & 4) != 0) {
                waypoint->type = (int16_t)(type_word | 0xf);
                waypoint->object_index = (datum_index)-1;
                waypoint->arrow_index = -1;
                ignore_object = (datum_index)-1;
                continue;
            }
            object_get_center_of_mass_and_scale(&target, ignore_object, &radius);
            break;
        }
        case 2: // custom waypoint
            target = custom_waypoints[(int16_t)waypoint->object_index].position;
            break;
        }
        target.z = target.z + waypoint->vertical_offset;
        visibility = hud_waypoint_visibility(local_player_index, &eye, &target, ignore_object);
        waypoint->type = (int16_t)(waypoint->type ^ (((visibility << 4) ^ *(uint8_t *)&waypoint->type) & 0xf0));
        ignore_object = (datum_index)-1;
    }
}

#if 0
Original Ghidra decompilation (0x4af370):

void FUN_004af370(short param_1)

{
  ushort uVar1;
  uint uVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  undefined4 uVar7;
  ushort *puVar8;
  int local_80;
  undefined1 local_70 [4];
  undefined1 local_6c [108];

  iVar5 = param_1 * 0x30 + DAT_006b3a44;
  if (((param_1 == -1) || (0 < param_1)) ||
     (uVar2 = *(uint *)(DAT_0087a478 + 4 + param_1 * 4), uVar2 == 0xffffffff)) {
    iVar6 = -1;
    puVar8 = (ushort *)(iVar5 + 2);
    local_80 = 4;
  }
  else {
    iVar6 = *(int *)((uVar2 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
    puVar8 = (ushort *)(iVar5 + 2);
    local_80 = 4;
  }
  do {
    uVar7 = 0xffffffff;
    if (((puVar8[-1] == 0xffff) || (*(int *)(puVar8 + 3) == -1)) || ((*puVar8 & 0xf) == 0xf)) {
      *(byte *)puVar8 = (byte)*puVar8 | 0xf;
    }
    else if (iVar6 != -1) {
      FUN_004f6080(iVar6,&DAT_0066bfa0,local_6c,1);
      uVar1 = *puVar8;
      sVar4 = (short)(uVar1 << 0xc) >> 0xc;
      if ((sVar4 != 0) && (sVar4 == 1)) {
        uVar7 = *(undefined4 *)(puVar8 + 3);
        iVar5 = object_try_and_get(0xffffffff);
        if ((iVar5 == 0) || ((*(byte *)(iVar5 + 0x106) & 4) != 0)) {
          *puVar8 = uVar1 | 0xf;
          puVar8[3] = 0xffff;
          puVar8[4] = 0xffff;
          puVar8[-1] = 0xffff;
          goto LAB_004af521;
        }
        FUN_004088e0(local_70);
      }
      cVar3 = FUN_004af540(uVar7);
      *puVar8 = *puVar8 ^ (byte)(cVar3 << 4 ^ (byte)*puVar8) & 0xf0;
    }
LAB_004af521:
    puVar8 = puVar8 + 6;
    local_80 = local_80 + -1;
    if (local_80 == 0) {
      return;
    }
  } while( true );
}
#endif
