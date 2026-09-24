// camera_observer_get_target_angles  (Ghidra: FUN_004596f0; renamed per symbols/review_queue.txt)
// address 0x4596f0, size 517 bytes
// name confidence: 0.3   rewrite confidence: 0.2
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

extern int16_t camera_get_type_for_player(void); // UNSURE module/address
extern player_globals *local_player_globals;     // 0x0087a478
extern data_array *player_data;                  // 0x0087a480
extern uint8_t *camera_state_table;              // 0x006ac6d0, stride 0x29c bytes; TYPES-GAP

extern uint32_t unit_noop_569670(void); // 0x569670, units module; returns nothing (matches src/units/unit_noop_569670.c)
extern uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out); // this batch, 0x459e80
extern char camera_observer_find_best_target(real_point3d *observer_position,
    observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team,
    observer_target_candidate *out); // this batch, 0x459a00; observer_position travels in EBX
extern double atan2(double y, double x); // x87 FPATAN
extern double sqrt(double x);            // x87 FSQRT
extern real FUN_004f6aa0(real_vector3d *previous_offset, real_vector3d *offset); // UNSURE signature (TYPES-GAP)

// Resolves the best observer target for `local_player_slot`, returning its two weights and the
// yaw/pitch (and their rates of change) from the observer toward it.
uint32_t camera_observer_get_target_angles(real *out_weight_primary, real *out_weight_secondary,
                                            real *out_yaw_pitch, real *out_yaw_pitch_rate,
                                            int16_t local_player_slot)
    // blam-cc: stack -> out_weight_primary, out_weight_secondary, out_yaw_pitch,
    //          out_yaw_pitch_rate, in_AX -> local_player_slot
{
    int16_t camera_type;
    datum_index player_index;
    int16_t team;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint32_t exclude_object;
    real_vector3d *facing;
    real horizontal;
    uint8_t *row;   // camera_state_table row for this local player, 0x29c bytes
    real *previous; // UNSURE: TYPES-GAP camera-state row, six floats

    camera_type = camera_get_type_for_player();
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

    // CORRECTED (phase 4 review, objdump 0x459772..0x45977a): Ghidra prints this as
    // `exclude_object = FUN_00569670()`, but EAX is loaded with the local player's unit
    // (player+0x34) immediately BEFORE the call, and 0x569670 is a true no-op that leaves
    // EAX alone, so the value is the unit handle -- not a return value.
    exclude_object = ((player *)((uint8_t *)player_data->data +
                                 (player_index & 0xffff) * sizeof(player)))->unit;
    unit_noop_569670();
    if (unit_get_current_weapon_autoaim_cone(exclude_object, 0, cone_buffer) == 0) {
        return 0xffffffff;
    }

    row = camera_state_table + local_player_slot * 0x29c;
    facing = (local_player_slot == -1) ? (real_vector3d *)0 : (real_vector3d *)(row + 0x20);
    previous = (real *)row; // dwords 0..5: the previous and current camera positions (TYPES-GAP)
    team = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player)))->team; // UNSURE

    if (camera_observer_find_best_target((real_point3d *)row, (observer_target_cone *)cone_buffer,
                                          facing, exclude_object, team, &candidate) == 0) {
        return 0xffffffff;
    }

    *out_weight_primary = candidate.weight_primary;
    *out_weight_secondary = candidate.weight_secondary;
    out_yaw_pitch[0] = (real)atan2((double)candidate.offset.j, (double)candidate.offset.i);
    horizontal = (real)sqrt((double)(candidate.offset.j * candidate.offset.j + candidate.offset.i * candidate.offset.i));
    out_yaw_pitch[1] = (real)atan2((double)candidate.offset.k, (double)horizontal);

    // UNSURE: rate-of-change formula transcribed from the decompile, operand sourcing per header.
    {
        real denom = FUN_004f6aa0((real_vector3d *)previous, &candidate.offset);
        real denom_sqrt = (real)sqrt((double)denom);
        out_yaw_pitch_rate[0] = ((previous[3] - previous[0]) * candidate.offset.i -
                                  (previous[4] - previous[1]) * candidate.offset.j) / denom;
        out_yaw_pitch_rate[1] = ((previous[2] - previous[5]) * denom_sqrt -
                                  (((previous[3] - previous[0]) * candidate.offset.j +
                                    (previous[4] - previous[1]) * candidate.offset.i) / denom_sqrt) * horizontal) /
                                 (horizontal * horizontal + denom);
    }

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
