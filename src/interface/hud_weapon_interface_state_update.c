// hud_weapon_interface_state_update  (Ghidra: FUN_004b1740, named in phase 4)
// address 0x4b1740, size 557 bytes
// name confidence: 0.5 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b1740..0x4b196c in the phase-4 review. The first rewrite
// passed the weapon datum as the hud interface tag id, ran the no-weapon path for a seat whose
// Unit tag seat flags lack bit 3 (the binary skips it) and passed -1 instead of the HUDGlobals
// default weapon hud (+0x2cc). For each local player with a unit: the weapon is the unit
// current weapon, else the current weapon of the parent vehicle when the unit rides a seat with
// seat flags bit 3; with a weapon, its HUD ammo state (weapon_build_hud_ammo_state) and its
// Weapon tag hud_interface (+0x48c) go to hud_weapon_interface_meters_evaluate; without one,
// and only when unit_count_deployed_weapons (EAX unit) is 0, the default weapon hud is
// evaluated with a zeroed state. The weapon (or -1) is then cached per local player at
// hud_weapon_state + index * 0x28 + 0x20.
// register convention: none.

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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals;         // 0x0087a478
extern data_array *player_data;                      // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;                  // 0x0087bc14
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_weapon_interface_state *hud_weapon_state; // 0x00719430

extern void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out); // 0x4c29d0, blam-cc: EAX item_index
extern int16_t unit_count_deployed_weapons(datum_index unit_index); // 0x56d990, blam-cc: EAX
extern void hud_weapon_interface_meters_evaluate(datum_index hud_interface_tag_id, int16_t local_player_index,
                                                 int32_t weapon_or_vehicle_index, void *state_ptr); // 0x4b1970, blam-cc: EAX hud_interface_tag_id

