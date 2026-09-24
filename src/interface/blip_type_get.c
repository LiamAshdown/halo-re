// blip_type_get  (Ghidra: blip_type_get, already named)
// address 0x4b3450, size 415 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b3450..0x4b35ee in the phase-4 review. The first rewrite
// passed player indices to teams_are_enemies (0x45bd50, CX team, DX team); the binary passes
// the team of the viewing player (player +0x20, as a short) and the owner team word at object
// +0xb8 (types/objects.h calls that field name_index; here it is compared as a team). Result:
// 5 without an object, 0 for an object owned by the viewing local player (0x474db0 maps the
// object to its player, stack argument), 2 for a non-unit object, for a vehicle 3 or 4 (enemy)
// by the team of its occupant at +0x328, else +0x324, else 4 for a "c_dropship" (Unit tag
// seats +0x2e4/+0x2e8, first seat label) and 3 otherwise, for any other unit 1 or 2 (enemy) by
// its own team against the team of the viewing player.
// register convention: EBX object; one stack argument (a short); result in AL.
//   // blam-cc: object -> EBX

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;          // 0x0087bc14

extern datum_index players_iterate_and_discard(datum_index object_index); // 0x474db0, UNSURE name: returns the player owning the object (stack argument)
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, blam-cc: CX team_a, DX team_b
extern datum_index local_player_to_player_index(int16_t local_player_index); // 0x474d30, blam-cc: AX

static player *blip_player(datum_index player_index)
{
    return (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200);
}

// blam-cc: object -> EBX
uint8_t blip_type_get(int16_t local_player_index, datum_index object_index)
{
    datum_index viewer = (local_player_index != -1 && local_player_index < 1)
                             ? local_player_globals->local_players[local_player_index] : (datum_index)-1;
    int32_t viewer_team = blip_player(viewer)->team; // read before the object test
    int32_t owner_local_index;
    uint8_t *object_data_ptr;

    if (object_index == (datum_index)-1) {
        return _blip_type_unavailable;
    }
    owner_local_index = players_iterate_and_discard(object_index) == (datum_index)-1
                            ? -1 : blip_player(players_iterate_and_discard(object_index))->local_player_index;
    if (owner_local_index == local_player_index) {
        return _blip_type_friendly;
    }
    if (object_try_and_get(object_index, 3) == 0) {
        return _blip_type_enemy_special;
    }
    object_data_ptr = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    if (object_try_and_get(object_index, 2) != 0) { // a vehicle
        datum_index occupant = *(datum_index *)(object_data_ptr + 0x328);

        if (occupant == (datum_index)-1) {
            occupant = *(datum_index *)(object_data_ptr + 0x324);
        }
        if (occupant != (datum_index)-1) {
            uint8_t *occupant_data = (uint8_t *)((object_header *)object_data->data)[occupant & 0xffff].data;
            return (uint8_t)((teams_are_enemies((int16_t)viewer_team, *(int16_t *)(occupant_data + 0xb8)) != 0) + 3);
        }
        {
            uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)object_data_ptr & 0xffff].data;
            if (*(int32_t *)(vehicle_tag + 0x2e4) > 1 &&
                strncmp(*(char **)(vehicle_tag + 0x2e8) + 4, "c_dropship", 10) == 0) { // 0x623bf0 strncmp
                return _blip_type_vehicle_special;
            }
        }
        return _blip_type_vehicle;
    }
    return (uint8_t)((teams_are_enemies((int16_t)blip_player(local_player_to_player_index(local_player_index))->team,
                                        *(int16_t *)(object_data_ptr + 0xb8)) != 0) + 1);
}

#if 0
Original Ghidra decompilation (0x4b3450):

char blip_type_get(int param_1)

{
  uint *puVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  uint unaff_EBX;

  iVar5 = DAT_0087a480;
  if (unaff_EBX == 0xffffffff) {
    return '\x05';
  }
  iVar3 = FUN_00474db0();
  if (iVar3 == -1) {
    iVar5 = -1;
  }
  else {
    uVar4 = FUN_00474db0();
    iVar5 = (int)*(short *)((uVar4 & 0xffff) * 0x200 + 2 + *(int *)(iVar5 + 0x34));
  }
  if (iVar5 == param_1) {
    return '\0';
  }
  iVar5 = object_try_and_get(3);
  if (iVar5 == 0) {
    return '\x02';
  }
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  iVar5 = object_try_and_get(2);
  if (iVar5 != 0) {
    if (puVar1[0xca] != 0xffffffff) {
      cVar2 = FUN_0045bd50();
      return (cVar2 != '\0') + '\x03';
    }
    if (puVar1[0xc9] == 0xffffffff) {
      iVar5 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((1 < *(int *)(iVar5 + 0x2e4)) &&
         (iVar5 = _strncmp((char *)(*(int *)(iVar5 + 0x2e8) + 4),"c_dropship",10), iVar5 == 0)) {
        return '\x04';
      }
      return '\x03';
    }
    cVar2 = FUN_0045bd50();
    return (cVar2 != '\0') + '\x03';
  }
  local_player_to_player_index();
  cVar2 = FUN_0045bd50();
  return (cVar2 != '\0') + '\x01';
}
#endif
