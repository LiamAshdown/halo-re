// camera_observer_get_target_angles  (Ghidra: FUN_004596f0; renamed per symbols/review_queue.txt)
// address 0x4596f0, size 517 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: types/game.h observer_target_candidate (offset field layout, confirming
//   *param_1/*param_2 are the candidate's weight_primary/weight_secondary and the yaw/pitch
//   computation reads candidate.offset); symbols/review_queue.txt 0x4596f0.
// register convention: local-player slot in AX (in_AX); out_weight_primary/out_weight_secondary
//   and the yaw/pitch and rate-of-change output pairs are the recognized stack parameters.
//   // blam-cc: in_AX -> local_player_slot, stack -> out_weight_primary, out_weight_secondary,
//   //          out_yaw_pitch, out_yaw_pitch_rate
//
// UNSURE: shares every open question documented in camera_observer_get_target_id.c (0x459900,
// its closely related sibling): the FUN_00569670 "exclude object" result, the camera-state table
// at 0x006ac6d0, and FUN_00459e80/FUN_00459a00's true argument sources.
// UNSURE: FUN_004f6aa0 is not in this batch. It is called twice here with no visible arguments,
// against six floats (local_68/64/60/5c/58/54) that come from the same camera-state table row
// used for the facing vector, most likely a previous-frame direction pair used to derive an
// angular rate. Modelled with the minimum shape needed to preserve the arithmetic; the six
// floats are read from that table at a guessed offset (TYPES-GAP) rather than invented.
// reconciled: R17 0x006ac6d0 was declared as a pointer (uint8_t *camera_state_table) but the binary addresses the array (add reg,0x6ac6d0); now (uint8_t *)&observers[slot].camera from camera.h

// CORRECTED (phase 4 review): the camera-state row stride is 0x29c BYTES, not 0xa7. Ghidra
// prints "&DAT_006ac6d0 + slot * 0xa7" over a 4-byte element type, so 0xa7 is a DWORD count;
// the disassembly of both callers of 0x459a00 spells it out as
//   imul ebx,ebx,0x29c ; add ebx,0x6ac6d0
// and then passes ebx (row + 0x00, the observer position) to camera_observer_find_best_target in
// EBX and "lea eax,[ebx+0x20]" (row + 0x20, the facing) as its second stack argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "camera.h"
#include "fn_game.h"

extern int16_t camera_get_type_for_player(int16_t local_player_index); // 0x445ac0, CX
extern player_globals *local_player_globals;     // 0x0087a478
extern data_array *player_data;                  // 0x0087a480
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern observer observers[1]; // 0x006ac65c, camera.h; observers[i].camera is the 0x006ac6d0 row (R17)

extern uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out); // 0x459e80, EAX, EDX, EDI
extern char camera_observer_find_best_target(real_point3d *observer_position,
    observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team,
    observer_target_candidate *out); // 0x459a00; observer_position travels in EBX
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, EAX object, ESI velocity, EDI angular (NULL here)
extern double atan2(double y, double x); // x87 FPATAN
extern double sqrt(double x);            // x87 FSQRT

