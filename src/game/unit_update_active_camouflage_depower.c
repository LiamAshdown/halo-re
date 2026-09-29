// unit_update_active_camouflage_depower  (Ghidra: FUN_00466420; named per
// out/phase4/game_functions.md: "Decays a unit's active-camouflage-related timer field each
// tick, using the current weapon's camo-depower flag to determine how fast it counts down.")
// address 0x466420, size 272 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x466420
//   --stop-address=0x466530) because Ghidra invents an "in_EAX"/"unaff_EBX" story for the two
//   callees that the real code does not support:
//     - unit_get_weapon_object_index (0x569970, blam-cc EAX=unit_index/CX=slot_index) is called
//       with EAX still holding this unit's own datum_index (loaded a few instructions earlier
//       from player::unit) and CX loaded from unit_data::current_weapon_index (+0x2f2) --
//       i.e. "which object is in the readied slot of *this* unit's own inventory".
//     - unit_current_weapon_prevents_camo_depower (0x466390, blam-cc ECX=player_handle) is
//       called with ECX reloaded from EBX (this function's own player_handle parameter); its
//       EAX input in Ghidra's signature is not read by that function at all (see its header).
//   The three float globals are read straight out of bin/halo.exe: 0x672bac = 0.1,
//   0x672ac0 = 0.0, 0x672be8 = 0.05 (this last one matches Ghidra's own literal "0.05" compares).
//   Weapon+0x4cc is types/tags.h Weapon::active_camo_ding (Item base 0x308 bytes, then
//   weapon_flags, then a chain of fixed-size fields down to the pad ending at 0x4c0).
// register convention: player handle in EBX (unaff_EBX, the incoming argument no prologue code
//   initializes).
//   // blam-cc: EBX -> player_handle
// UNSURE: unit_data::unknown_37c / unknown_422 (types/units.h) are not named there yet; their
//   role as an active-camouflage countdown and a "value just changed" dirty flag is inferred
//   from this function's own behaviour, not from units.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern data_array *object_data;                  // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14

extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970
extern uint8_t unit_current_weapon_prevents_camo_depower(datum_index player_handle);      // 0x466390

