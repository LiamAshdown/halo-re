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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t camera_get_type_for_player(int16_t local_player_index); // 0x445ac0, blam-cc: CX -> local_player_index
extern player_globals *local_player_globals;     // 0x0087a478
extern data_array *player_data;                  // 0x0087a480
extern observer observers[1]; // 0x006ac65c, camera.h; observers[i].camera is the 0x006ac6d0 row (R17), an array, not a pointer

extern uint32_t unit_noop_569670(uint32_t object_index); // 0x569670, EAX object (its result is kept as the excluded object)
extern void *player_control_globals_ptr; // 0x006b145c, 0x40-byte records per local player
extern uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out); // 0x459e80,
    // blam-cc: EAX -> unit_index, EDX -> require_zoomed, EDI -> out
extern char camera_observer_find_best_target(real_point3d *observer_position,
    observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team,
    observer_target_candidate *out); // this batch, 0x459a00; observer_position travels in EBX

// Resolves the best observer target for the given local-player slot and returns its weight
// (via camera_observer_find_best_target's candidate), writing the target's object handle
// through `out_id`.
// REWRITTEN (first-boot track, objdump 0x459900..0x4599f7): camera_get_type_for_player gets CX = the slot; the
//   autoaim cone call gets EDX = the player control record's +0x34 (0x006b145c + 0x40 * slot; -1 without a slot);
//   the stack out value is the best candidate's primary weight (+0x30) and the result its object; the facing passed
//   is the observer camera + 0x20 even for slot -1 (then 0x20), as in the original.
// blam-cc: stack -> out_weight, SI -> local_player_slot
uint32_t camera_observer_get_target_id(datum_index *out_id, int16_t local_player_slot)
{
    int16_t camera_type = camera_get_type_for_player(local_player_slot);
    datum_index player_index;
    uint8_t *player_record;
    uint32_t exclude_object;
    int16_t zoom_requirement = -1;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint8_t *observer_camera;

    *out_id = 0;
    if (camera_type != 0 && camera_type != 1) {
        return 0xffffffff;
    }
    player_index = (local_player_slot != -1 && local_player_slot < 1)
        ? local_player_globals->local_players[local_player_slot] : k_datum_index_none;
    player_record = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    exclude_object = unit_noop_569670(*(uint32_t *)(player_record + 0x34));
    if (local_player_slot != -1) {
        zoom_requirement = *(int16_t *)(*(uint8_t **)&player_control_globals_ptr + local_player_slot * 0x40 + 0x34);
    }
    if (!unit_get_current_weapon_autoaim_cone(*(uint32_t *)(player_record + 0x34), zoom_requirement, cone_buffer)) {
        return 0xffffffff;
    }
    observer_camera = local_player_slot == -1 ? 0 : (uint8_t *)observers + local_player_slot * 0x29c + 0x74;
    if (!camera_observer_find_best_target((real_point3d *)observer_camera, (observer_target_cone *)cone_buffer,
                                          (real_vector3d *)(observer_camera + 0x20), exclude_object,
                                          *(int16_t *)(player_record + 0x20), &candidate)) {
        return 0xffffffff;
    }
    *out_id = *(datum_index *)&candidate.weight_primary;
    return candidate.object;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
