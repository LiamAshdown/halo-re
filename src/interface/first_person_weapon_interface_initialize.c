// first_person_weapon_interface_initialize  (Ghidra: FUN_00493c60, renamed per types/interface.h)
// address 0x493c60, size 487 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: types/interface.h first_person_weapon_interface struct comment names this address
// first_person_weapon_interface_initialize; first_person_weapon_set_attached.c's disassembly-
// confirmed effect_reattach_markers_for_object/effect_release_first_person_markers/
// particles_delete_by_first_person_weapon signatures (the same detach-then-reattach pair this
// function calls around a unit_index==-1 style check); types/units.h unit::current_weapon_index
// (+0x2f2) / unit::weapons[4] (+0x2f8); types/tags.h GlobalsFirstPersonInterface (the
// first_person_hands.tag_id read at globals->first_person_interface[0]+0xc).
// register convention: player index as the recognized stack parameter (param_1).
// blam-cc: stack -> local_player_index
// TYPES-GAP / UNSURE: the weapon-tag navigation (weapon tag +0x468/+0x478 -> hud_interface tag
// +0x48/+0x4c/+0x10/+0x14 -> model node array +0x78) is the exact same chain
// first_person_weapon_set_state.c already flags as raw, unmodelled offsets; reproduced the same
// way here rather than guessed at. first_person_weapon_set_state's third parameter (new_state,
// in AX, not visible at this call site) is supplied as 0 -- a guess, not an attested value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;     // 0x00746fa0

extern void effect_reattach_markers_for_object(int16_t local_player_index, datum_index weapon_index); // 0x450cb0
extern void effect_release_first_person_markers(int16_t local_player_index); // 0x450d50
extern void particles_delete_by_first_person_weapon(uint8_t local_player_index); // 0x455c80
extern void first_person_weapon_set_state(int16_t local_player_index, uint8_t force_pose_snapshot,
                                           int16_t new_state); // 0x492e60
extern uint8_t hud_meter_find_matching_elements(uint32_t source_tag_ref, uint32_t target_tag_ref,
                                                 int16_t *out); // 0x493f00, this module
extern void first_person_weapon_interface_tick_reset(int16_t local_player_index); // 0x4942e0, this module -- UNSURE
                                                              // name, see that file

// blam-cc: stack -> local_player_index
// (Re)initializes local_player_index's first_person_weapon_interface when its controlled unit's
// current weapon (and that weapon's hud_interface tag and first-person model) are all valid: if
// it was already attached, detaches first; resets the animation/pose/blend fields; looks up
// whether the weapon and device HUD elements match this globals-defined interface's message
// table (hud_meter_find_matching_elements); and, only if both match, commits weapon_index,
// clears the remaining scratch fields, enters state 1 and re-attaches. Always finishes by
// resetting the interface's shutdown countdown (first_person_weapon_interface_tick_reset).
void first_person_weapon_interface_initialize(int16_t local_player_index)
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];
    uint8_t was_attached = fp->attached;
    struct object *unit;
    int16_t current_weapon_slot;
    datum_index weapon_index;
    struct object *weapon_object;
    char *weapon_tag_data;
    char *hud_interface_tag_data;
    int32_t *node_array_block;
    int16_t marker_node_index;
    uint8_t weapon_hud_matched;
    uint32_t weapon_tag_ref;
    uint32_t hud_interface_tag_ref;

    fp->weapon_index = (datum_index)0xffffffff;

    if (was_attached != 0) {
        effect_release_first_person_markers(local_player_index);
        particles_delete_by_first_person_weapon((uint8_t)local_player_index);
        fp->attached = 0;
    }

    if (fp->unit_index == (datum_index)0xffffffff) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    unit = *(struct object **)((char *)object_data->data + 8 +
                                (uint16_t)fp->unit_index * 0xc);
    current_weapon_slot = *(int16_t *)((char *)unit + 0x2f2); // unit::current_weapon_index
    if (current_weapon_slot == -1) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }
    weapon_index = *(datum_index *)((char *)unit + 0x2f8 + current_weapon_slot * 4); // unit::weapons[]
    if (weapon_index == (datum_index)0xffffffff) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    weapon_object = *(struct object **)((char *)object_data->data + 8 +
                                         (uint16_t)weapon_index * 0xc);
    weapon_tag_ref = *(uint32_t *)weapon_object;
    weapon_tag_data = (char *)tag_instances[(uint16_t)weapon_tag_ref].data;
    if (*(int32_t *)(weapon_tag_data + 0x468) == -1) { // UNSURE: Weapon tag field, see header
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    hud_interface_tag_ref = *(uint32_t *)(weapon_tag_data + 0x478); // UNSURE
    hud_interface_tag_data = (char *)tag_instances[(uint16_t)hud_interface_tag_ref].data;
    if (*(int32_t *)(hud_interface_tag_data + 0x48) == 0) { // UNSURE: WeaponHUDInterface reflexive
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }
    node_array_block = *(int32_t **)(hud_interface_tag_data + 0x4c); // UNSURE
    if (node_array_block == (int32_t *)0) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    fp->animation_index = -1;
    if (node_array_block[4] > 4 /* +0x10 */) {
        marker_node_index = *(int16_t *)(*(int32_t *)((char *)node_array_block + 0x14) + 8); // UNSURE
        if (marker_node_index != -1 &&
            *(int16_t *)((char *)hud_interface_tag_data + 0x22 +
                          (uint32_t)marker_node_index * 0xb4) > 8) { // UNSURE: model node table
            fp->animation_index = marker_node_index;
        }
    }

    if (*(int32_t *)(*(int32_t *)((char *)global_globals + 0x180) + 0xc) != -1) {
        fp->device_hud_valid = hud_meter_find_matching_elements(
            weapon_tag_ref, hud_interface_tag_ref, fp->device_hud_element);
    }
    weapon_hud_matched = hud_meter_find_matching_elements(weapon_tag_ref, hud_interface_tag_ref,
                                                            fp->weapon_hud_element);
    fp->weapon_hud_valid = weapon_hud_matched;

    if (weapon_hud_matched != 0 && fp->device_hud_valid != 0) {
        fp->weapon_index = weapon_index;
        fp->state = -1;
        fp->unknown_16 = -1;
        fp->unknown_1a = -1;
        fp->unknown_20 = -1;
        fp->unknown_28 = 0.0f;
        fp->charge = 0.0f;
        fp->unknown_10 = 0;
        fp->unknown_1e98 = -1;
        fp->unknown_1e9c = -1;
        first_person_weapon_set_state(local_player_index, 1, 0); // UNSURE: new_state guessed
        fp->blend_end = 0;
        if (was_attached != 0 && fp->attached != 1) {
            effect_reattach_markers_for_object(local_player_index, fp->weapon_index);
            fp->attached = 1;
        }
    }

    first_person_weapon_interface_tick_reset(local_player_index);
}