void hud_weapon_interface_state_update(void)
{
    int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

    while (local_player_index != -1) {
        if (local_player_index >= 0 && local_player_index < 1 &&
            local_player_globals->local_players[local_player_index] != (datum_index)-1) {
            datum_index unit_index =
                ((player *)((uint8_t *)player_data->data +
                            (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;

            if (unit_index != (datum_index)-1) {
                uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
                int16_t slot = ((unit_object *)unit)->unit.current_weapon_index; // current_weapon_index
                datum_index weapon = slot != -1 ? *(datum_index *)(unit + 0x2f8 + slot * 4) : (datum_index)-1;
                uint8_t evaluate_default = 0;

                if (weapon == (datum_index)-1) {
                    datum_index parent = ((unit_object *)unit)->base.parent_object;
                    int16_t seat = ((unit_object *)unit)->unit.vehicle_seat_index;

                    if (parent == (datum_index)-1 || seat == -1) {
                        evaluate_default = 1;
                    } else {
                        uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
                        uint8_t *seats = *(uint8_t **)((uint8_t *)tag_instances[*(datum_index *)parent_object & 0xffff].data + 0x2e8);

                        if ((seats[seat * 0x11c] & 8) != 0) {
                            int16_t parent_slot = *(int16_t *)(parent_object + 0x2f2);
                            weapon = parent_slot != -1 ? *(datum_index *)(parent_object + 0x2f8 + parent_slot * 4)
                                                       : (datum_index)-1;
                            if (weapon == (datum_index)-1) {
                                evaluate_default = 1;
                            }
                        }
                    }
                }

                if (weapon != (datum_index)-1) {
                    uint8_t *weapon_object = (uint8_t *)((object_header *)object_data->data)[weapon & 0xffff].data;
                    uint8_t *weapon_tag = (uint8_t *)tag_instances[*(datum_index *)weapon_object & 0xffff].data;
                    weapon_hud_ammo_state ammo;

                    weapon_build_hud_ammo_state(weapon, &ammo);
                    if (*(datum_index *)(weapon_tag + 0x48c) != (datum_index)-1) { // Weapon hud_interface tag id
                        hud_weapon_interface_meters_evaluate(*(datum_index *)(weapon_tag + 0x48c), local_player_index,
                                                             weapon, &ammo);
                    }
                } else if (evaluate_default && unit_count_deployed_weapons(unit_index) == 0) {
                    weapon_hud_ammo_state ammo;

                    memset(&ammo, 0, sizeof(ammo));
                    hud_weapon_interface_meters_evaluate(*(datum_index *)((uint8_t *)hud_globals_tag_data + 0x2cc),
                                                         local_player_index, -1, &ammo); // default weapon hud
                }
                *(datum_index *)((uint8_t *)hud_weapon_state + local_player_index * 0x28 + 0x20) = weapon;
            }
        }
        local_player_index = (local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0)
                                 ? 0 : -1;
    }
}

#if 0
Original Ghidra decompilation (0x4b1740) -- correct only up to the first branch; every
register/loop detail below that point is reconstructed from objdump -d 0x4b1740..0x4b196c
(see the rewrite above):

void FUN_004b1740(void)

{
  int iVar1;
  short sVar2;
  short sVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined1 local_20 [32];

  iVar1 = DAT_008603b0;
  uVar4 = 0xffffffff;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    uVar4 = 0;
  }
  sVar2 = (short)uVar4;
  iVar6 = DAT_0087a478;
  do {
    if (sVar2 == -1) {
      return;
    }
    sVar2 = (short)uVar4;
    if ((((sVar2 != -1) && (sVar2 < 1)) &&
        (uVar7 = *(uint *)(iVar6 + 4 + sVar2 * 4), uVar7 != 0xffffffff)) &&
       (uVar7 = *(uint *)((uVar7 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
       uVar7 != 0xffffffff)) {
      iVar5 = (uVar7 & 0xffff) * 0xc;
      iVar6 = *(int *)(*(int *)(iVar1 + 0x34) + 8 + iVar5);
      sVar3 = *(short *)(iVar6 + 0x2f2);
      uVar7 = 0xffffffff;
      if (sVar3 != -1) {
        uVar7 = *(uint *)(iVar6 + 0x2f8 + sVar3 * 4);
      }
      if (uVar7 == 0xffffffff) {
        iVar6 = *(int *)(*(int *)(iVar1 + 0x34) + 8 + iVar5);
        if ((*(uint *)(iVar6 + 0x11c) != 0xffffffff) && (*(short *)(iVar6 + 0x2f0) != -1)) {
          if ((*(byte *)(*(short *)(iVar6 + 0x2f0) * 0x11c +
                        *(int *)(*(int *)((**(uint **)(*(int *)(iVar1 + 0x34) + 8 +
                                                      (*(uint *)(iVar6 + 0x11c) & 0xffff) * 0xc) &
                                          0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) & 8) == 0)
          goto LAB_004b1930;
          iVar6 = *(int *)(*(int *)(iVar1 + 0x34) + 8 + (*(uint *)(iVar6 + 0x11c) & 0xffff) * 0xc);
          sVar3 = *(short *)(iVar6 + 0x2f2);
          if (sVar3 != -1) {
            uVar7 = *(uint *)(iVar6 + 0x2f8 + sVar3 * 4);
          }
          if (uVar7 != 0xffffffff) goto LAB_004b188a;
        }
        sVar3 = FUN_0056d990();
        if (sVar3 == 0) {
          local_3c = 0;
          local_38 = 0;
          local_34 = 0;
          local_30 = 0;
          local_2c = 0;
          local_28 = 0;
          local_40 = 0;
          local_24 = 0;
          FUN_004b1970(uVar4,0xffffffff,&local_40);
        }
      }
      else {
LAB_004b188a:
        iVar6 = *(int *)((**(uint **)(*(int *)(iVar1 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc) & 0xffff)
                         * 0x20 + 0x14 + DAT_0087bc14);
        FUN_004c29d0(local_20);
        if (*(int *)(iVar6 + 0x48c) != -1) {
          FUN_004b1970(uVar4,uVar7,local_20);
        }
      }
LAB_004b1930:
      iVar6 = DAT_0087a478;
      *(uint *)(DAT_00719430 + 0x20 + sVar2 * 0x28) = uVar7;
    }
    uVar4 = 0xffffffff;
    if ((*(int *)(iVar6 + 4) != -1) && (sVar2 < 0)) {
      uVar4 = 0;
    }
    sVar2 = (short)uVar4;
  } while( true );
}

Disassembly (objdump, 0x4b1740..0x4b196c) that the rewrite above actually follows:

004b1740:
  8b 0d 78a48700       mov    ecx,ds:0x87a478             ; local_player_globals
  8b 51 04             mov    edx,[ecx+0x4]                 ; local_players[0]
  83 ec 44             sub    esp,0x44
  83 c8 ff             or     eax,0xffffffff
  83 fa ff             cmp    edx,0xffffffff
  74 02                je     0x4b1756
  33 c0                xor    eax,eax
  4b1756:
  66 3d ffff           cmp    ax,0xffff
  53                   push   ebx
  8b d8                mov    ebx,eax                        ; ebx = local_player_index (0 or -1)
  89 5c 24 04          mov    [esp+0x4],ebx
  0f 84 01020000       je     0x4b1968                          ; -1: nothing to do, return
  55                   push   ebp
  8b 2d b0038600       mov    ebp,ds:0x8603b0                    ; object_headers
  56                   push   esi
  57                   push   edi
  eb 04                jmp    0x4b1776
  4b1772:
  8b 5c 24 10          mov    ebx,[esp+0x10]
  4b1776:
  66 83 fb ff          cmp    bx,0xffff
  0f 84 c5010000       je     0x4b1945
  66 83 fb 01          cmp    bx,0x1
  0f 8d bb010000       jge    0x4b1945
  0f bf c3             movsx  eax,bx
  8b 44 81 04          mov    eax,[ecx+eax*4+0x4]                ; local_players[local_player_index]
  83 f8 ff             cmp    eax,0xffffffff
  0f 84 ab010000       je     0x4b1945
  8b 15 80a48700       mov    edx,ds:0x87a480                     ; player_data
  8b 52 34             mov    edx,[edx+0x34]                       ; player_data->data
  25 ffff0000          and    eax,0xffff
  c1 e0 09             shl    eax,0x9
  8b 44 10 34          mov    eax,[eax+edx*1+0x34]                  ; player->unit
  83 f8 ff             cmp    eax,0xffffffff
  0f 84 8d010000       je     0x4b1945
  8b 55 34             mov    edx,[ebp+0x34]                          ; object_headers->data
  8b c8                mov    ecx,eax
  81 e1 ffff0000       and    ecx,0xffff
  8d 0c 49             lea    ecx,[ecx+ecx*2]
  c1 e1 02             shl    ecx,0x2                                   ; ecx = unit_index*0xc
  8b 54 0a 08          mov    edx,[edx+ecx*1+0x8]                        ; edx = unit object pointer
  66 8b b2 f2020000    mov    si,[edx+0x2f2]                              ; unit->current_weapon_index
  83 cf ff             or     edi,0xffffffff
  66 83 fe ff          cmp    si,0xffff
  74 0a                je     0x4b17e7
  0f bf f6             movsx  esi,si
  8b bc b2 f8020000    mov    edi,[edx+esi*4+0x2f8]                       ; unit->weapons[slot]
  4b17e7:
  83 ff ff             cmp    edi,0xffffffff
  8b f7                mov    esi,edi
  0f 85 98000000       jne    0x4b188a                                     ; have a weapon: resolve its hud interface
  8b 7d 34             mov    edi,[ebp+0x34]
  8b 4c 0f 08          mov    ecx,[edi+ecx*1+0x8]                            ; reload unit object pointer
  8b 91 1c010000       mov    edx,[ecx+0x11c]                                  ; unit_object->parent_object
  3b d6                cmp    edx,esi
  0f 84 d7000000       je     0x4b18de                                          ; parent == -1: no vehicle, ask FUN_0056d990
  66 8b 99 f0020000    mov    bx,[ecx+0x2f0]                                      ; unit->vehicle_seat_index
  66 3b de             cmp    bx,si
  0f 84 c3000000       je     0x4b18da                                              ; seat == -1: not seated, no weapon
  81 e2 ffff0000       and    edx,0xffff
  8d 14 52             lea    edx,[edx+edx*2]
  8b 54 97 08          mov    edx,[edi+edx*4+0x8]                                    ; vehicle object pointer
  8b 12                mov    edx,[edx]                                                ; vehicle->definition_tag
  8b 3d 14bc8700       mov    edi,ds:0x87bc14                                          ; tag_instances
  81 e2 ffff0000       and    edx,0xffff
  c1 e2 05             shl    edx,0x5
  8b 54 3a 14          mov    edx,[edx+edi*1+0x14]                                       ; vehicle tag data
  8b 92 e8020000       mov    edx,[edx+0x2e8]                                              ; Unit::seats.pointer
  0f bf fb             movsx  edi,bx                                                        ; seat index
  69 ff 1c010000       imul   edi,edi,0x11c                                                  ; * sizeof(UnitSeat)
  f6 04 17 08          test   BYTE PTR [edi+edx*1],0x8                                        ; seat.flags & gunner
  0f 84 da000000       je     0x4b192c                                                          ; not a gunner seat: no weapon
  8b 89 1c010000       mov    ecx,[ecx+0x11c]                                                     ; unit_object->parent_object (again)
  8b 55 34             mov    edx,[ebp+0x34]
  81 e1 ffff0000       and    ecx,0xffff
  8d 0c 49             lea    ecx,[ecx+ecx*2]
  8b 4c 8a 08          mov    ecx,[edx+ecx*4+0x8]                                                   ; vehicle object pointer (again)
  66 8b 91 f2020000    mov    dx,[ecx+0x2f2]                                                          ; vehicle_unit->current_weapon_index
  0b f6                or     esi,esi
  66 83 fa ff          cmp    dx,0xffff
  74 0a                je     0x4b1881
  0f bf d2             movsx  edx,dx
  8b b4 91 f8020000    mov    esi,[ecx+edx*4+0x2f8]                                                    ; vehicle_unit->weapons[slot]
  4b1881:
  83 fe ff             cmp    esi,0xffffffff
  74 54                je     0x4b18da                                                                   ; no weapon: fall through
  8b 5c 24 10          mov    ebx,[esp+0x10]
  4b188a:
  8b 4d 34             mov    ecx,[ebp+0x34]
  8b c6                mov    eax,esi
  25 ffff0000          and    eax,0xffff
  8d 04 40             lea    eax,[eax+eax*2]
  8b 54 81 08          mov    edx,[ecx+eax*4+0x8]                                                          ; weapon object pointer
  8b 02                mov    eax,[edx]                                                                       ; weapon->definition_tag
  8b 0d 14bc8700       mov    ecx,ds:0x87bc14
  25 ffff0000          and    eax,0xffff
  c1 e0 05             shl    eax,0x5
  8b 7c 08 14          mov    edi,[eax+ecx*1+0x14]                                                             ; weapon tag data
  8d 54 24 34          lea    edx,[esp+0x34]
  52                   push   edx
  8b c6                mov    eax,esi                                                                            ; EAX = resolved weapon/vehicle index
  e8 15110100          call   0x4c29d0                                                                             ; FUN_004c29d0(eax, &state)
  8b 87 8c040000       mov    eax,[edi+0x48c]
  83 c4 04             add    esp,0x4
  83 f8 ff             cmp    eax,0xffffffff
  74 67                je     0x4b1930
  8d 4c 24 34          lea    ecx,[esp+0x34]
  51                   push   ecx
  56                   push   esi
  53                   push   ebx
  e8 9b000000          call   0x4b1970                                                                              ; hud_weapon_interface_meters_evaluate(esi, ebx, esi, &state)  [EAX still weapon tag id]
  83 c4 0c             add    esp,0xc
  eb 56                jmp    0x4b1930
  4b18da:
  8b 5c 24 10          mov    ebx,[esp+0x10]
  4b18de:
  e8 adc00b00          call   0x56d990                                                                                ; FUN_0056d990()
  66 85 c0             test   ax,ax
  75 48                jne    0x4b1930
  8b 0d 1c947100       mov    ecx,ds:0x71941c                                                                          ; hud_globals_tag_data
  33 d2                xor    edx,edx
  89 54 24 18          mov    [esp+0x18],edx
  89 54 24 1c          mov    [esp+0x1c],edx
  89 54 24 20          mov    [esp+0x20],edx
  89 54 24 24          mov    [esp+0x24],edx
  8d 44 24 14          lea    eax,[esp+0x14]
  50                   push   eax
  8b 81 cc020000       mov    eax,[ecx+0x2cc]                                                                           ; hud_globals_tag_data+0x2cc (EAX for the call)
  89 54 24 2c          mov    [esp+0x2c],edx
  89 54 24 30          mov    [esp+0x30],edx
  6a ff                push   0xffffffff
  53                   push   ebx
  c7 44 24 20 00000000 mov    [esp+0x20],0x0
  89 54 24 3c          mov    [esp+0x3c],edx
  e8 49000000          call   0x4b1970                                                                                    ; hud_weapon_interface_meters_evaluate(hud_globals+0x2cc, ebx, -1, &blank_state)
  83 c4 0c             add    esp,0xc
  eb 04                jmp    0x4b1930
  4b192c:
  8b 5c 24 10          mov    ebx,[esp+0x10]
  4b1930:
  8b 0d 78a48700       mov    ecx,ds:0x87a478
  0f bf c3             movsx  eax,bx
  8d 14 80             lea    edx,[eax+eax*4]
  a1 30947100          mov    eax,ds:0x719430                                                                              ; hud_weapon_state
  89 74 d0 20          mov    [eax+edx*8+0x20],esi                                                                          ; cache the resolved index
  4b1945:
  8b 51 04             mov    edx,[ecx+0x4]
  83 c8 ff             or     eax,0xffffffff
  83 fa ff             cmp    edx,0xffffffff
  74 07                je     0x4b1957
  66 85 db             test   bx,bx
  7d 02                jge    0x4b1957
  33 c0                xor    eax,eax
  4b1957:
  66 3d ffff           cmp    ax,0xffff
  89 44 24 10          mov    [esp+0x10],eax
  0f 85 0dfeffff       jne    0x4b1772
  5f                   pop    edi
  5e                   pop    esi
  5d                   pop    ebp
  5b                   pop    ebx
  83 c4 44             add    esp,0x44
  c3                   ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
