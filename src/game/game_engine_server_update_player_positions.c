// game_engine_server_update_player_positions  (Ghidra: FUN_00476760; named per this rewrite)
// address 0x476760, size 339 bytes (0x476760..0x4768b2; Ghidra's metadata said 231 and catalogued the
//   log-argument tail as the bogus function 0x476847 "player_add_equipment_unit_grenade_count_mod",
//   which is the `fstp QWORD PTR [esp+0x28]` of velocity.i for the history log below; argument order
//   re-checked against objdump 0x476831..0x47688c by orphan pass 4)
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Server-side per-tick routine that copies a moved
//   unit's position/velocity into its owning player's record and logs completion for local
//   players"); types/objects.h object (position +0x5c, velocity +0x68, parent_object +0x11c);
//   types/units.h unit_data::throttle (+0x278); network_player_update_history_log_write already
//   carries its real name via CEA/Chimera symbol match. player+0xf4/0xf8/0xfc/0x100 are within
//   this module's own documented unresolved player tail (0xf0..0x11f, "the network state the
//   client/server update code owns").
// register convention: no arguments.
// UNSURE: unit+0x4b8/0x4bc are a flag and a value this module does not otherwise attest to; kept
// as raw offsets.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data;      // 0x0087a480
extern data_array *object_data;      // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode;    // 0x00719720

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern unsigned long GetTickCount(void); // Win32
extern void network_player_update_history_log_write(const char *format, ...); // established name (CEA/Chimera)

// While hosting: for every player whose unit has its (UNSURE) +0x4b8 flag set, clears the flag,
// copies +0x4bc into player+0xf4, and copies the unit's parent object's (or its own, if
// unparented) position into player+0xf8/+0xfc/+0x100. For a non-local player, additionally logs
// a debug line with the current tick count, game time, the copied value, position and the
// unit's own velocity/throttle.
void game_engine_server_update_player_positions(void)
{
    data_iterator iter;
    player *plr;

    if (network_game_mode != 2) {
        return;
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->unit != (datum_index)-1) {
            object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
            if (*((uint8_t *)unit_obj + 0x4b8) == 1) {
                object *position_source;

                *((uint8_t *)unit_obj + 0x4b8) = 0;
                *(int32_t *)((uint8_t *)plr + 0xf4) = *(int32_t *)((uint8_t *)unit_obj + 0x4bc);

                position_source = unit_obj;
                if (unit_obj->parent_object != (datum_index)-1) {
                    position_source = ((object_header *)object_data->data)[unit_obj->parent_object & 0xffff].data;
                }
                *(float *)((uint8_t *)plr + 0xf8) = position_source->position.x;
                *(float *)((uint8_t *)plr + 0xfc) = position_source->position.y;
                *(float *)((uint8_t *)plr + 0x100) = position_source->position.z;

                if (plr->local_player_index == -1) {
                    int32_t value = *(int32_t *)((uint8_t *)plr + 0xf4);
                    float pos_x = *(float *)((uint8_t *)plr + 0xf8);
                    float pos_y = *(float *)((uint8_t *)plr + 0xfc);
                    float pos_z = *(float *)((uint8_t *)plr + 0x100);
                    unsigned long ticks = GetTickCount();
                    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

                    network_player_update_history_log_write(
                        "[%d]: [%d]:\t Completed [%d] ([%f] [%f] [%f]), ([%f] [%f]), ([%f] [%f])\n",
                        ticks, game_time->game_time, value, (double)pos_x, (double)pos_y, (double)pos_z,
                        (double)unit->throttle.i, (double)unit->throttle.j,
                        (double)unit_obj->velocity.i, (double)unit_obj->velocity.j);
                }
            }
        }
        plr = (player *)data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x476760), from tools/pack.py 0x476760:

void FUN_00476760(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  DWORD DVar4;
  int iVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  double dVar8;
  double dVar9;
  double dVar10;
  double dVar11;
  double dVar12;
  double dVar13;
  double dVar14;

  if (DAT_00719720 == 2) {
    iVar3 = data_iterator_next();
    iVar5 = DAT_008603b0;
    while (iVar3 != 0) {
      if ((*(uint *)(iVar3 + 0x34) != 0xffffffff) &&
         (iVar1 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc),
         *(char *)(iVar1 + 0x4b8) == '\x01')) {
        *(undefined1 *)(iVar1 + 0x4b8) = 0;
        *(undefined4 *)(iVar3 + 0xf4) = *(undefined4 *)(iVar1 + 0x4bc);
        iVar2 = iVar1;
        if (*(uint *)(iVar1 + 0x11c) != 0xffffffff) {
          iVar2 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc);
        }
        *(float *)(iVar3 + 0xf8) = *(float *)(iVar2 + 0x5c);
        *(undefined4 *)(iVar3 + 0xfc) = *(undefined4 *)(iVar2 + 0x60);
        *(undefined4 *)(iVar3 + 0x100) = *(undefined4 *)(iVar2 + 100);
        if (*(short *)(iVar3 + 2) == -1) {
          uVar6 = *(undefined4 *)(DAT_006f1d6c + 0xc);
          dVar14 = (double)*(float *)(iVar1 + 0x6c);
          dVar13 = (double)*(float *)(iVar1 + 0x68);
          dVar12 = (double)*(float *)(iVar1 + 0x27c);
          dVar11 = (double)*(float *)(iVar1 + 0x278);
          dVar10 = (double)*(float *)(iVar3 + 0x100);
          uVar7 = *(undefined4 *)(iVar3 + 0xf4);
          dVar9 = (double)*(float *)(iVar3 + 0xfc);
          dVar8 = (double)*(float *)(iVar3 + 0xf8);
          DVar4 = GetTickCount();
          network_player_update_history_log_write
                    ("[%d]: [%d]:\t Completed [%d] ([%f] [%f] [%f]), ([%f] [%f]), ([%f] [%f])\n",
                     DVar4,uVar6,uVar7,dVar8,dVar9,dVar10,dVar11,dVar12,dVar13,dVar14);
          iVar5 = DAT_008603b0;
        }
      }
      iVar3 = data_iterator_next();
    }
  }
  return;
}
#endif
