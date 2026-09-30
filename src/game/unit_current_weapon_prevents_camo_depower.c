// unit_current_weapon_prevents_camo_depower  (Ghidra: unit_current_weapon_prevents_camo_depower,
// already named)
// address 0x466390, size 133 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: raw disassembly (objdump -d --start-address=0x466390 --stop-address=0x466420) is
//   much cleaner than Ghidra's decompile of this one: `xor al,al` gives a plain false default
//   and every register read after entry only ever depends on ECX, not EAX (Ghidra's "in_EAX"
//   input and its "& 0xffffff00" masking are decompiler noise from the caller's leftover EAX --
//   see game_engine_variant_defaults... no, see FUN_00466420's header). The chain
//   player+0x34 (player::unit) -> object_data -> unit+0x2f2 (current_weapon_index,
//   types/units.h) -> unit+0x2f8 (weapons[], types/units.h) -> object_data ->
//   object::definition_tag -> tag_instances -> +0x308 matches types/tags.h Weapon
//   (Item base is 0x308 bytes, weapon_flags is the first field after it) and bit 13 of
//   WeaponFlags is literally named `does_not_depower_active_camo_in_multilplayer`.
// register convention: player handle in ECX (in_ECX); EAX is read by no instruction in this
//   function and is not part of its real signature.
//   // blam-cc: ECX -> player_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

extern data_array *player_data;      // 0x0087a480
extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

// blam-cc: ECX -> player_handle
// Returns whether the weapon the given player's unit currently has readied carries the
// "does not depower active camouflage in multiplayer" flag. False whenever the player has no
// unit, the unit has no readied weapon, or the weapon object/tag cannot be resolved.
uint8_t unit_current_weapon_prevents_camo_depower(datum_index player_handle)
{
    player *p;
    object *unit_obj;
    unit_data *unit;
    int16_t weapon_slot;
    datum_index weapon_handle;
    object *weapon_obj;
    Weapon *weapon_tag;

    if (player_handle == (datum_index)0xffffffff) {
        return 0;
    }

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    if (p->unit == (datum_index)0xffffffff) {
        return 0;
    }

    unit_obj = ((object_header *)object_data->data)[p->unit & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    weapon_slot = unit->current_weapon_index;
    if (weapon_slot == -1) {
        return 0;
    }

    weapon_handle = unit->weapons[weapon_slot];
    if (weapon_handle == (datum_index)0xffffffff) {
        return 0;
    }

    weapon_obj = ((object_header *)object_data->data)[weapon_handle & 0xffff].data;
    if (weapon_obj->definition_tag == (datum_index)0xffffffff) {
        return 0;
    }

    weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
    return (uint8_t)((weapon_tag->weapon_flags >> 13) & 1);
}

#if 0
Original Ghidra decompilation (0x466390), from tools/pack.py 0x466390:

uint unit_current_weapon_prevents_camo_depower(void)

{
  uint uVar1;
  int iVar2;
  uint in_EAX;
  uint uVar3;
  uint in_ECX;
  int iVar4;

  uVar3 = in_EAX & 0xffffff00;
  if ((in_ECX != 0xffffffff) &&
     (uVar1 = *(uint *)((in_ECX & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
     uVar1 != 0xffffffff)) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
    iVar4 = (int)*(short *)(iVar2 + 0x2f2);
    if ((iVar4 != -1) &&
       ((uVar1 = *(uint *)(iVar2 + 0x2f8 + iVar4 * 4), uVar1 != 0xffffffff &&
        (uVar1 = **(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc),
        uVar1 != 0xffffffff)))) {
      uVar3 = *(uint *)(*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 0xd &
              0xffffff01;
    }
  }
  return uVar3;
}

Raw disassembly (objdump -d -M intel --start-address=0x466390 --stop-address=0x466420):

00466390:  xor al,al
00466392:  cmp ecx,0xffffffff
00466395:  je 0x466414
00466397:  mov edx,ds:0x87a480
0046639d:  mov edx,[edx+0x34]
004663a0:  and ecx,0xffff
004663a6:  shl ecx,0x9
004663a9:  mov ecx,[ecx+edx*1+0x34]
004663ad:  cmp ecx,0xffffffff
004663b0:  je 0x466414
004663b2:  mov edx,ds:0x8603b0
004663b8:  mov edx,[edx+0x34]
004663bb:  and ecx,0xffff
004663c1:  lea ecx,[ecx+ecx*2]
004663c4:  mov ecx,[edx+ecx*4+0x8]
004663c8:  push esi
004663c9:  movsx esi,word ptr [ecx+0x2f2]
004663d0:  cmp esi,0xffffffff
004663d3:  je 0x466413
004663d5:  mov ecx,[ecx+esi*4+0x2f8]
004663dc:  cmp ecx,0xffffffff
004663df:  je 0x466413
004663e1:  and ecx,0xffff
004663e7:  lea ecx,[ecx+ecx*2]
004663ea:  mov edx,[edx+ecx*4+0x8]
004663ee:  mov ecx,[edx]
004663f0:  cmp ecx,0xffffffff
004663f3:  je 0x466413
004663f5:  mov edx,ds:0x87bc14
004663fb:  and ecx,0xffff
00466401:  shl ecx,0x5
00466404:  mov eax,[ecx+edx*1+0x14]
00466408:  mov eax,[eax+0x308]
0046640e:  shr eax,0xd
00466411:  and al,0x1
00466413:  pop esi
00466414:  ret
#endif
