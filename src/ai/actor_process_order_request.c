// actor_process_order_request  (Ghidra: actor_process_order_request, already named)
// address 0x409ea0, size 621 bytes
// name confidence: 0.5   rewrite confidence: 0.2
// evidence: types/ai.h actor.unknown_64 (0x64, "actor_new sets -1", used here as a per-actor
//   cooldown timestamp)/mode (0x6c)/swarm (0x06)/order_committed (0x160)/awareness_level
//   (0x6a); calls actor_set_mode, this session's actor_check_melee_target_reachable
//   (actor_check_melee_target_reachable), actor_build_order_default (actor_build_order_default) and two more order builders;
//   phase-4 summary "central dispatcher that turns a requested order code into a concrete
//   built order for the actor, subject to per-code cooldowns".
//
// Kept close to the Ghidra decompilation given its size and an unresolved global lookup
// table (DAT_00655590); see UNSURE notes below.
// UNSURE: the builder calls (actor_build_order_default/_004044b0/_00404510/_00403f00) are shown with at
// most one visible stack argument each; the rest of their parameters (actor index and the
// order buffer pointer) are register-inherited from this function's own locals, matching
// each builder's own established signature in this session. The order buffer this function
// passes to actor_set_mode is a plain byte buffer sized to the largest builder used
// (0x5c bytes, actor_order's size), not a single shared named struct.
//   UNSURE: DAT_00655590 (an int16 table indexed by order_code) is not identified anywhere
//   else in this module; kept as a raw extern array.
//   UNSURE: actor+0xc0/0xaa (the "melee" order sub-state) and actor+0x9c (mode_data's first
//   field, reused here as a generic "current order marker") are used as raw/mode_data
//   offsets consistent with their treatment elsewhere in this session.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include <string.h>

extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t order_code_mode_data_expect[16]; // 0x00655590, UNSURE: guessed size

extern int32_t actor_build_order_default(uint32_t actor_index, int16_t order_code, actor_order *order, int16_t parameter); // 0x401090, this session
extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order); // 0x403f00, this session
extern void actor_build_order_return_to_anchor(uint32_t actor_index, actor_order *order); // 0x4044b0, this session
extern void actor_build_order_guard(uint32_t actor_index, actor_order *order, int16_t guard_at_current_position); // 0x404510, this session
extern uint8_t actor_update_melee_combat_action(datum_index actor_index); // 0x40cdf0, sibling session
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, sibling session
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, sibling session

