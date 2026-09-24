// player_check_vehicle_boarding_interaction  (Ghidra: FUN_004788a0; renamed per
// out/phase4/game_functions.md: "Evaluates whether the player can board, swap seats in, or pick
// up equipment from a nearby vehicle/object, and queues or commits the appropriate action.")
// address 0x4788a0, size 926 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md; types/game.h player::unit (0x34); types/objects.h
//   object::parent_object (0x11c); the established object_headers / tag_instances lookup idiom
//   used throughout this module; weapon_transfer_ammunition, object_try_and_get already
//   established elsewhere. Most of the item/weapon-tag fields this function reads (+0x200,
//   +0x208, +0x308, +0x318, +0x4cc) are not named by any header this module owns, and several
//   sibling helper functions it calls (unit_invalidate_local_player_zoom_level, hud_post_item_message, hud_add_item_message, unit_try_give_grenade,
//   unit_count_deployed_weapons, unit_check_weapon_use_permission, unit_weapon_is_best_of_type, unit_pickup_weapon) are outside this batch with
//   unverified signatures -- this rewrite is therefore a comparatively literal, offset-preserving
//   transcription rather than a fully named one, consistent with this module's own precedent for
//   very low-confidence legacy paths (e.g. player_respawn.c's leading despawn branch).
// register convention: none -- both are genuine stack parameters (Ghidra's own
//   param_1/param_2).
// UNSURE: essentially every field offset past player/object/object_header is unverified; see
//   evidence note above. Preserved exactly as Ghidra decompiled it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode;  // 0x00719720

extern void unit_invalidate_local_player_zoom_level(void); // 0x4726f0, not in this batch
extern uint8_t player_is_busy_with_interaction(void); // this batch, 0x478820 (player_is_busy_with_interaction);
    // UNSURE: called here with no visible arguments, unlike its own established two-register
    // signature -- kept as a bare call, matching Ghidra
extern void player_set_pending_interaction_action(int16_t priority_type, int16_t seat,
    uint32_t player_index, uint32_t candidate_object); // this batch, 0x478e00
extern void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t mode,
    uint32_t interaction_type_and_seat, int32_t secondary_key); // 0x478ff0, this module
    // (canonical form, per player_execute_pending_interaction.c)
extern void player_apply_pickup_effect(uint32_t player_index, uint32_t candidate_object); // this batch, 0x479930
extern void hud_post_item_message(int16_t a, uint8_t b); // 0x4ae350, not in this batch
extern void hud_add_item_message(uint32_t a); // 0x4ae400, not in this batch
extern uint8_t weapon_transfer_ammunition(uint32_t weapon_index, uint32_t source_item_index,
    int16_t amount, int16_t *out_transferred); // 0x4c2610, not in this batch; UNSURE exact signature
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_try_give_grenade(uint32_t candidate_object); // 0x56d080, not in this batch
extern void unit_pickup_weapon(uint8_t is_primary); // 0x56d400, not in this batch; UNSURE: called with
    // only one visible argument here, unlike unit_apply_starting_profile.c's own two-argument use
extern int16_t unit_count_deployed_weapons(void); // 0x56d990, not in this batch
extern uint8_t unit_check_weapon_use_permission(void); // 0x56da00, not in this batch
extern uint8_t unit_weapon_is_best_of_type(void); // 0x56dae0, not in this batch