#if 0
Original Ghidra decompilation (0x493c60):

void FUN_00493c60(undefined4 param_1)

{
  char *pcVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  char cVar8;

  pcVar1 = (char *)((short)param_1 * 0x1ea0 + DAT_006b2d98);
  cVar2 = *pcVar1;
  pcVar1[8] = -1;
  pcVar1[9] = -1;
  pcVar1[10] = -1;
  pcVar1[0xb] = -1;
  if ((cVar2 != '\0') && (cVar2 != '\0')) {
    FUN_00450d50(param_1);
    FUN_00455c80();
    *pcVar1 = '\0';
  }
  if (*(uint *)(pcVar1 + 4) != 0xffffffff) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(pcVar1 + 4) & 0xffff) * 0xc);
    sVar3 = *(short *)(iVar4 + 0x2f2);
    if (((((sVar3 != -1) && (uVar5 = *(uint *)(iVar4 + 0x2f8 + sVar3 * 4), uVar5 != 0xffffffff)) &&
         (iVar4 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc)
                           & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), *(int *)(iVar4 + 0x468) != -1))
        && ((uVar6 = *(uint *)(iVar4 + 0x478), uVar6 != 0xffffffff &&
            (iVar4 = *(int *)((uVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
            *(int *)(iVar4 + 0x48) != 0)))) && (iVar7 = *(int *)(iVar4 + 0x4c), iVar7 != 0)) {
      pcVar1[0x14] = -1;
      pcVar1[0x15] = -1;
      if (((4 < *(int *)(iVar7 + 0x10)) &&
          (sVar3 = *(short *)(*(int *)(iVar7 + 0x14) + 8), sVar3 != -1)) &&
         (8 < *(short *)(sVar3 * 0xb4 + 0x22 + *(int *)(iVar4 + 0x78)))) {
        *(short *)(pcVar1 + 0x14) = sVar3;
      }
      if (*(int *)(*(int *)(DAT_00746fa0 + 0x180) + 0xc) != -1) {
        cVar8 = FUN_00493f00(pcVar1 + 0x1e10);
        pcVar1[0x1e0e] = cVar8;
      }
      cVar8 = FUN_00493f00(pcVar1 + 0x1d8e);
      pcVar1[0x1d8c] = cVar8;
      if ((cVar8 != '\0') && (pcVar1[0x1e0e] != '\0')) {
        *(uint *)(pcVar1 + 8) = uVar5;
        pcVar1[0xc] = -1;
        pcVar1[0xd] = -1;
        pcVar1[0x16] = -1;
        pcVar1[0x17] = -1;
        pcVar1[0x1a] = -1;
        pcVar1[0x1b] = -1;
        pcVar1[0x20] = -1;
        pcVar1[0x21] = -1;
        pcVar1[0x28] = '\0';
        pcVar1[0x29] = '\0';
        pcVar1[0x2a] = '\0';
        pcVar1[0x2b] = '\0';
        pcVar1[0x2c] = '\0';
        pcVar1[0x2d] = '\0';
        pcVar1[0x2e] = '\0';
        pcVar1[0x2f] = '\0';
        pcVar1[0x10] = '\0';
        pcVar1[0x11] = '\0';
        pcVar1[0x1e98] = -1;
        pcVar1[0x1e99] = -1;
        pcVar1[0x1e9a] = -1;
        pcVar1[0x1e9b] = -1;
        pcVar1[0x1e9c] = -1;
        pcVar1[0x1e9d] = -1;
        first_person_weapon_set_state(param_1,1);
        pcVar1[0x8a] = '\0';
        pcVar1[0x8b] = '\0';
        if ((cVar2 != '\0') && (*pcVar1 != '\x01')) {
          FUN_00450cb0(param_1,*(undefined4 *)(pcVar1 + 8));
          *pcVar1 = '\x01';
        }
      }
    }
  }
  FUN_004942e0();
  return;
}
#endif