// Central dispatcher: turns a requested order_code (or, if 0xffff, whichever of the actor's
// two pending order codes is set) into a concrete order, subject to a ~45-tick (0x2d) per-
// actor cooldown when no explicit code is given. See the file header for scope.
uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t committed = 0;
    uint32_t new_mode = 0;
    uint8_t order[0x5c];
    uint8_t have_order = 0;

    if (order_code == 0xffff && a->unknown_64 != -1 && game_time->game_time <= a->unknown_64 + 0x2d) {
        return 0;
    }
    a->unknown_64 = game_time->game_time;

    if (order_code == 0xffff) {
        order_code = *(uint16_t *)((uint8_t *)a + 0x60);
        if (order_code == 0xffff) {
            uint16_t alt = *(uint16_t *)((uint8_t *)a + 0x62);
            order_code = alt & (uint16_t)-(alt == 0xffff ? 0 : 1); // preserves the original's odd zero-vs-passthrough selection
        } else {
            *(uint16_t *)((uint8_t *)a + 0x60) = 0xffff;
        }
    }

    switch ((int16_t)order_code) {
    case 0: case 2: case 3: case 4: case 5: case 6: case 7:
        if (a->mode == 2 && *(int16_t *)(a->mode_data) == order_code_mode_data_expect[(int16_t)order_code]) {
            break;
        }
        memset(order, 0, sizeof(actor_order));
        if (actor_build_order_default(actor_index, order_code, (actor_order *)order, 0xffff) != 0) {
            have_order = 1;
            new_mode = 2;
        }
        break;
    case 1:
        if (a->awareness_level != 1) {
            a->awareness_level = 1;
            actor_set_mode(actor_index, 1, 0);
            return 1;
        }
        break;
    case 8:
        if (a->mode == 6 && *(int16_t *)(a->mode_data + (0xc0 - 0x9c)) == 1) {
            break;
        }
        memset(order, 0, sizeof(actor_order));
        actor_build_order_return_to_anchor(actor_index, (actor_order *)order);
        // UNSURE: the builder no longer reports success/failure explicitly in this rewrite
        // (it always fills the order); treated as always succeeding here.
        have_order = 1;
        new_mode = 6;
        break;
    case 9:
        if (a->mode == 6) {
            if (*(int16_t *)(a->mode_data + (0xc0 - 0x9c)) != 3) {
                a->mode_data[0xaa - 0x9c] = 1;
            }
        } else {
            memset(order, 0, sizeof(actor_order));
            actor_build_order_guard(actor_index, (actor_order *)order, 0);
            have_order = 1;
            new_mode = 6;
        }
        break;
    case 10:
        if (actor_get_current_mode_combat_grade(actor_index) != 3) {
            a->awareness_level = 3;
            a->unknown_72 = 2;
            a->unknown_6e = 2;
            if (actor_update_melee_combat_action(actor_index) == 0) {
                memset(order, 0, sizeof(actor_order));
                actor_build_order_return_to_anchor(actor_index, (actor_order *)order);
                have_order = 1;
                new_mode = 6;
            }
        }
        break;
    case 0xb:
        if (a->mode != 4) {
            if (a->order_committed == 0) {
                memset(order, 0, sizeof(order));
                *(int16_t *)(order + 8) = -1;
                *(int32_t *)(order + 0x1c) = -1;
                *(int16_t *)(order + 0x14) = 0xd;
                *(int16_t *)order = 0xb4;
                order[0xc] = 0;
                order[0xd] = 0;
                if (a->swarm == 0) {
                    actor_check_melee_target_reachable(actor_index, (int16_t *)order);
                    if (*(int16_t *)(order + 8) != -1) {
                        have_order = 1;
                        new_mode = 4;
                        break;
                    }
                    order[0x12] = 0; // UNSURE: local_82, byte offset guessed
                }
            }
            if (a->mode != 6) {
                break;
            }
        }
        // falls through to the mode==0 default below, matching the original's fallthrough
        break;
    }

    if (!have_order && a->mode == 0) {
        memset(order, 0, sizeof(actor_order));
        if (actor_build_order_default(actor_index, order_code, (actor_order *)order, 0xffff) != 0) {
            have_order = 1;
            new_mode = 2;
        }
    }

    if (have_order) {
        actor_set_mode(actor_index, new_mode, order);
        committed = 1;
    }
    return committed;
}

#if 0
Original Ghidra decompilation (0x409ea0):

undefined1 actor_process_order_request(uint param_1,ushort param_2)

