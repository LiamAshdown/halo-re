// actor_process_order_request  (Ghidra: actor_process_order_request, already named)
// address 0x409ea0, size 621 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x409ea0..0x40a10b (the draft built default orders from the request code instead of
//   its order kind (table 0x655590), passed the request code in the idle fallback where the binary passes 0,
//   ignored the order builders' results and mis-shaped the melee order). Stack (actor, request code or -1).
//   Without an explicit request, at most one every 45 ticks, taking the pending request at +0x60 (then clearing
//   it) or else +0x62 (-1 meaning 0). 1: awareness 1 (mode 1); 8: return to anchor (mode 6) unless already
//   guarding that way; 9: guard (mode 6), or flag +0xaa on a mode 6 actor not in guard kind 3; 10: combat
//   awareness, melee or return to anchor; 11: a melee order (mode 4) when a target is reachable, else return to
//   anchor; others: a default order (mode 2) of the table's kind. Any actor left in mode 0 gets a default order
//   of kind 0. Returns 1 when a new mode was set.
// blam-cc: stack -> (actor_index, order_code)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t order_code_mode_data_expect[12]; // 0x00655590: request code -> default order kind

extern int32_t actor_build_order_default(uint32_t actor_index, int16_t order_code, actor_order *order, int16_t parameter); // 0x401090, EAX, ECX, EDX, stack
extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order); // 0x403f00, stack, EBX
extern int32_t actor_build_order_return_to_anchor(uint32_t actor_index, actor_order *order); // 0x4044b0, EAX, EDX
extern int32_t actor_build_order_guard(uint32_t actor_index, actor_order *order, int16_t guard_at_current_position); // 0x404510, EAX, EDX, EBX
extern uint8_t actor_update_melee_combat_action(datum_index actor_index); // 0x40cdf0, stack
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, EAX

uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    int16_t mode = ((actor *)act)->mode;
    uint8_t order[k_actor_mode_data_size];
    int16_t code = (int16_t)order_code;

    if (code == -1 && ((struct actor *)act)->last_order_request_time != -1 && ((struct actor *)act)->last_order_request_time + 0x2d >= game_time->game_time) {
        return 0;
    }
    ((struct actor *)act)->last_order_request_time = game_time->game_time;
    if (code == -1) {
        code = ((struct actor *)act)->pending_order_request;
        if (code != -1) {
            ((struct actor *)act)->pending_order_request = -1;
        } else {
            code = ((struct actor *)act)->standing_order_request;
            if (code == -1) {
                code = 0;
            }
        }
    }

    switch (code) {
    case 1:
        if (((actor *)act)->awareness_level != 1) {
            ((actor *)act)->awareness_level = 1;
            actor_set_mode(actor_index, 1, 0);
            return 1;
        }
        break;

    case 8:
        if (mode == 6 && *(int16_t *)(act + 0xc0) == 1) {
            break;
        }
        if (actor_build_order_return_to_anchor(actor_index, (actor_order *)order)) {
            actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 9:
        if (mode == 6) {
            if (*(int16_t *)(act + 0xc0) != 3) {
                act[0xaa] = 1;
            }
            break;
        }
        if (actor_build_order_guard(actor_index, (actor_order *)order, 0)) {
            actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 10:
        if (actor_get_current_mode_combat_grade(actor_index) == 3) {
            break;
        }
        ((actor *)act)->awareness_level = 3;
        ((struct actor *)act)->minimum_combat_status = 2;
        ((struct actor *)act)->combat_status = 2;
        if (actor_update_melee_combat_action(actor_index)) {
            break;
        }
        if (actor_build_order_return_to_anchor(actor_index, (actor_order *)order)) {
            actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 11:
        if (mode == 4) {
            break;
        }
        if (act[0x160] == 0) {
            memset(order, 0, 0x30);
            ((struct actor_order *)order)->parameter = -1;
            *(int32_t *)(order + 0x1c) = -1;
            *(int16_t *)(order + 0xc) = 0xd;
            ((struct actor_order *)order)->order_code = 0xb4;
            order[0x4] = 0;
            order[0x5] = 0;
            if (act[0x6] == 0) {
                actor_check_melee_target_reachable(actor_index, (int16_t *)order);
                if (((struct actor_order *)order)->parameter != -1) {
                    actor_set_mode(actor_index, 4, order);
                    return 1;
                }
                order[0xe] = 0;
            }
        }
        if (((actor *)act)->mode == 6) {
            break;
        }
        if (actor_build_order_return_to_anchor(actor_index, (actor_order *)order)) {
            actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 0: case 2: case 3: case 4: case 5: case 6: case 7:
        if (mode == 2 && *(int16_t *)&((struct actor *)act)->mode_data == order_code_mode_data_expect[code]) {
            break;
        }
        if (actor_build_order_default(actor_index, order_code_mode_data_expect[code], (actor_order *)order, -1)) {
            actor_set_mode(actor_index, 2, order);
            return 1;
        }
        break;

    default:
        break;
    }

    // 0x409f8a: an idle actor always gets something to do
    if (((actor *)act)->mode == 0 &&
        actor_build_order_default(actor_index, 0, (actor_order *)order, -1)) {
        actor_set_mode(actor_index, 2, order);
        return 1;
    }
    return 0;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
