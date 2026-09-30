// player_swap_to_weapon  (Ghidra: FUN_00479240)
// address 0x479240, size 351 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x479240..0x47939e (the draft read the unit fields through an object pointer cast to
//   unit_data, 0x1f4 bytes early, and dropped every register argument of the pickup / HUD / zoom calls).
//   EAX: player, stack: weapon. Interaction 6 (swap): the weapon becomes the desired slot (+0x2f4) and is
//   readied unless it is already current; the current weapon is dropped and, if the interaction object
//   (+0x24) is then picked up (0x56d400: EAX weapon, ECX unit, stack 1), the HUD shows it (0x4ae400: AX local
//   player, ECX its tag, BL 0) and the zoom is reset (0x4726f0); returns 1. Interaction 7 (pick up) picks the
//   object up and shows it on the HUD but returns 0, as does any other interaction.
// blam-cc: EAX -> player_index, stack -> target_weapon

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"
#include "fn_units.h"
#include "fn_interface.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0


extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); // 0x56d400, stack, EAX, ECX

extern void unit_invalidate_local_player_zoom_level(datum_index unit); // 0x4726f0, EAX

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

uint8_t player_swap_to_weapon(uint32_t player_index, datum_index target_weapon)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200);
    uint8_t *record = (uint8_t *)p;
    datum_index unit_index = ((struct player *)record)->unit;
    datum_index interaction_object = ((struct player *)record)->interaction_object;
    uint8_t *unit = OBJECT_DATA(unit_index);

    switch (((struct player *)record)->interaction_type) {
    case 6: {
        uint8_t *current = OBJECT_DATA(unit_index);
        int16_t slot = *(int16_t *)(current + 0x2f2);
        datum_index current_weapon = (slot != -1) ? *(datum_index *)(current + 0x2f8 + slot * 4) : k_datum_index_none;

        if (current_weapon != target_weapon) {
            int32_t i;

            for (i = 0; i < 4; i++) {
                if (*(datum_index *)(unit + 0x2f8 + i * 4) == target_weapon) {
                    ((unit_object *)unit)->unit.desired_weapon_index = (int16_t)i;
                    unit_ready_desired_weapon(unit_index, 1);
                    break;
                }
            }
        }
        if (unit_drop_current_weapon(unit_index, 1) &&
            unit_pickup_weapon(1, interaction_object, unit_index)) {
            hud_add_item_message(((struct player *)record)->local_player_index,
                (int32_t)*(datum_index *)OBJECT_DATA(interaction_object), 0, 0);
            unit_invalidate_local_player_zoom_level(unit_index);
        }
        return 1;
    }
    case 7:
        if (unit_pickup_weapon(1, interaction_object, unit_index)) {
            hud_add_item_message(((struct player *)record)->local_player_index,
                (int32_t)*(datum_index *)OBJECT_DATA(interaction_object), 0, 0);
        }
        return 0;
    default:
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x479240), from tools/pack.py 0x479240:

undefined4 FUN_00479240(int param_1)

{
  short sVar1;
  int iVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;

  iVar4 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar4 + 0x34) & 0xffff) * 0xc);
  if (*(short *)(iVar4 + 0x28) != 6) {
    if ((*(short *)(iVar4 + 0x28) == 7) && (cVar3 = FUN_0056d400(1), cVar3 != '\0')) {
      FUN_004ae400(0);
      return 0;
    }
    return 0;
  }
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar4 + 0x34) & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar5 + 0x2f2);
  iVar7 = -1;
  if (sVar1 != -1) {
    iVar7 = *(int *)(iVar5 + 0x2f8 + sVar1 * 4);
  }
  if (iVar7 != param_1) {
    iVar5 = 0;
    piVar6 = (int *)(iVar2 + 0x2f8);
    do {
      if (*piVar6 == param_1) {
        *(short *)(iVar2 + 0x2f4) = (short)iVar5;
        unit_ready_desired_weapon(*(undefined4 *)(iVar4 + 0x34),1);
        break;
      }
      iVar5 = iVar5 + 1;
      piVar6 = piVar6 + 1;
    } while (iVar5 < 4);
  }
  cVar3 = unit_drop_current_weapon(*(undefined4 *)(iVar4 + 0x34),1);
  if ((cVar3 != '\0') && (cVar3 = FUN_0056d400(1), cVar3 != '\0')) {
    FUN_004ae400(0);
    FUN_004726f0();
  }
  return 1;
}
#endif
