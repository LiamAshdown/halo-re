// camera_observer_get_target_id  (Ghidra: FUN_00459900; renamed per symbols/review_queue.txt)
// address 0x459900, size 248 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: types/game.h player_globals::local_players (0x0087a478+4); player::team (0x20).
//   symbols/review_queue.txt 0x459900 "same lookup pattern as 0x4596f0 ..., but only returns a
//   single id/value pair".
// register convention: local-player slot in SI (unaff_SI, -1 or >0 both disable the lookup, so
//   only slot 0 is ever valid -- consistent with k_maximum_local_players == 1); out id pointer
//   as the recognized stack parameter (param_1).
//   // blam-cc: unaff_SI -> local_player_slot, stack -> out_id
//
// UNSURE: this file shares every open question already documented in
// camera_observer_get_target_angles.c (0x4596f0, the closely related sibling this was compiled
// next to) -- the FUN_00569670 result used as an "exclude object", the camera-state table at
// 0x006ac6d0, and FUN_00459e80/FUN_00459a00's true argument sources. Not re-derived here.
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

extern int16_t camera_get_type_for_player(void); // UNSURE module/address
extern player_globals *local_player_globals;     // 0x0087a478
extern data_array *player_data;                  // 0x0087a480
extern observer observers[1]; // 0x006ac65c, camera.h; observers[i].camera is the 0x006ac6d0 row (R17), an array, not a pointer

extern uint32_t unit_noop_569670(void); // 0x569670, units module; returns nothing (matches src/units/unit_noop_569670.c) (see header)
extern uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out); // this batch, 0x459e80
extern char camera_observer_find_best_target(real_point3d *observer_position,
    observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team,
    observer_target_candidate *out); // this batch, 0x459a00; observer_position travels in EBX

// Resolves the best observer target for the given local-player slot and returns its weight
// (via camera_observer_find_best_target's candidate), writing the target's object handle
// through `out_id`.
uint32_t camera_observer_get_target_id(datum_index *out_id, int16_t local_player_slot)
    // blam-cc: stack -> out_id, unaff_SI -> local_player_slot
{
    int16_t camera_type;
    datum_index player_index;
    int16_t team;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint32_t exclude_object;

    camera_type = camera_get_type_for_player();
    *out_id = 0;
    if (camera_type != 0 && camera_type != 1) {
        return 0xffffffff;
    }

    if (local_player_slot == -1 || 0 < local_player_slot) {
        player_index = k_datum_index_none;
    } else {
        player_index = local_player_globals->local_players[local_player_slot];
    }

    // CORRECTED (phase 4 review, objdump 0x45995a..0x459962): EAX is loaded with the local
    // player's unit (player+0x34) before the call and 0x569670 is a true no-op, so this is
    // the unit handle, not a return value.
    exclude_object = ((player *)((uint8_t *)player_data->data +
                                 (player_index & 0xffff) * sizeof(player)))->unit;
    unit_noop_569670();
    if (unit_get_current_weapon_autoaim_cone(exclude_object, 0, cone_buffer) != 0) {
        uint8_t *row = (uint8_t *)&observers[local_player_slot].camera;
        real_vector3d *facing = (local_player_slot == -1) ? (real_vector3d *)0 :
            (real_vector3d *)(row + 0x20);
        team = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player)))->team; // UNSURE: see header
        if (camera_observer_find_best_target((real_point3d *)row, (observer_target_cone *)cone_buffer,
                                              facing, exclude_object, team, &candidate) != 0) {
            *out_id = candidate.object;
            return candidate.object; // matches Ghidra's `return local_38[0];` (dword 0 = object)
        }
        return 0xffffffff;
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x459900), from tools/pack.py 0x459900:

undefined4 FUN_00459900(undefined4 *param_1)

{
  int iVar1;
  char cVar2;
  short sVar3;
  uint uVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  short unaff_SI;
  undefined1 local_50 [24];
  undefined4 local_38 [12];
  undefined4 local_8;

  sVar3 = camera_get_type_for_player();
  *param_1 = 0;
  if ((sVar3 != 0) && (sVar3 != 1)) {
    return 0xffffffff;
  }
  if ((unaff_SI == -1) || (0 < unaff_SI)) {
    uVar4 = 0xffffffff;
  }
  else {
    uVar4 = *(uint *)(DAT_0087a478 + 4 + unaff_SI * 4);
  }
  iVar1 = *(int *)(DAT_0087a480 + 0x34);
  uVar5 = FUN_00569670();
  cVar2 = FUN_00459e80();
  if (cVar2 != '\0') {
    puVar6 = (undefined4 *)0x0;
    if (unaff_SI != -1) {
      puVar6 = &DAT_006ac6d0 + unaff_SI * 0xa7;
    }
    cVar2 = FUN_00459a00(local_50,puVar6 + 8,uVar5,
                         *(undefined2 *)((uVar4 & 0xffff) * 0x200 + iVar1 + 0x20),local_38);
    if (cVar2 != '\0') {
      *param_1 = local_8;
      return local_38[0];
    }
    return 0xffffffff;
  }
  return 0xffffffff;
}
#endif
