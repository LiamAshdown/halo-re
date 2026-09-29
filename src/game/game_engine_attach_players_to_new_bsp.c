// game_engine_attach_players_to_new_bsp  (Ghidra: FUN_00473e90; named per this rewrite)
// address 0x473e90, size 517 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("After a structure-BSP switch, attempts to give
//   every player without a unit a valid unit and attach it to the new BSP's parent object,
//   retrying as needed"); types/game.h player_globals (mode +0x14, unknown_16 +0x16);
//   types/objects.h object_iterator, _object_mask_projectile/_object_mask_biped/
//   _object_mask_vehicle, parent_object (+0x11c); types/units.h biped_data::flags bit 0
//   (grounded) and vehicle_data::airborne_ticks. objdump -d -M intel
//   --start-address=0x473e90 --stop-address=0x4740a0 bin/halo.exe pins every register and the
//   two data ^ 0x69746572 stores (the data_iterator +0x0c signature, types/memory.h, now
//   stored), plus an unrelated 0x86868686 filler dword beside the first object_iterator).
// register convention: no arguments; return value in AL.
//
// UNSURE: the walk-to-root-ancestor loop's result is discarded (reverted to the previous best
// candidate) whenever the root turns out to be a grounded biped or an airborne vehicle, and
// player_globals::mode is stamped 3 in that case; the broader purpose of `mode` across this
// whole state machine is not established beyond what each assignment site shows.
// UNSURE: unit_any_dying_or_seat_transition and ai_scan_for_recent_combat_activity(1) are out-of-batch predicates (units and ai modules
// respectively) gating the very first attempt; their real behavior is not recovered here.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <stdint.h>

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_any_dying_or_seat_transition(void); // 0x56c070, units module, not in this batch; UNSURE purpose,
    // takes no visible arguments
extern uint8_t ai_scan_for_recent_combat_activity(uint32_t param); // 0x42c3e0, ai module, not in this batch;
    // blam-cc: stack -> param; UNSURE purpose
extern void player_respawn(datum_index player_handle); // this module's next batch, 0x477ea0;
    // blam-cc: EAX -> player_handle
extern uint8_t player_attach_unit_to_parent(datum_index player_handle, datum_index parent_object,
                             void *local_offset); // this batch, 0x475c60; blam-cc: EAX ->
    // player_handle, EDX -> parent_object, stack -> local_offset ("Attaches a player's unit as
    // a child of a target parent object at a given local offset" per
    // out/phase4/game_functions.md)