// blam-cc: EBX -> player_handle
// Counts a unit's active-camouflage timer (unit_data+0x37c) down toward a 0.05 floor once per
// tick, at a rate that is 0.0 while the readied weapon prevents camo depower, the weapon tag's
// own active_camo_ding when it supplies a nonzero one, or 0.1 by default. Does nothing while no
// multiplayer engine is running, the player has no unit, or the timer is already at the floor.
void unit_update_active_camouflage_depower(datum_index player_handle)
{
    player *p;
    datum_index unit_handle;
    object *unit_obj;
    unit_data *unit;
    int16_t weapon_slot;
    datum_index weapon_handle;
    uint8_t prevents_depower;
    float rate;

    if (current_game_engine == 0 || player_handle == (datum_index)0xffffffff) {
        return;
    }

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    unit_handle = p->unit;
    if (unit_handle == (datum_index)0xffffffff) {
        return;
    }

    unit_obj = ((object_header *)object_data->data)[unit_handle & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    weapon_slot = unit->current_weapon_index;

    weapon_handle = unit_get_weapon_object_index(unit_handle, weapon_slot); // blam-cc: EAX, CX
    prevents_depower = unit_current_weapon_prevents_camo_depower(player_handle); // blam-cc: ECX

    rate = 0.1f; // 0x672bac
    if (prevents_depower) {
        rate = 0.0f; // 0x672ac0
    } else if (weapon_handle != (datum_index)0xffffffff) {
        object *weapon_obj = ((object_header *)object_data->data)[weapon_handle & 0xffff].data;
        Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
        if (weapon_tag->active_camo_ding != 0.0f) { // 0x672ac0
            rate = weapon_tag->active_camo_ding;
        }
    }

    if (unit->active_camouflage_power >= 0.05f) { // 0x672be8
        unit->active_camouflage_power = unit->active_camouflage_power - rate;
        unit->active_camouflage_regrowth = 1;
        if (unit->active_camouflage_power < 0.05f) { // 0x672be8
            unit->active_camouflage_power = 0.05f;   // 0x672be8
        }
    }
}

#if 0
Original Ghidra decompilation (0x466420), from tools/pack.py 0x466420:

void FUN_00466420(void)

{
  int iVar1;
  int iVar2;
  char cVar3;
  uint uVar4;
  uint unaff_EBX;
  float10 extraout_ST0;
  float10 fVar5;

  iVar2 = DAT_008603b0;
  if (((DAT_006f1d20 != 0) && (unaff_EBX != 0xffffffff)) &&
     (uVar4 = *(uint *)((unaff_EBX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34),
     uVar4 != 0xffffffff)) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
    uVar4 = unit_get_weapon_object_index();
    cVar3 = unit_current_weapon_prevents_camo_depower();
    if (cVar3 == '\0') {
      fVar5 = extraout_ST0;
      if ((uVar4 != 0xffffffff) &&
         (iVar2 = *(int *)((**(uint **)(*(int *)(iVar2 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) &
                           0xffff) * 0x20 + 0x14 + DAT_0087bc14), *(float *)(iVar2 + 0x4cc) != 0.0))
      {
        fVar5 = (float10)*(float *)(iVar2 + 0x4cc);
      }
    }
    else {
      fVar5 = (float10)0.0;
    }
    if (0.05 <= *(float *)(iVar1 + 0x37c)) {
      fVar5 = (float10)*(float *)(iVar1 + 0x37c) - fVar5;
      *(undefined2 *)(iVar1 + 0x422) = 1;
      *(float *)(iVar1 + 0x37c) = (float)fVar5;
      if (fVar5 < (float10)0.05) {
        fVar5 = (float10)0.05;
      }
      *(float *)(iVar1 + 0x37c) = (float)fVar5;
      return;
    }
  }
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x466420 --stop-address=0x466530):

00466420:  mov eax,ds:0x6f1d20
00466425:  test eax,eax
00466427:  je 0x46652f
0046642d:  cmp ebx,0xffffffff
00466430:  je 0x46652f
00466436:  mov ecx,ds:0x87a480
0046643c:  mov edx,[ecx+0x34]
0046643f:  mov eax,ebx
00466441:  and eax,0xffff
00466446:  shl eax,0x9
00466449:  add eax,edx
0046644b:  mov eax,[eax+0x34]
0046644e:  cmp eax,0xffffffff
00466451:  je 0x46652f
00466457:  mov ecx,eax
00466459:  and ecx,0xffff
0046645f:  push ebp
00466460:  mov ebp,ds:0x8603b0
00466466:  lea edx,[ecx+ecx*2]
00466469:  mov ecx,[ebp+0x34]
0046646c:  push esi
0046646d:  push edi
0046646e:  mov edi,[ecx+edx*4+0x8]
00466472:  mov cx,word ptr [edi+0x2f2]
00466479:  call 0x569970
0046647e:  fld dword ptr ds:0x672bac
00466484:  mov ecx,ebx
00466486:  mov esi,eax
00466488:  call 0x466390
0046648d:  test al,al
0046648f:  je 0x46649b
00466491:  fstp st(0)
00466493:  fld dword ptr ds:0x672ac0
00466499:  jmp 0x4664e1
0046649b:  cmp esi,0xffffffff
0046649e:  je 0x4664e1
004664a0:  mov eax,[ebp+0x34]
004664a3:  and esi,0xffff
004664a9:  lea edx,[esi+esi*2]
004664ac:  mov ecx,[eax+edx*4+0x8]
004664b0:  mov edx,[ecx]
004664b2:  mov eax,ds:0x87bc14
004664b7:  and edx,0xffff
004664bd:  shl edx,0x5
004664c0:  mov ecx,[edx+eax*1+0x14]
004664c4:  fld dword ptr [ecx+0x4cc]
004664ca:  fld dword ptr ds:0x672ac0
004664d0:  fucompp
004664d2:  fnstsw ax
004664d4:  test ah,0x44
004664d7:  jnp 0x4664e1
004664d9:  fstp st(0)
004664db:  fld dword ptr [ecx+0x4cc]
004664e1:  fld dword ptr [edi+0x37c]
004664e7:  fcomp dword ptr ds:0x672be8
004664ed:  fnstsw ax
004664ef:  test ah,0x1
004664f2:  jne 0x46652a
004664f4:  fsubr dword ptr [edi+0x37c]
004664fa:  mov word ptr [edi+0x422],0x1
00466503:  fst dword ptr [edi+0x37c]
00466509:  fld dword ptr ds:0x672be8
0046650f:  fcomp st(1)
00466511:  fnstsw ax
00466513:  test ah,0x41
00466516:  jne 0x466520
00466518:  fstp st(0)
0046651a:  fld dword ptr ds:0x672be8
00466520:  fstp dword ptr [edi+0x37c]
00466526:  pop edi
00466527:  pop esi
00466528:  pop ebp
00466529:  ret
0046652a:  pop edi
0046652b:  fstp st(0)
0046652d:  pop esi
0046652e:  pop ebp
0046652f:  ret
#endif