{
  char cVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  bool bVar7;
  undefined8 uVar8;
  undefined4 uVar9;
  undefined1 local_91;
  undefined4 local_90;
  undefined1 local_8c;
  undefined1 local_8b;
  short local_88;
  undefined2 local_84;
  undefined1 local_82;
  undefined4 local_74;

  iVar3 = (param_1 & 0xffff) * 0x724;
  iVar5 = *(int *)(DAT_00880360 + 0x34) + iVar3;
  local_91 = 0;
  if (((param_2 == 0xffff) && (*(int *)(iVar5 + 100) != -1)) &&
     (*(int *)(DAT_006f1d6c + 0xc) <= *(int *)(iVar5 + 100) + 0x2d)) {
    return 0;
  }
  *(int *)(iVar5 + 100) = *(int *)(DAT_006f1d6c + 0xc);
  if (param_2 == 0xffff) {
    param_2 = *(ushort *)(iVar5 + 0x60);
    if (param_2 == 0xffff) {
      param_2 = *(ushort *)(iVar5 + 0x62) & (*(ushort *)(iVar5 + 0x62) == 0xffff) - 1;
    }
    else {
      *(undefined2 *)(iVar5 + 0x60) = 0xffff;
    }
  }
  switch((int)(short)param_2) {
  case 0:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
    if ((*(short *)(iVar5 + 0x6c) == 2) &&
       (*(short *)(iVar5 + 0x9c) == *(short *)(&DAT_00655590 + (short)param_2 * 2))) break;
    uVar8 = FUN_00401090(0xffffffff);
    puVar6 = (undefined4 *)((ulonglong)uVar8 >> 0x20);
    if ((char)uVar8 == '\0') break;
    goto LAB_00409faa;
  case 1:
    if (*(short *)(iVar5 + 0x6a) != 1) {
      *(undefined2 *)(iVar5 + 0x6a) = 1;
      actor_set_mode(param_1,1,0);
      return 1;
    }
    break;
  case 8:
    if (*(short *)(iVar5 + 0x6c) == 6) {
      bVar7 = *(short *)(iVar5 + 0xc0) == 1;
LAB_0040a09a:
      if (bVar7) break;
    }
    uVar8 = FUN_004044b0();
    puVar6 = (undefined4 *)((ulonglong)uVar8 >> 0x20);
    if ((char)uVar8 == '\0') break;
    uVar9 = 6;
    goto LAB_00409fac;
  case 9:
    if (*(short *)(iVar5 + 0x6c) == 6) {
      if (*(short *)(iVar5 + 0xc0) != 3) {
        *(undefined1 *)(iVar5 + 0xaa) = 1;
      }
    }
    else {
      uVar8 = FUN_00404510();
      puVar6 = (undefined4 *)((ulonglong)uVar8 >> 0x20);
      if ((char)uVar8 != '\0') {
        uVar9 = 6;
        goto LAB_00409fac;
      }
    }
    break;
  case 10:
    sVar2 = FUN_0040e760();
    if (sVar2 != 3) {
      *(undefined2 *)(iVar5 + 0x6a) = 3;
      *(undefined2 *)(iVar5 + 0x72) = 2;
      *(undefined2 *)(iVar5 + 0x6e) = 2;
      cVar1 = FUN_0040cdf0(param_1);
      if (cVar1 == '\0') {
        uVar8 = FUN_004044b0();
        puVar6 = (undefined4 *)((ulonglong)uVar8 >> 0x20);
        if ((char)uVar8 != '\0') {
          uVar9 = 6;
          goto LAB_00409fac;
        }
      }
    }
    break;
  case 0xb:
    if (*(short *)(iVar5 + 0x6c) != 4) {
      iVar3 = *(int *)(DAT_00880360 + 0x34) + iVar3;
      if (*(char *)(iVar3 + 0x160) == '\0') {
        puVar6 = &local_90;
        for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar6 = 0;
          puVar6 = puVar6 + 1;
        }
        local_88 = -1;
        local_74 = 0xffffffff;
        local_84 = 0xd;
        local_90._0_2_ = 0xb4;
        local_8c = 0;
        local_8b = 0;
        if (*(char *)(iVar3 + 6) == '\0') {
          FUN_00403f00(param_1);
          if (local_88 != -1) {
            puVar6 = &local_90;
            uVar9 = 4;
            goto LAB_00409fac;
          }
          local_82 = 0;
        }
      }
      bVar7 = *(short *)(iVar5 + 0x6c) == 6;
      goto LAB_0040a09a;
    }
  }
  if (*(short *)(iVar5 + 0x6c) == 0) {
    uVar8 = FUN_00401090(0xffffffff);
    puVar6 = (undefined4 *)((ulonglong)uVar8 >> 0x20);
    if ((char)uVar8 != '\0') {
LAB_00409faa:
      uVar9 = 2;
LAB_00409fac:
      actor_set_mode(param_1,uVar9,puVar6);
      local_91 = 1;
    }
  }
  return local_91;
}
#endif
