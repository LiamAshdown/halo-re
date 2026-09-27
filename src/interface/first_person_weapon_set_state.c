// first_person_weapon_set_state  (Ghidra: already named)
// address 0x492e60, size 524 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED against objdump 0x492e60..0x49306b (both jump tables decoded: 6..9 need state in {0,4,5,6,0xd,0xe,0xf,0x10,0x11,0x16}, 0xb/0xc need state 0 or 5, 0x13 is a no-op when already 0x13; blend 0 for 3/10/0x13, 3 for 6..9, else 6); FIXED the blend-0 test: CX = weapon type (+0x4e2) == 1, not the stage)
// evidence: out/phase4/interface_functions.md "Weapon animation state machine: validates and
// applies a state transition for the local player's first-person weapon, recording the previous
// pose for blending when needed." types/items.h weapon_data.flags bit 0x01
// (_weapon_overheated_bit) at object+0x22c explains the two overheat-state substitutions
// (0x13->2, 0x14->0x15) and the `puVar2[0x8b] & 1` recheck (0x8b*4 == 0x22c).
// register convention: local_player_index and force_pose_snapshot are the two
// Ghidra-recognized parameters (param_1, param_2); new_state arrives in AX (in_AX), unrecognized
// new_state=AX
// UNSURE / TYPES-GAP: this function's tag-block navigation (weapon tag +0x478 -> hud_interface
// tag +0x48/+0x4c/+0x10/+0x14, the same chain as hud_play_pickup_notification.c) and the weapon
// tag field at +0x4e2 (compared against 3 and 1, likely a weapon_type-shaped enum not yet
// modelled in types/items.h) are preserved as raw byte-offset pointer arithmetic. Ghidra's
// `extraout_CX` (checked against 1 right after the item_type_to_animation_stage call) and
// `extraout_EDX` (dereferenced as the weapon's object pointer) are both stale-register artifacts
// of the decompiler, not real extra parameters: extraout_CX is read as though it were a second
// return value of that call but nothing in item_type_to_animation_stage's own body writes CX, so
// it is treated here as the same in_AX -> item_type_to_animation_stage input value surviving in
// CX; extraout_EDX is treated as the same object pointer computed earlier (puVar2), since
// nothing between its computation and this use touches EDX. Neither is proven by disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "cache.h"

extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14, types/cache.h
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern int16_t item_type_to_animation_stage(int16_t message_stage); // 0x492880, this module
extern void first_person_weapon_snapshot_pose(int16_t local_player_index, int16_t blend_gap); // 0x4930b0, this module
extern void sound_impulse_fade_out(datum_index sound_index); // 0x549ee0, ECX

