// hud_draw_weapon_interface  (Ghidra: chimera__spectate_hud, renamed in the phase-4 review;
// the Chimera signature spectate_hud_sig matches at the entry)
// address 0x4b1e20, size 453 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b1e20..0x4b1fe4 in the phase-4 review. Per local
// player (argument: the player record): the weapon is the unit current weapon, else the
// current weapon of the parent vehicle (unit_get_weapon_object_index, EAX parent, CX its
// current weapon slot) when the seat flags have bit 3. With a weapon and a Weapon tag
// hud_interface (+0x48c): hud_weapon_crosshairs_draw (EAX tag, ECX player, stack ammo state)
// and hud_weapon_interface_draw_elements (tag, index, Weapon tag, ammo state, NULL, NULL,
// NULL). Without a weapon and with no deployed weapon (unit_count_deployed_weapons, EAX
// unit): only the crosshairs of the HUDGlobals default weapon hud (+0x2cc) with a zeroed
// state. Always: hud_draw_grenade_interface, then the weapon (or -1) is cached at
// hud_weapon_state + index * 0x28 + 0x20. The first rewrite passed wrong arguments to all
// four callees.
// register convention: plain cdecl, one stack argument.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "items.h"
#include "interface.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;                  // 0x0087bc14
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_weapon_interface_state *hud_weapon_state; // 0x00719430

extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970; blam-cc: EAX, CX
extern void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out); // 0x4c29d0, blam-cc: EAX item_index
extern int16_t unit_count_deployed_weapons(datum_index unit_index); // 0x56d990, blam-cc: EAX
extern void hud_weapon_crosshairs_draw(datum_index hud_tag, const player *p, const weapon_hud_ammo_state *ammo); // 0x4b2cf0, blam-cc: EAX hud_tag, ECX player
extern void hud_weapon_interface_draw_elements(datum_index hud_tag, int16_t local_player_index, const Weapon *weapon_tag,
                                               const weapon_hud_ammo_state *ammo, const uint16_t *parent_state_flags,
                                               const uint16_t *parent_overlay_types, const int16_t *parent_numbers); // 0x4b1ff0
extern void hud_draw_grenade_interface(int16_t local_player_index, datum_index unit_index); // 0x4b2ac0

void hud_draw_weapon_interface(player *p)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
    int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;
    datum_index weapon = slot != -1 ? *(datum_index *)(unit + 0x2f8 + slot * 4) : (datum_index)-1;
    uint8_t no_weapon = 0;
    weapon_hud_ammo_state ammo;

    if (weapon == (datum_index)-1) {
        datum_index parent;
        int16_t seat;

        unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
        parent = ((unit_object *)unit)->base.parent_object;
        seat = ((unit_object *)unit)->unit.vehicle_seat_index;
        if (parent == (datum_index)-1 || seat == -1) {
            no_weapon = 1;
        } else {
            uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
            uint8_t *seats = *(uint8_t **)((uint8_t *)tag_instances[*(datum_index *)parent_object & 0xffff].data + 0x2e8);

            if ((seats[seat * 0x11c] & 8) != 0) {
                weapon = unit_get_weapon_object_index(parent, *(int16_t *)(parent_object + 0x2f2));
                if (weapon == (datum_index)-1) {
                    no_weapon = 1;
                }
            }
        }
    }

    if (weapon != (datum_index)-1) {
        uint8_t *weapon_object = (uint8_t *)((object_header *)object_data->data)[weapon & 0xffff].data;
        Weapon *weapon_tag = (Weapon *)tag_instances[*(datum_index *)weapon_object & 0xffff].data;
        datum_index hud_tag;

        weapon_build_hud_ammo_state(weapon, &ammo);
        hud_tag = *(datum_index *)&((struct Weapon *)weapon_tag)->hud_interface.tag_id; // Weapon hud_interface tag id
        if (hud_tag != (datum_index)-1) {
            hud_weapon_crosshairs_draw(hud_tag, p, &ammo);
            hud_weapon_interface_draw_elements(hud_tag, p->local_player_index, weapon_tag, &ammo, 0, 0, 0);
        }
    } else if (no_weapon && unit_count_deployed_weapons(p->unit) == 0) {
        memset(&ammo, 0, sizeof(ammo));
        hud_weapon_crosshairs_draw(*(datum_index *)((uint8_t *)hud_globals_tag_data + 0x2cc), p, &ammo);
    }

    hud_draw_grenade_interface(p->local_player_index, p->unit);
    if (p->local_player_index != -1) {
        *(datum_index *)((uint8_t *)hud_weapon_state + p->local_player_index * 0x28 + 0x20) = weapon;
    }
}

#if 0
Original Ghidra decompilation (0x4b1e20):

void chimera__spectate_hud(int param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  uint uVar4;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar2 = DAT_008603b0;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(param_1 + 0x34) & 0xffff) * 0xc);
  sVar3 = *(short *)(iVar1 + 0x2f2);
  uVar4 = 0xffffffff;
  if (sVar3 != -1) {
    uVar4 = *(uint *)(iVar1 + 0x2f8 + sVar3 * 4);
  }
  if (uVar4 == 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(param_1 + 0x34) & 0xffff) * 0xc)
    ;
    if ((*(uint *)(iVar1 + 0x11c) != 0xffffffff) && (*(short *)(iVar1 + 0x2f0) != -1)) {
      if ((*(byte *)(*(short *)(iVar1 + 0x2f0) * 0x11c +
                    *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                  (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc) &
                                      0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) & 8) == 0)
      goto LAB_004b1fb1;
      uVar4 = unit_get_weapon_object_index();
      if (uVar4 != 0xffffffff) goto LAB_004b1ef8;
    }
    sVar3 = FUN_0056d990();
    if (sVar3 == 0) {
      local_1c = 0;
      local_18 = 0;
      local_14 = 0;
      local_10 = 0;
      local_c = 0;
      local_8 = 0;
      local_4 = 0;
      local_20 = 0;
      FUN_004b2cf0(&local_20);
    }
  }
  else {
LAB_004b1ef8:
    iVar1 = *(int *)((**(uint **)(*(int *)(iVar2 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) & 0xffff) *
                     0x20 + 0x14 + DAT_0087bc14);
    FUN_004c29d0(&local_20);
    iVar2 = *(int *)(iVar1 + 0x48c);
    if (iVar2 != -1) {
      FUN_004b2cf0(&local_20);
      FUN_004b1ff0(iVar2,*(undefined2 *)(param_1 + 2),iVar1,&local_20,0,0,0);
    }
  }
LAB_004b1fb1:
  FUN_004b2ac0(*(undefined2 *)(param_1 + 2),*(undefined4 *)(param_1 + 0x34));
  if (*(short *)(param_1 + 2) != -1) {
    *(uint *)(DAT_00719430 + 0x20 + *(short *)(param_1 + 2) * 0x28) = uVar4;
  }
  return;
}
#endif
