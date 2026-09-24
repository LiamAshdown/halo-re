// hud_waypoints_draw_for_player  (Ghidra: FUN_004afb90, renamed in the phase-4 review)
// address 0x4afb90, size 397 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4afb90..0x4afd1c in the phase-4 review. Needs a local
// player with a unit and the HUDGlobals arrow bitmap (+0x15c). For each used slot the target
// position comes from the kind (0 Scenario cutscene flag, 1 object center of mass (a missing
// object skips the slot), anything else custom_waypoint_get_position 0x462230 with EAX out,
// CX index), raised by the vertical offset, and goes to hud_waypoint_draw (EAX position) with
// the arrow index, the visibility bits 4..7 of the kind word (sign-extended) and 1 for the
// distance readout. An empty slot gets its kind bits forced to 0xf. Always ends with
// game_engine_update_custom_waypoint_navpoints (0x462a90, BX local player index), also when
// the early checks fail.
// register convention: plain cdecl, one stack argument (a short).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"

extern player_globals *local_player_globals;  // 0x0087a478
extern data_array *player_data;               // 0x0087a480
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_waypoint_state *hud_waypoints;     // 0x006b3a44
extern Scenario *global_scenario; // 0x00746f8c

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void object_get_center_of_mass_and_scale(real_point3d *out_center, uint32_t object_index,
                                                float *out_radius); // 0x4088e0, blam-cc: EAX out_center, ECX object_index
extern void custom_waypoint_get_position(real_point3d *out, int16_t slot); // 0x462230, blam-cc: EAX out, CX slot; EAX still holds out afterwards
extern void hud_waypoint_draw(const real_point3d *position, int16_t local_player_index, int16_t arrow_index,
                              int16_t visibility, uint8_t show_distance); // 0x4af5e0, blam-cc: EAX position
extern void game_engine_update_custom_waypoint_navpoints(int16_t local_player_slot); // 0x462a90, blam-cc: BX local_player_slot

void hud_waypoints_draw_for_player(int16_t local_player_index)
{
    hud_waypoint *waypoints;
    datum_index player_index;
    int16_t i;

    if (local_player_index == -1 || local_player_index >= 1) {
        game_engine_update_custom_waypoint_navpoints(local_player_index);
        return;
    }
    player_index = local_player_globals->local_players[local_player_index];
    if (player_index == (datum_index)-1 ||
        ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->unit == (datum_index)-1 ||
        *(datum_index *)&hud_globals_tag_data->arrow_bitmap.tag_id == (datum_index)-1) {
        game_engine_update_custom_waypoint_navpoints(local_player_index);
        return;
    }

    waypoints = hud_waypoints[local_player_index].waypoints;
    for (i = 0; i < 4; i++) {
        hud_waypoint *waypoint = &waypoints[i];
        uint16_t type_word;
        real_point3d position;
        float radius;

        if (waypoint->arrow_index == -1 || waypoint->object_index == (datum_index)-1 ||
            (((uint32_t)(uint16_t)waypoint->type << 12) & 0xf000) == 0xf000) {
            *(uint8_t *)&waypoint->type |= 0xf;
            continue;
        }
        type_word = (uint16_t)waypoint->type;
        switch ((int16_t)(type_word << 12) >> 12) {
        case 0: // scenario cutscene flag
            position = *(real_point3d *)&((ScenarioCutsceneFlag *)global_scenario->cutscene_flags.pointer +
                                          waypoint->object_index)->position;
            break;
        case 1: // object
            if (object_try_and_get(waypoint->object_index, 0xffffffff) == 0) {
                continue;
            }
            object_get_center_of_mass_and_scale(&position, waypoint->object_index, &radius);
            break;
        default: // custom waypoint
            custom_waypoint_get_position(&position, (int16_t)waypoint->object_index);
            break;
        }
        position.z = position.z + waypoint->vertical_offset;
        hud_waypoint_draw(&position, local_player_index, waypoint->arrow_index,
                          (int16_t)((int16_t)(type_word << 8) >> 12), 1);
    }
    game_engine_update_custom_waypoint_navpoints(local_player_index);
}

#if 0
Original Ghidra decompilation (0x4afb90):

void FUN_004afb90(undefined4 param_1)

{
  int iVar1;
  short sVar2;
  short sVar3;
  uint uVar4;
  short sVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  undefined1 local_1c [4];
  undefined4 local_18;
  undefined4 local_14;
  float local_10;

  sVar9 = (short)param_1;
  if ((sVar9 != -1) && (sVar9 < 1)) {
    uVar4 = *(uint *)(DAT_0087a478 + 4 + sVar9 * 4);
    if ((uVar4 != 0xffffffff) &&
       ((*(int *)((uVar4 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != -1 &&
        (*(int *)(DAT_0071941c + 0x15c) != -1)))) {
      iVar8 = sVar9 * 0x30 + DAT_006b3a44;
      sVar9 = 0;
      do {
        sVar2 = *(short *)(iVar8 + sVar9 * 0xc);
        iVar1 = iVar8 + sVar9 * 0xc;
        if ((sVar2 == -1) || (*(int *)(iVar1 + 8) == -1)) {
LAB_004afd1d:
          *(byte *)(iVar1 + 2) = *(byte *)(iVar1 + 2) | 0xf;
        }
        else {
          sVar3 = *(short *)(iVar1 + 2);
          sVar5 = sVar3 << 0xc;
          if (sVar5 == -0x1000) goto LAB_004afd1d;
          sVar5 = sVar5 >> 0xc;
          if (sVar5 == 0) {
            puVar6 = (undefined4 *)
                     (*(int *)(DAT_00746f8c + 0x4e8) + 0x24 + *(int *)(iVar1 + 8) * 0x5c);
            local_18 = *puVar6;
            local_14 = puVar6[1];
            local_10 = (float)puVar6[2];
LAB_004afccd:
            local_10 = local_10 + *(float *)(iVar1 + 4);
            hud_waypoint_draw(param_1,sVar2,(short)(sVar3 << 8) >> 0xc,1);
          }
          else {
            if (sVar5 != 1) {
              puVar6 = (undefined4 *)FUN_00462230();
              local_18 = *puVar6;
              local_14 = puVar6[1];
              local_10 = (float)puVar6[2];
              goto LAB_004afccd;
            }
            iVar7 = object_try_and_get(0xffffffff);
            if (iVar7 != 0) {
              FUN_004088e0(local_1c);
              goto LAB_004afccd;
            }
          }
        }
        sVar9 = sVar9 + 1;
      } while (sVar9 < 4);
    }
  }
  FUN_00462a90();
  return;
}
#endif