// Validates and applies a first-person weapon animation state transition. new_state (AX) is
// first remapped 0x13->2 / 0x14->0x15 while the current weapon is overheated
// (weapon_data.flags & _weapon_overheated_bit, object+0x22c); a battery of per-state gates then
// either rejects the transition outright or falls through to look up an animation index via
// item_type_to_animation_stage and the weapon's hud_interface tag message table, applying the
// new state (and snapshotting the previous pose first, when force_pose_snapshot is set and no
// blend is already pending) only if that lookup succeeds.
// blam-cc: AX -> new_state, stack -> local_player_index, force_pose_snapshot
void first_person_weapon_set_state(int16_t local_player_index, uint8_t force_pose_snapshot, int16_t new_state)
{
    first_person_weapon_interface *fp;
    uint8_t *weapon_obj;
    int16_t current_state;
    uint8_t reject;
    int16_t animation_stage;
    int16_t blend_gap;
    uint8_t *item_tag_data;
    uint8_t *hud_tag_data;
    uint8_t *block_a_base;
    int32_t block_a_count;

    fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)0xffffffff) {
        weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
        if ((*(uint8_t *)(weapon_obj + 0x22c) & 1) != 0) { // weapon_data.flags & _weapon_overheated_bit
            if (new_state == 0x13) {
                new_state = 2;
            } else if (new_state == 0x14) {
                new_state = 0x15;
            }
        }
    }

    reject = 0;
    switch (new_state) {
    case 6: case 7: case 8: case 9:
        current_state = fp->state;
        if (current_state != 0 && current_state != 5 && current_state != 6 &&
            current_state != 4 && current_state != 0xf && current_state != 0x16 &&
            current_state != 0x10 && current_state != 0x11 && current_state != 0xd &&
            current_state != 0xe) {
            reject = 1;
        }
        break;
    case 0xb: case 0xc:
        if (fp->state != 0 && fp->state != 5) {
            reject = 1;
        }
        break;
    case 0x13:
        if (fp->state == 0x13) {
            return;
        }
        break;
    default:
        break;
    }
    if (reject) {
        return;
    }

    if (new_state == -1) {
        return;
    }
    if (fp->weapon_index == (datum_index)0xffffffff) {
        return;
    }

    weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
    item_tag_data = *(uint8_t **)((uint8_t *)tag_instances +
                                   (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
    if (*(int16_t *)(item_tag_data + 0x4e2) == 3 && new_state == 3 &&
        (*(uint32_t *)(weapon_obj + 0x22c) & 1) == 0) {
        new_state = 0;
    }

    animation_stage = item_type_to_animation_stage(new_state);

    // 0x492f8b: CX is still the weapon type (tag +0x4e2, loaded at 0x492f66; item_type_to_animation_stage never
    // touches ECX), not the returned stage.
    if (*(int16_t *)(item_tag_data + 0x4e2) == 1 && fp->state == 0x10) {
        blend_gap = 0;
    } else {
        switch (new_state) {
        case 3: case 10: case 0x13:
            blend_gap = 0;
            break;
        case 6: case 7: case 8: case 9:
            blend_gap = 3;
            break;
        default:
            blend_gap = 6;
            break;
        }
    }

    if (fp->unit_index == (datum_index)0xffffffff) {
        return;
    }

    // UNSURE: extraout_EDX, treated as weapon_obj (see header note).
    hud_tag_data = *(uint8_t **)((uint8_t *)tag_instances +
                                  ((*(uint32_t *)(item_tag_data + 0x478)) & 0xffff) * 0x20 + 0x14);
    block_a_count = *(int32_t *)(hud_tag_data + 0x48);
    if (block_a_count == 0) {
        return;
    }
    block_a_base = *(uint8_t **)(hud_tag_data + 0x4c);
    if (block_a_base == 0) {
        return;
    }
    if (animation_stage < 0 || animation_stage >= *(int32_t *)(block_a_base + 0x10)) {
        return;
    }
    animation_stage = *(int16_t *)(*(uint8_t **)(block_a_base + 0x14) + animation_stage * 2);
    if (animation_stage == -1) {
        return;
    }

    if (force_pose_snapshot != 0 && fp->unknown_1e98 != -1 && fp->unknown_1e9c != 1) {
        sound_impulse_fade_out((datum_index)fp->unknown_1e98); // FIXED: ECX = fp +0x1e98 (0x49301f)
        fp->unknown_1e98 = -1;
        fp->unknown_1e9c = -1;
    }
    if (blend_gap != 0) {
        first_person_weapon_snapshot_pose(local_player_index, blend_gap);
    }
    fp->state = new_state;
    fp->unknown_16 = animation_stage;
    *(int16_t *)fp->unknown_18 = 0;
}

#if 0
Original Ghidra decompilation (0x492e60):

void first_person_weapon_set_state(short param_1,char param_2)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  short in_AX;
  short sVar4;
  short extraout_CX;
  uint *extraout_EDX;
  short sVar5;
  int iVar6;
  bool bVar7;

  iVar3 = DAT_0087bc14;
  iVar6 = param_1 * 0x1ea0 + DAT_006b2d98;
  uVar1 = *(uint *)(iVar6 + 8);
  if ((uVar1 != 0xffffffff) &&
     ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 0x22c) & 1)
      != 0)) {
    if (in_AX == 0x13) {
      in_AX = 2;
    }
    else if (in_AX == 0x14) {
      in_AX = 0x15;
    }
  }
  switch(in_AX) {
  case 6:
  case 7:
  case 8:
  case 9:
    sVar4 = *(short *)(iVar6 + 0xc);
    if (((((sVar4 != 0) && (sVar4 != 5)) && (sVar4 != 6)) &&
        (((sVar4 != 4 && (sVar4 != 0xf)) &&
         ((sVar4 != 0x16 && ((sVar4 != 0x10 && (sVar4 != 0x11)))))))) && (sVar4 != 0xd)) {
      bVar7 = sVar4 == 0xe;
LAB_00492f2a:
      if (!bVar7) {
        return;
      }
    }
    break;
  case 0xb:
  case 0xc:
    if (*(short *)(iVar6 + 0xc) != 0) {
      bVar7 = *(short *)(iVar6 + 0xc) == 5;
      goto LAB_00492f2a;
    }
    break;
  case 0x13:
    if (*(short *)(iVar6 + 0xc) == 0x13) {
      return;
    }
  }
  if (in_AX == -1) {
    return;
  }
  if (uVar1 == 0xffffffff) {
    return;
  }
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
  if (((*(short *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x4e2) == 3) &&
      (in_AX == 3)) && ((puVar2[0x8b] & 1) == 0)) {
    in_AX = 0;
  }
  sVar4 = FUN_00492880();
  if ((extraout_CX == 1) && (*(short *)(iVar6 + 0xc) == 0x10)) {
switchD_00492fae_caseD_3:
    sVar5 = 0;
  }
  else {
    switch(in_AX) {
    case 3:
    case 10:
    case 0x13:
      goto switchD_00492fae_caseD_3;
    default:
      sVar5 = 6;
      break;
    case 6:
    case 7:
    case 8:
    case 9:
      sVar5 = 3;
    }
  }
  if (((*(int *)(iVar6 + 4) != -1) &&
      (iVar3 = *(int *)((*(uint *)(*(int *)((*extraout_EDX & 0xffff) * 0x20 + 0x14 + iVar3) + 0x478)
                        & 0xffff) * 0x20 + 0x14 + iVar3), *(int *)(iVar3 + 0x48) != 0)) &&
     ((iVar3 = *(int *)(iVar3 + 0x4c), iVar3 != 0 &&
      (((-1 < sVar4 && ((int)sVar4 < *(int *)(iVar3 + 0x10))) &&
       (sVar4 = *(short *)(*(int *)(iVar3 + 0x14) + sVar4 * 2), sVar4 != -1)))))) {
    if (((param_2 != '\0') && (*(int *)(iVar6 + 0x1e98) != -1)) && (*(short *)(iVar6 + 0x1e9c) != 1)
       ) {
      FUN_00549ee0();
      *(undefined4 *)(iVar6 + 0x1e98) = 0xffffffff;
      *(undefined2 *)(iVar6 + 0x1e9c) = 0xffff;
    }
    if (sVar5 != 0) {
      FUN_004930b0();
    }
    *(short *)(iVar6 + 0xc) = in_AX;
    *(short *)(iVar6 + 0x16) = sVar4;
    *(undefined2 *)(iVar6 + 0x18) = 0;
  }
  return;
}
#endif
