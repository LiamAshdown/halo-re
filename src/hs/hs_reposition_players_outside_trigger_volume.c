// hs_reposition_players_outside_trigger_volume  (Ghidra: FUN_00487750)
// address 0x487750, size 203 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// REWRITTEN (objdump 0x487750..0x48781a; the draft took both arguments in registers and handed the unit index to
//   the containment test). cdecl: [esp+4] trigger volume, [esp+8] cutscene flag. For every player (datum_next,
//   then the same walk inlined) with a unit, scenario_trigger_volume_contains_point(EAX volume, ECX = the unit's
//   centre, object +0xa0); a unit outside is moved onto the flag by hs_object_detach_and_place_at_location
//   (AX flag, stack unit, 1, 1). The only caller is the volume_teleport_players_not_inside evaluator (0x47a459).
// blam-cc: stack -> trigger_volume_index, location_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, blam-cc: DX, EDI
extern uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point);
    // 0x53f020, blam-cc: EAX, ECX
extern void hs_object_detach_and_place_at_location(int16_t location_index, datum_index object_index,
    char detach_from_parent, char reorient); // 0x487f50, blam-cc: AX, stack

extern data_array *player_data; // 0x0087a480, stride 0x200
extern data_array *object_data; // 0x008603b0

void hs_reposition_players_outside_trigger_volume(int32_t trigger_volume_index, int32_t location_index)
{
    datum_index player_index = datum_next(-1, player_data);

    while (player_index != k_datum_index_none) {
        datum_index unit = *(datum_index *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200 + 0x34);

        if (unit != k_datum_index_none) {
            uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (unit & 0xffff) * 0xc + 8);

            if (!scenario_trigger_volume_contains_point((int16_t)trigger_volume_index, (real_point3d *)(object + 0xa0))) {
                hs_object_detach_and_place_at_location((int16_t)location_index, unit, 1, 1);
            }
        }
        player_index = datum_next((int16_t)player_index, player_data);
    }
}

#if 0
Original Ghidra decompilation (0x487750):

void FUN_00487750(void)

{
  char cVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;

  iVar7 = DAT_0087a480;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar5 = *(int *)(iVar7 + 0x34);
      iVar6 = (uVar2 & 0xffff) * 0x200;
      if ((*(int *)(iVar6 + 0x34 + iVar5) != -1) &&
         (cVar1 = scenario_trigger_volume_contains_point(), cVar1 == '\0')) {
        FUN_00487f50(*(undefined4 *)(iVar6 + iVar5 + 0x34),1,1);
        iVar7 = DAT_0087a480;
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
    } while ((sVar4 < 0) || (*(short *)(iVar7 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar7 + 0x22) + *(int *)(iVar7 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar7 + 0x22));
    } while ((short)iVar5 < *(short *)(iVar7 + 0x2e));
  } while( true );
}
#endif
