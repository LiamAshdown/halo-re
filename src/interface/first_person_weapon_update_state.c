// first_person_weapon_update_state  (Ghidra: FUN_00492d20, unnamed)
// address 0x492d20, size 269 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (VERIFIED against objdump 0x492d20..0x492e2c (jump table 0x492e30/0x492e48 decoded: every state maps as in the C; set_state is AX state, stack (player, 0)))
// evidence: out/phase4/interface_functions.md "Given the local player's current first-person
// weapon animation state, decides whether/how to transition to a new state via FUN_00492e60."
// Disassembled directly (objdump bin/halo.exe 0x492d20..0x492e28) since Ghidra shows all three
// first_person_weapon_set_state calls with zero visible arguments; this pins new_state to a
// literal 0, 3, or a byte-flag-selected 0x10/0x11 depending which call site is reached, and
// force_pose_snapshot to a literal 0 at every site.
// register convention: local_player_index in DX (in_DX), unrecognized by Ghidra.
// // blam-cc: local_player_index=DX
// UNSURE: fp+0x1e90 (read as a byte) and fp+0x1e94 (read as a short) both fall inside
// first_person_weapon_interface::device_hud_element (types/interface.h, 0x1e10..0x1e98,
// "second match table"); kept as raw offsets from fp rather than forced into that array, since
// the header does not resolve individual elements of it. The weapon tag field at +0x4e2 is the
// same one first_person_weapon_set_state.c flags as an unmodelled likely weapon_type enum.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "cache.h"
#include "fn_interface.h"

extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14, types/cache.h
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98


// Looks at the local player's current first-person weapon animation state and either leaves it
// alone (states 3, 4, and the default case), decrements a countdown in place (state 0x12), or
// requests a transition via first_person_weapon_set_state: states 0xd/0xe and 0xf each gate on
// the weapon tag's +0x4e2 field and a device_hud_element entry before either resetting to state
// 0 or falling through to the shared 0x10/0x11 "ready" transition that every other listed state
// (0, 5-11, 0x10, 0x11, 0x13, 0x14, 0x16 directly; 1, 2, 0x15, 0x17 to state 3 instead) reaches.
void first_person_weapon_update_state(int16_t local_player_index)
{
    first_person_weapon_interface *fp;
    uint8_t *fpb;
    uint8_t *weapon_obj;
    uint8_t *item_tag_data;
    int16_t device_entry;
    uint8_t flag;

    fp = &first_person_weapon_interfaces[local_player_index];
    fpb = (uint8_t *)fp;

    switch (fp->state) {
    case 0: case 5: case 6: case 7: case 8: case 9: case 10: case 0xb: case 0xc:
    case 0x10: case 0x11: case 0x13: case 0x14: case 0x16:
        first_person_weapon_set_state(local_player_index, 0, 0);
        return;
    case 1: case 2: case 0x15: case 0x17:
        first_person_weapon_set_state(local_player_index, 0, 3);
        return;
    case 3: case 4:
        return;
    case 0xd: case 0xe:
        weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
        item_tag_data = *(uint8_t **)((uint8_t *)tag_instances +
                                       (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
        device_entry = *(int16_t *)(fpb + 0x1e94); // UNSURE, see header note
        if (*(int16_t *)(item_tag_data + 0x4e2) != 1 || device_entry == 0 || device_entry == -1) {
            first_person_weapon_set_state(local_player_index, 0, 0);
            return;
        }
        break;
    case 0xf:
        weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
        item_tag_data = *(uint8_t **)((uint8_t *)tag_instances +
                                       (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
        if (*(int16_t *)(item_tag_data + 0x4e2) != 1 || *(int16_t *)(fpb + 0x1e94) != 2) {
            first_person_weapon_set_state(local_player_index, 0, 0);
            return;
        }
        break;
    case 0x12:
        // UNSURE: fp+0x18 is types/interface.h's unnamed unknown_18[2]; first_person_weapon_
        // set_state.c also writes a fresh int16 0 there on every successful transition, so this
        // is read as the same int16 field, just decremented here instead of reset.
        *(int16_t *)(fpb + 0x18) = *(int16_t *)(fpb + 0x18) - 1;
        return;
    default:
        return;
    }

    flag = *(uint8_t *)(fpb + 0x1e90); // UNSURE, see header note
    first_person_weapon_set_state(local_player_index, 0, (flag != 0) ? 0x10 : 0x11);
}

#if 0
Original Ghidra decompilation (0x492d20):

void FUN_00492d20(void)

{
  int iVar1;
  short in_DX;

  iVar1 = in_DX * 0x1ea0 + DAT_006b2d98;
  switch(*(undefined2 *)(iVar1 + 0xc)) {
  case 0:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
  case 0xb:
  case 0xc:
  case 0x10:
  case 0x11:
  case 0x13:
  case 0x14:
  case 0x16:
switchD_00492d46_caseD_0:
    first_person_weapon_set_state();
    return;
  case 1:
  case 2:
  case 0x15:
  case 0x17:
    first_person_weapon_set_state();
    return;
  case 3:
  case 4:
    goto switchD_00492d46_caseD_3;
  case 0xd:
  case 0xe:
    if ((*(short *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                         (*(uint *)(iVar1 + 8) & 0xffff) * 0xc) & 0xffff) * 0x20 +
                             0x14 + DAT_0087bc14) + 0x4e2) != 1) ||
       ((*(short *)(iVar1 + 0x1e94) == 0 || (*(short *)(iVar1 + 0x1e94) == -1))))
    goto switchD_00492d46_caseD_0;
    break;
  case 0xf:
    if ((*(short *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                         (*(uint *)(iVar1 + 8) & 0xffff) * 0xc) & 0xffff) * 0x20 +
                             0x14 + DAT_0087bc14) + 0x4e2) != 1) ||
       (*(short *)(iVar1 + 0x1e94) != 2)) goto switchD_00492d46_caseD_0;
    break;
  case 0x12:
    *(short *)(iVar1 + 0x18) = *(short *)(iVar1 + 0x18) + -1;
    return;
  default:
    return;
  }
  if ((*(char *)(iVar1 + 0x1e90) != '\0') != true) {
    first_person_weapon_set_state();
  }
switchD_00492d46_caseD_3:
  return;
}
#endif
