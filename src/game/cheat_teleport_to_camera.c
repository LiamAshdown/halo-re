// cheat_teleport_to_camera  (Ghidra: cheat_teleport_to_camera, already named)
// address 0x45a630, size 136 bytes
// name confidence: 0.9   rewrite confidence: 0.55
// evidence: types/game.h player::unit (0x34); types/objects.h object::parent_object (0x11c);
//   the "camera is outside BSP" string. `&DAT_006ac6e0 + slot*0xa7` is the same per-local-player
//   camera-state table cheat_make_player_invincible's siblings in the observer cluster reference
//   at 0x006ac6d0 (this batch's own UNSURE notes there); DAT_006ac6e0 = table + 0x10, a
//   short field this file's own evidence does not otherwise name.
// register convention: no arguments.
//
// UNSURE: relies on cheat_get_target_object_index (0x45a7a0), which always returns "not found"
// per this batch's own analysis of that function -- so this cheat is dead code as shipped.
// object_set_position_and_orientation's exact signature is not in this batch; called here with
// the two literal 0 arguments Ghidra shows plus the object index, matching its apparent role
// (teleport target, zero rotation).
// reconciled: R17 0x006ac6d0 was declared as a pointer (uint8_t *camera_state_table) but the binary addresses the array (add reg,0x6ac6d0); now (uint8_t *)&observers[slot].camera from camera.h

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

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern observer observers[1]; // 0x006ac65c, camera.h; observers[i].camera is the 0x006ac6d0 row (R17), an array, not a pointer
                                    // player (Ghidra prints 0xa7 over a 4-byte element type)

extern uint32_t cheat_get_target_object_index(void); // this batch, 0x45a7a0
extern void console_printf_verbose(const char *format, ...); // 0x496a80, UNSURE identity (on-screen
    // error/log). It also takes a register argument in EAX (DAT_00685220 at this call site)
    // that cannot be expressed through this prototype; see UNSURE below.
extern void object_set_position_and_orientation(datum_index object_index, real_vector3d *forward,
    real_vector3d *up, real_point3d *position); // 0x4f51c0, blam-cc: stack -> object_index,
    // forward, up; EDI -> position (matches src/objects/object_set_position_and_orientation.c)

// Teleports the debug-selected player's unit (or its root parent) to the current camera
// position, or logs an error if the camera is outside the BSP.
void cheat_teleport_to_camera(void)
{
    uint32_t player_index;
    int16_t local_player_slot;
    datum_index unit_index;
    datum_index root;
    object *unit_obj;
    uint8_t *camera_row;

    player_index = cheat_get_target_object_index();
    if (player_index != 0xffffffff) {
        local_player_slot = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player)))->local_player_index;
        if (local_player_slot != -1) {
            camera_row = (uint8_t *)&observers[local_player_slot].camera;
            if (*(int16_t *)(camera_row + 0x10) != -1) { // UNSURE offset
                unit_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player)))->unit;
                unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
                root = unit_obj->parent_object;
                if (root == k_datum_index_none) {
                    root = unit_index;
                }
                // CORRECTED (phase 4 review, objdump 0x45a69a): the call pushes only
                // (object_index, 0, 0); EDI still holds the camera-state row base, which is
                // the position argument the owning module names. The camera position is the
                // first field of that row.
                object_set_position_and_orientation(root, 0, 0, (real_point3d *)camera_row);
                return;
            }
            console_printf_verbose("Camera is outside BSP... cannot initiate teleportation...");
        }
    }
}

#if 0
Original Ghidra decompilation (0x45a630), from tools/pack.py 0x45a630:

void __cdecl cheat_teleport_to_camera(void)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;

  uVar2 = FUN_0045a7a0();
  if (uVar2 != 0xffffffff) {
    iVar3 = (uVar2 & 0xffff) * 0x200;
    sVar1 = *(short *)(iVar3 + 2 + *(int *)(DAT_0087a480 + 0x34));
    if (sVar1 != -1) {
      if (*(short *)(&DAT_006ac6e0 + sVar1 * 0xa7) != -1) {
        uVar2 = *(uint *)(iVar3 + *(int *)(DAT_0087a480 + 0x34) + 0x34);
        uVar4 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) +
                         0x11c);
        if (uVar4 == 0xffffffff) {
          uVar4 = uVar2;
        }
        object_set_position_and_orientation(uVar4,0,0);
        return;
      }
      FUN_00496a80("Camera is outside BSP... cannot initiate teleportation...");
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