// Deferral gate (only checked while a retry is not already pending): bails out (mode 1) if any
// projectile object exists or unit_any_dying_or_seat_transition says so, and bails out (mode 2) if ai_scan_for_recent_combat_activity(1)
// says so. Otherwise scans every player, walking each one's unit up its parent chain to find
// the topmost attached object; a candidate is dropped (mode 3, previous best kept) when that
// root turns out to be a grounded biped or an airborne vehicle. If any candidate root survives
// the scan, every player still missing a unit is respawned and, if that succeeded, attached as
// a child of the surviving root object at its +0xa0 local-offset field. Finally updates the
// "keep retrying" flag: it is cleared on success and left set (to keep retrying) on failure
// only if it was already set.
uint8_t game_engine_attach_players_to_new_bsp(void)
{
    object_iterator obj_iter;
    data_iterator player_iter;
    player *plr;
    datum_index unit_handle, walk, next, root, best_root, player_handle;
    object *root_obj;
    biped_data *biped;
    vehicle_data *vehicle;
    uint8_t success;

    local_player_globals->mode = 0;

    if (local_player_globals->teleported == 0) {
        obj_iter.type_mask = _object_mask_projectile;
        obj_iter.flags_mask = 0;
        obj_iter.index = 0;
        obj_iter.handle = (datum_index)-1;
        if (object_iterator_next(&obj_iter) != (object *)0 || unit_any_dying_or_seat_transition() != 0) {
            local_player_globals->mode = 1;
            return 0;
        }
        if (local_player_globals->teleported == 0 && ai_scan_for_recent_combat_activity(1) != 0) {
            local_player_globals->mode = 2;
            return 0;
        }
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    best_root = (datum_index)-1;
    success = 0;

    plr = (player *)data_iterator_next(&player_iter);
    if (plr != (player *)0) {
        do {
            unit_handle = plr->unit;
            root = best_root;
            if (unit_handle != (datum_index)-1) {
                walk = unit_handle;
                do {
                    root = walk;
                    next = ((object_header *)object_data->data)[walk & 0xffff].data->parent_object;
                    walk = next;
                } while (next != (datum_index)-1);

                if (root == unit_handle) {
                    root_obj = object_try_and_get(unit_handle, _object_mask_biped);
                    if (root_obj != (object *)0) {
                        biped = (biped_data *)((uint8_t *)root_obj + 0x4cc);
                        if ((biped->flags & 1) != 0) { // grounded
                            local_player_globals->mode = 3;
                            root = best_root;
                        }
                    }
                } else {
                    root_obj = object_try_and_get(root, _object_mask_vehicle);
                    if (root_obj != (object *)0) {
                        vehicle = (vehicle_data *)((uint8_t *)root_obj + 0x4cc);
                        if (vehicle->airborne_ticks != 0) {
                            local_player_globals->mode = 3;
                            root = best_root;
                        }
                    }
                }
            }
            plr = (player *)data_iterator_next(&player_iter);
            best_root = root;
        } while (plr != (player *)0);

        success = 0;
        if (best_root != (datum_index)-1) {
            success = 1;
            player_iter.data = player_data;
            player_iter.next_index = 0;
            player_iter.index = (datum_index)-1;
            player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
            plr = (player *)data_iterator_next(&player_iter);
            while (plr != (player *)0) {
                if (plr->unit == (datum_index)-1) {
                    player_handle = player_iter.index;
                    player_respawn(player_handle);
                    if (plr->unit == (datum_index)-1) {
                        success = 0;
                    } else {
                        root_obj = ((object_header *)object_data->data)[best_root & 0xffff].data;
                        success = player_attach_unit_to_parent(player_handle, best_root, (uint8_t *)root_obj + 0xa0);
                    }
                }
                plr = (player *)data_iterator_next(&player_iter);
            }
        }
    }

    if (local_player_globals->teleported == 0 || success != 0) {
        local_player_globals->teleported = 0;
    } else {
        local_player_globals->teleported = 1;
    }
    if (success != 0) {
        local_player_globals->mode = 0;
    }
    return success;
}

#if 0
Original Ghidra decompilation (0x473e90), from tools/pack.py 0x473e90:

uint FUN_00473e90(void)

{
  char *pcVar1;
  uint uVar2;
  undefined4 uVar3;
  bool bVar4;
  byte bVar5;
  undefined1 uVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  uint local_14;
  ushort local_10;
  undefined2 local_e;
  undefined4 local_c;
  uint local_8;

  iVar11 = DAT_0087a478;
  pcVar1 = (char *)(DAT_0087a478 + 0x16);
  *(undefined2 *)(DAT_0087a478 + 0x14) = 0;
  if (*pcVar1 == '\0') {
    local_8 = 0x86868686;
    local_14 = 0x20;
    local_10 = local_10 & 0xff00;
    local_e = 0;
    local_c = 0xffffffff;
    uVar7 = object_iterator_next(&local_14);
    if ((uVar7 != 0) || (uVar7 = FUN_0056c070(), (char)uVar7 != '\0')) {
      *(undefined2 *)(iVar11 + 0x14) = 1;
      return uVar7 & 0xffffff00;
    }
    if ((*(char *)(iVar11 + 0x16) == '\0') && (uVar7 = FUN_0042c3e0(1), (char)uVar7 != '\0')) {
      *(undefined2 *)(iVar11 + 0x14) = 2;
      return uVar7 & 0xffffff00;
    }
  }
  local_14 = DAT_0087a480;
  uVar8 = DAT_0087a480 ^ 0x69746572;
  local_10 = 0;
  local_c = 0xffffffff;
  local_8 = uVar8;
  iVar9 = data_iterator_next();
  uVar7 = 0xffffffff;
  bVar5 = 0;
  if (iVar9 != 0) {
    do {
      uVar2 = *(uint *)(iVar9 + 0x34);
      uVar12 = uVar7;
      if (uVar2 != 0xffffffff) {
        uVar12 = uVar2;
        do {
          uVar10 = uVar12;
          uVar12 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar10 & 0xffff) * 0xc) +
                            0x11c);
        } while (uVar12 != 0xffffffff);
        uVar12 = uVar2;
        if (uVar10 == uVar2) {
          iVar11 = object_try_and_get(1);
          if (iVar11 != 0) {
            bVar4 = (bool)(*(byte *)(iVar11 + 0x4cc) & 1);
LAB_00473fb3:
            if (bVar4 != false) {
              *(undefined2 *)(DAT_0087a478 + 0x14) = 3;
              uVar12 = uVar7;
            }
          }
        }
        else {
          iVar11 = object_try_and_get(2);
          if (iVar11 != 0) {
            bVar4 = *(char *)(iVar11 + 0x4d0) != '\0';
            goto LAB_00473fb3;
          }
        }
      }
      iVar9 = data_iterator_next();
      uVar7 = uVar12;
    } while (iVar9 != 0);
    iVar11 = DAT_0087a478;
    bVar5 = 0;
    if (uVar12 != 0xffffffff) {
      local_10 = 0;
      bVar5 = 1;
      local_14 = DAT_0087a480;
      local_c = 0xffffffff;
      local_8 = uVar8;
      iVar9 = data_iterator_next();
      iVar11 = DAT_0087a478;
      uVar3 = local_c;
      while (DAT_0087a478 = iVar11, iVar9 != 0) {
        local_c = uVar3;
        if (*(int *)(iVar9 + 0x34) == -1) {
          player_respawn();
          if (*(int *)(iVar9 + 0x34) == -1) {
            bVar5 = 0;
          }
          else {
            bVar5 = FUN_00475c60(uVar3,uVar12,
                                 *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                         (uVar12 & 0xffff) * 0xc) + 0xa0);
          }
        }
        iVar9 = data_iterator_next();
        iVar11 = DAT_0087a478;
        uVar3 = local_c;
      }
    }
  }
  if ((*(char *)(iVar11 + 0x16) == '\0') || (bVar5 != 0)) {
    uVar6 = 0;
  }
  else {
    uVar6 = 1;
  }
  *(undefined1 *)(iVar11 + 0x16) = uVar6;
  if (bVar5 != 0) {
    *(undefined2 *)(iVar11 + 0x14) = 0;
  }
  return (uint)bVar5;
}
#endif
