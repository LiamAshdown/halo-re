// hud_player_weapon_ammo_state  (Ghidra: FUN_004acef0, renamed in the phase-4 review)
// address 0x4acef0, size 226 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4acef0..0x4acfd1 in the phase-4 review. EAX is a player
// record (the only caller, hud_draw_multitexture_overlay, passes players->data + index * 0x200
// and the function reads player::unit at +0x34), not a unit index. The weapon is the unit
// current weapon (unit +0x2f2 / +0x2f8); failing that, when the unit rides a seat of its parent
// (+0x11c, +0x2f0) whose Unit tag seat flags (seats at tag +0x2e8, stride 0x11c) have bit 3
// set, the parent current weapon through unit_get_weapon_object_index (EAX parent datum, CX
// parent current weapon slot). The first rewrite passed 0, 0 to that call and took a unit
// index. weapon_build_hud_ammo_state (0x4c29d0, EAX weapon) fills the caller buffer.
// register convention: EAX player; one stack argument; returns a byte in AL.
//   // blam-cc: player -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "interface.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970; blam-cc: EAX, CX
extern void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out); // 0x4c29d0, blam-cc: EAX item_index

// blam-cc: player -> EAX
// Fills out with the HUD ammo state of the weapon the player is holding, or of the seat
// weapon of the vehicle it rides. Returns 0 (out untouched) when there is none.
uint8_t hud_player_weapon_ammo_state(const player *p, weapon_hud_ammo_state *out)
{
    uint8_t *unit;
    datum_index weapon;
    int16_t slot;

    unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
    slot = ((unit_object *)unit)->unit.current_weapon_index; // unit current_weapon_index
    weapon = (datum_index)-1;
    if (slot != -1) {
        weapon = *(datum_index *)(unit + 0x2f8 + slot * 4); // unit weapons[slot]
    }
    if (weapon == (datum_index)-1) {
        datum_index parent;
        int16_t seat;
        uint8_t *parent_object;
        uint8_t *seats;

        unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
        parent = ((unit_object *)unit)->base.parent_object; // object parent_object
        if (parent == (datum_index)-1) {
            return 0;
        }
        seat = ((unit_object *)unit)->unit.vehicle_seat_index; // unit vehicle_seat_index
        if (seat == -1) {
            return 0;
        }
        parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
        seats = *(uint8_t **)((uint8_t *)tag_instances[*(datum_index *)parent_object & 0xffff].data + 0x2e8);
        if ((seats[seat * 0x11c] & 8) == 0) {
            return 0;
        }
        parent = ((unit_object *)unit)->base.parent_object;
        parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
        weapon = unit_get_weapon_object_index(parent, *(int16_t *)(parent_object + 0x2f2));
        if (weapon == (datum_index)-1) {
            return 0;
        }
    }
    weapon_build_hud_ammo_state(weapon, out);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4acef0):

undefined4 FUN_004acef0(undefined4 param_1)

{
  short sVar1;
  uint uVar2;
  int in_EAX;
  int iVar3;
  int iVar4;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(in_EAX + 0x34) & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar3 + 0x2f2);
  iVar4 = -1;
  if (sVar1 != -1) {
    iVar4 = *(int *)(iVar3 + 0x2f8 + sVar1 * 4);
  }
  if (iVar4 == -1) {
    iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(in_EAX + 0x34) & 0xffff) * 0xc);
    uVar2 = *(uint *)(iVar3 + 0x11c);
    if (((uVar2 != 0xffffffff) && (sVar1 = *(short *)(iVar3 + 0x2f0), sVar1 != -1)) &&
       ((*(byte *)(sVar1 * 0x11c +
                  *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                (uVar2 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                   DAT_0087bc14) + 0x2e8)) & 8) != 0)) {
      iVar3 = unit_get_weapon_object_index();
      if (iVar3 != -1) goto LAB_004acfb9;
    }
    return 0;
  }
LAB_004acfb9:
  FUN_004c29d0(param_1);
  return 1;
}
#endif