// Evaluates a boarding/pickup/swap interaction between `player_index`'s unit and the nearby
// object `candidate_object`. See header note: this is a literal, offset-preserving
// transcription of Ghidra's own decompilation, not a fully re-derived one.
void player_check_vehicle_boarding_interaction(uint32_t player_index, uint32_t candidate_object)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit_handle = p->unit;
    object *unit_obj = (object *)((object_header *)object_headers->data)[unit_handle & 0xffff].data;
    object *candidate = (object *)((object_header *)object_headers->data)[candidate_object & 0xffff].data;

    if (candidate->parent_object == (datum_index)0xffffffff &&
        *(uint32_t *)((uint8_t *)candidate + 0x200) != (uint32_t)unit_handle) {
        int16_t slot;
        int16_t transferred = 0;

        for (slot = 0; slot < 4; slot++) {
            datum_index weapon = *(datum_index *)((uint8_t *)unit_obj + 0x2f8 + slot * 4);
            if (weapon != (datum_index)0xffffffff &&
                weapon_transfer_ammunition((uint32_t)weapon, candidate_object, p->interaction_seat, &transferred)) {
                if (0 < transferred) {
                    hud_post_item_message(p->local_player_index, *(uint8_t *)((uint8_t *)p + 100));
                }
                break;
            }
        }

        {
            object *device = object_try_and_get(candidate_object, 8);
            if (device != 0) {
                tag_instance *tag = &tag_instances[device->definition_tag & 0xffff];
                int16_t field_308 = *(int16_t *)((uint8_t *)tag->data + 0x308);
                if (field_308 == 6) {
                    if (unit_try_give_grenade(candidate_object) != 0) {
                        hud_post_item_message(p->local_player_index, *(uint8_t *)((uint8_t *)p + 100));
                    }
                } else if (field_308 != 0) {
                    object *unit_for_field = (object *)((object_header *)object_headers->data)[unit_handle & 0xffff].data;
                    if (*(int32_t *)((uint8_t *)unit_for_field + 0x318) == -1) {
                        player_apply_pickup_effect(player_index, candidate_object);
                    } else if (field_308 != *(int16_t *)((uint8_t *)tag->data + 0x308)) {
                        player_set_pending_interaction_action(5, (int16_t)0xffff, player_index, candidate_object);
                    }
                }
            }
        }

        {
            object *weapon_candidate = object_try_and_get(candidate_object, 4);
            if (weapon_candidate != 0 && unit_check_weapon_use_permission() != 0) {
                tag_instance *weapon_tag = &tag_instances[weapon_candidate->definition_tag & 0xffff];
                uint32_t flags_208 = *(uint32_t *)((uint8_t *)unit_obj + 0x208);
                unit_data *unit2 = (unit_data *)((object_header *)object_headers->data)[unit_handle & 0xffff].data;
                int16_t current_weapon_index = unit2->current_weapon_index;
                datum_index current_weapon = (datum_index)0xffffffff;
                int16_t weapon_state;
                uint8_t assassination_target = 0;

                if (current_weapon_index != -1) {
                    current_weapon = unit2->weapons[current_weapon_index];
                }
                weapon_state = unit_count_deployed_weapons();

                if (1 < weapon_state && current_weapon != (datum_index)0xffffffff &&
                    (*(uint8_t *)((uint8_t *)weapon_tag->data + 0x308) & 0x10) == 0) {
                    object *current_weapon_obj = (object *)((object_header *)object_headers->data)[current_weapon & 0xffff].data;
                    tag_instance *current_weapon_tag = &tag_instances[current_weapon_obj->definition_tag & 0xffff];
                    if ((*(uint8_t *)((uint8_t *)current_weapon_tag->data + 0x308) & 0x10) != 0) {
                        assassination_target = 1;
                    }
                }

                if ((flags_208 & 0x1800) == 0 || (*(uint8_t *)((uint8_t *)weapon_tag->data + 0x308) & 8) == 0) {
                    if (player_is_busy_with_interaction() == 0) {
                        if (!assassination_target && unit_weapon_is_best_of_type() != 0) {
                            object *reacquired = object_try_and_get(candidate_object, 4);
                            uint32_t priority;
                            if (weapon_state == 1 && reacquired != 0 &&
                                *(uint32_t *)reacquired != *(uint32_t *)weapon_candidate) {
                                // UNSURE: compares the two lookups' raw first dword
                                // (definition_tag), exactly as Ghidra decompiled it
                                priority = 7;
                            } else {
                                priority = 6;
                            }
                            player_set_pending_interaction_action((int16_t)priority, (int16_t)0xffff,
                                player_index, candidate_object);
                        }
                    }
                } else {
                    unit_pickup_weapon(1);
                    if (network_game_mode == 2) {
                        hud_post_item_message(p->local_player_index, *(uint8_t *)((uint8_t *)p + 100));
                    } else {
                        hud_add_item_message(0);
                    }
                    unit_invalidate_local_player_zoom_level();
                    if (network_game_mode == 2) {
                        game_engine_notify_player_interaction(1, 7, 0xffffffff, 0xffffffff);
                        return;
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4788a0), from tools/pack.py 0x4788a0:

void FUN_004788a0(uint param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  char cVar5;
  short sVar6;
  uint uVar7;
  uint *puVar8;
  uint uVar9;
  int iVar10;
  undefined4 uVar11;
  uint *local_8;
  int local_4;

  iVar10 = (param_1 & 0xffff) * 0x200;
  uVar7 = *(uint *)(iVar10 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  iVar10 = iVar10 + *(int *)(DAT_0087a480 + 0x34);
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
  local_4 = (param_2 & 0xffff) * 0xc;
  iVar2 = *(int *)(local_4 + 8 + *(int *)(DAT_008603b0 + 0x34));
  if ((*(int *)(iVar2 + 0x11c) == -1) && (*(uint *)(iVar2 + 0x200) != uVar7)) {
    sVar6 = 0;
    do {
      uVar7 = *(uint *)(iVar1 + 0x2f8 + sVar6 * 4);
      if ((uVar7 != 0xffffffff) &&
         (uVar7 = item_transfer_ammunition(uVar7,param_2,*(short *)(iVar10 + 2),(short *)&local_8),
         (char)uVar7 != '\0')) {
        if (0 < (short)local_8) {
          FUN_004ae350(*(undefined2 *)(iVar10 + 2),*(undefined1 *)(iVar10 + 100));
        }
        break;
      }
      sVar6 = sVar6 + 1;
    } while (sVar6 < 4);
    puVar8 = (uint *)object_try_and_get(8);
    if (puVar8 != (uint *)0x0) {
      iVar2 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      sVar6 = *(short *)(iVar2 + 0x308);
      if (sVar6 == 6) {
        cVar5 = FUN_0056d080(param_2);
        if (cVar5 != '\0') {
          FUN_004ae350(*(undefined2 *)(iVar10 + 2),*(undefined1 *)(iVar10 + 100));
        }
      }
      else if (sVar6 != 0) {
        if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(iVar10 + 0x34) & 0xffff) * 0xc) + 0x318) == -1) {
          FUN_00479930(param_1,param_2);
        }
        else if (sVar6 != *(short *)(iVar2 + 0x308)) {
          player_set_pending_interaction_action(5,0xffffffff);
        }
      }
    }
    local_8 = (uint *)object_try_and_get(4);
    if ((local_8 != (uint *)0x0) && (cVar5 = FUN_0056da00(), iVar2 = DAT_008603b0, cVar5 != '\0')) {
      iVar3 = *(int *)((*local_8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uVar7 = *(uint *)(iVar1 + 0x208);
      iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar10 + 0x34) & 0xffff) * 0xc
                      );
      sVar6 = *(short *)(iVar1 + 0x2f2);
      uVar9 = 0xffffffff;
      if (sVar6 != -1) {
        uVar9 = *(uint *)(iVar1 + 0x2f8 + sVar6 * 4);
      }
      sVar6 = FUN_0056d990();
      bVar4 = false;
      if ((((1 < sVar6) && (uVar9 != 0xffffffff)) && ((*(byte *)(iVar3 + 0x308) & 0x10) == 0)) &&
         ((*(byte *)(*(int *)((**(uint **)(*(int *)(iVar2 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) &
                              0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) & 0x10) != 0)) {
        bVar4 = true;
      }
      if (((uVar7 & 0x1800) == 0) || ((*(byte *)(iVar3 + 0x308) & 8) == 0)) {
        cVar5 = FUN_00478820();
        if (cVar5 == '\0') {
          if ((!bVar4) && (cVar5 = FUN_0056dae0(), cVar5 != '\0')) {
            puVar8 = (uint *)object_try_and_get(4);
            if ((sVar6 == 1) && ((puVar8 != (uint *)0x0 && (*puVar8 != *local_8)))) {
              uVar11 = 7;
            }
            else {
              uVar11 = 6;
            }
            player_set_pending_interaction_action(uVar11,0xffffffff);
          }
        }
        else {
          cVar5 = FUN_0056d400(1);
          if (cVar5 != '\0') {
            if (DAT_00719720 == 2) {
              FUN_004ae350(*(undefined2 *)(iVar10 + 2),*(undefined1 *)(iVar10 + 100));
            }
            else {
              FUN_004ae400(0);
            }
            FUN_004726f0();
            if (DAT_00719720 == 2) {
              FUN_00478ff0(1,7,0xffffffff,0xffffffff);
              return;
            }
          }
        }
      }
    }
  }
}
#endif
