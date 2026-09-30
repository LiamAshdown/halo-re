// player_execute_weapon_drop_interaction  (Ghidra: FUN_004790d0)
// address 0x4790d0, size 353 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x4790d0..0x479230 (the draft read unit fields through an object pointer cast to
//   unit_data and dropped the pickup / HUD / zoom / notify register arguments). Stack: player. Interaction 6
//   (swap): the held weapon is remembered and dropped; when the interaction object (+0x24) is then picked up
//   (0x56d400: EAX weapon, ECX unit, stack 1) the HUD shows it (0x4ae400: AX local player, ECX its tag, BL 0),
//   the zoom is reset and an authoritative unit tells the game engine (0x478ff0: ECX player, EDI the object,
//   stack 1, type, seat, the dropped weapon); returns 1 either way. Interaction 7 (pick up) does the same with
//   no drop and returns 0; anything else returns 0.
// blam-cc: stack -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); // 0x56d400, stack, EAX, ECX
extern void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind, int16_t count); // 0x4ae400, EAX, ECX, BL, stack
extern void unit_invalidate_local_player_zoom_level(datum_index unit); // 0x4726f0, EAX


#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

uint8_t player_execute_weapon_drop_interaction(uint32_t player_index)
{
    uint8_t *record = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    datum_index unit_index = ((player *)record)->unit;
    uint8_t *unit = OBJECT_DATA(unit_index);
    datum_index held_weapon = k_datum_index_none;
    uint8_t result = 0;

    switch (((player *)record)->interaction_type) {
    case 6: {
        uint8_t *current = OBJECT_DATA(unit_index);
        int16_t slot = *(int16_t *)(current + 0x2f2);
        uint8_t picked_up = 0;

        if (slot != -1) {
            held_weapon = *(datum_index *)(current + 0x2f8 + slot * 4);
        }
        if (unit_drop_current_weapon(unit_index, 1) &&
            unit_pickup_weapon(1, ((player *)record)->interaction_object, unit_index)) {
            hud_add_item_message(((player *)record)->local_player_index,
                (int32_t)*(datum_index *)OBJECT_DATA(((player *)record)->interaction_object), 0, 0);
            unit_invalidate_local_player_zoom_level(unit_index);
            picked_up = 1;
        }
        result = 1;
        if (picked_up != 1) {
            return result;
        }
        break;
    }
    case 7:
        if (!unit_pickup_weapon(1, ((player *)record)->interaction_object, unit_index)) {
            return 0;
        }
        hud_add_item_message(((player *)record)->local_player_index,
            (int32_t)*(datum_index *)OBJECT_DATA(((player *)record)->interaction_object), 0, 0);
        break;
    default:
        return 0;
    }
    if (((unit_object *)unit)->base.network_role == 0) {
        game_engine_notify_player_interaction(player_index, ((player *)record)->interaction_object, 1,
            *(uint16_t *)&((player *)record)->interaction_type, *(uint16_t *)&((player *)record)->interaction_seat, (int32_t)held_weapon);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4790d0), from tools/pack.py 0x4790d0:

undefined1 FUN_004790d0(uint param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  bool bVar6;
  undefined1 uVar7;
  int iVar8;
  undefined4 uVar9;

  iVar8 = (param_1 & 0xffff) * 0x200;
  uVar2 = *(uint *)(iVar8 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  iVar8 = iVar8 + *(int *)(DAT_0087a480 + 0x34);
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
  uVar7 = 0;
  uVar9 = 0xffffffff;
  if (*(short *)(iVar8 + 0x28) == 6) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar8 + 0x34) & 0xffff) * 0xc);
    sVar1 = *(short *)(iVar4 + 0x2f2);
    uVar9 = 0xffffffff;
    if (sVar1 != -1) {
      uVar9 = *(undefined4 *)(iVar4 + 0x2f8 + sVar1 * 4);
    }
    cVar5 = unit_drop_current_weapon(uVar2,1);
    if ((cVar5 == '\0') || (cVar5 = FUN_0056d400(1), cVar5 == '\0')) {
      bVar6 = false;
    }
    else {
      FUN_004ae400(0);
      FUN_004726f0();
      bVar6 = true;
    }
    uVar7 = 1;
    if (!bVar6) {
      return 1;
    }
  }
  else {
    if (*(short *)(iVar8 + 0x28) != 7) {
      return 0;
    }
    cVar5 = FUN_0056d400(1);
    if (cVar5 == '\0') {
      return 0;
    }
    FUN_004ae400(0);
  }
  if (*(int *)(iVar3 + 4) == 0) {
    FUN_00478ff0(1,*(undefined2 *)(iVar8 + 0x28),*(undefined2 *)(iVar8 + 0x2a),uVar9);
  }
  return uVar7;
}
#endif