// REWRITTEN 2026-09-27 (static loop) from objdump 0x4596f0..0x4598f4 -- the aim-assist target for a local player
// (caller: game_engine_build_local_player_control_input). Draft defects fixed: camera_get_type_for_player takes the
// slot (CX); the autoaim cone query gets the player's desired_zoom_level (EDX, control record +0x24; -1 without a
// player) instead of 0; the yaw / pitch RATES come from the ROOT-OBJECT VELOCITIES of the target and the player's unit
// (0x4f6aa0), not from camera-row positions.
// blam-cc: AX -> local_player_slot, stack -> out_weight_primary, out_weight_secondary, out_yaw_pitch, out_yaw_pitch_rate
uint32_t camera_observer_get_target_angles(real *out_weight_primary, real *out_weight_secondary,
                                            real *out_yaw_pitch, real *out_yaw_pitch_rate,
                                            int16_t local_player_slot)
{
    int16_t camera_type;
    datum_index player_index;
    player *p;
    datum_index unit_index;
    int16_t zoom_level;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint8_t *row;
    real_vector3d player_velocity;
    real_vector3d target_velocity;
    real dx, dy, dz, h2, h;

    camera_type = camera_get_type_for_player(local_player_slot);
    *out_weight_primary = 0.0f;
    *out_weight_secondary = 0.0f;
    out_yaw_pitch[1] = 0.0f;
    out_yaw_pitch[0] = 0.0f;
    out_yaw_pitch_rate[1] = 0.0f;
    out_yaw_pitch_rate[0] = 0.0f;
    if (camera_type != 0 && camera_type != 1) {
        return 0xffffffff;
    }

    if (local_player_slot == -1 || 0 < local_player_slot) {
        player_index = k_datum_index_none;
    } else {
        player_index = local_player_globals->local_players[local_player_slot];
    }
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit_index = p->unit; // 0x459772..0x45977a: through the no-op 0x569670, which leaves EAX alone

    zoom_level = -1;
    if (local_player_slot != -1) {
        zoom_level = player_control_globals_ptr->local_players[local_player_slot].desired_zoom_level;
    }
    if (unit_get_current_weapon_autoaim_cone(unit_index, zoom_level, cone_buffer) == 0) {
        return 0xffffffff;
    }

    row = (local_player_slot == -1) ? (uint8_t *)0 : (uint8_t *)&observers[local_player_slot].camera;
    if (camera_observer_find_best_target((real_point3d *)row, (observer_target_cone *)cone_buffer,
            (real_vector3d *)(row + 0x20), unit_index, (int16_t)p->team, &candidate) == 0) {
        return 0xffffffff;
    }

    *out_weight_primary = candidate.weight_primary;
    *out_weight_secondary = candidate.weight_secondary;
    out_yaw_pitch[0] = (real)atan2((double)candidate.offset.j, (double)candidate.offset.i);
    h2 = candidate.offset.i * candidate.offset.i + candidate.offset.j * candidate.offset.j;
    out_yaw_pitch[1] = (real)atan2((double)candidate.offset.k, sqrt((double)h2));

    // 0x459848..0x4598d0
    object_get_root_object_velocities(unit_index, &player_velocity, (real_vector3d *)0);
    object_get_root_object_velocities(candidate.object, &target_velocity, (real_vector3d *)0);
    dx = target_velocity.i - player_velocity.i;
    dy = target_velocity.j - player_velocity.j;
    dz = target_velocity.k - player_velocity.k;
    h = (real)sqrt((double)h2);
    out_yaw_pitch_rate[0] = (dy * candidate.offset.i - dx * candidate.offset.j) / h2;
    out_yaw_pitch_rate[1] = (dz * h - candidate.offset.k * ((dx * candidate.offset.i + dy * candidate.offset.j) / h)) /
                            (candidate.offset.k * candidate.offset.k + h2);

    return candidate.object;
}

#if 0
Original Ghidra decompilation (0x4596f0), from tools/pack.py 0x4596f0:

undefined4 FUN_004596f0(undefined4 *param_1,undefined4 *param_2,float *param_3,float *param_4)

{
  int iVar1;
  char cVar2;
  short in_AX;
  short sVar3;
  uint uVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  undefined4 uVar7;
  float10 fVar8;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  undefined1 local_50 [24];
  undefined4 local_38 [4];
  float local_28;
  float local_24;
  float local_20;
  undefined4 local_8;
  undefined4 local_4;

  sVar3 = camera_get_type_for_player();
  *param_1 = 0;
  *param_2 = 0;
  param_3[1] = 0.0;
  *param_3 = 0.0;
  param_4[1] = 0.0;
  *param_4 = 0.0;
  if ((sVar3 != 0) && (sVar3 != 1)) {
    return 0xffffffff;
  }
  if ((in_AX == -1) || (0 < in_AX)) {
    uVar4 = 0xffffffff;
  }
  else {
    uVar4 = *(uint *)(DAT_0087a478 + 4 + in_AX * 4);
  }
  iVar1 = *(int *)(DAT_0087a480 + 0x34);
  uVar5 = FUN_00569670();
  cVar2 = FUN_00459e80();
  uVar7 = 0xffffffff;
  if (cVar2 != '\0') {
    puVar6 = (undefined4 *)0x0;
    if (in_AX != -1) {
      puVar6 = &DAT_006ac6d0 + in_AX * 0xa7;
    }
    cVar2 = FUN_00459a00(local_50,puVar6 + 8,uVar5,
                         *(undefined2 *)((uVar4 & 0xffff) * 0x200 + iVar1 + 0x20),local_38);
    if (cVar2 == '\0') {
      return 0xffffffff;
    }
    fVar8 = (float10)fpatan((float10)local_24,(float10)local_28);
    *param_1 = local_8;
    *param_2 = local_4;
    *param_3 = (float)fVar8;
    fVar8 = (float10)fpatan((float10)local_20,
                            SQRT((float10)local_24 * (float10)local_24 +
                                 (float10)local_28 * (float10)local_28));
    param_3[1] = (float)fVar8;
    FUN_004f6aa0();
    fVar8 = (float10)FUN_004f6aa0();
    *param_4 = (float)((((float10)local_64 - (float10)local_58) * (float10)local_28 -
                       ((float10)local_68 - (float10)local_5c) * (float10)local_24) / fVar8);
    param_4[1] = (float)((((float10)local_60 - (float10)local_54) * (float10)(float)SQRT(fVar8) -
                         ((((float10)local_64 - (float10)local_58) * (float10)local_24 +
                          ((float10)local_68 - (float10)local_5c) * (float10)local_28) /
                         (float10)(float)SQRT(fVar8)) * (float10)local_20) /
                        ((float10)local_20 * (float10)local_20 + fVar8));
    uVar7 = local_38[0];
  }
  return uVar7;
}
#endif
