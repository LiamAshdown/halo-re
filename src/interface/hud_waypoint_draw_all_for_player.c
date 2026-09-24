// hud_waypoint_draw_all_for_player  (Ghidra: FUN_004aa5f0, renamed)
// address 0x4aa5f0, size 179 bytes
// name confidence: 0.45 (chosen)   rewrite confidence: 0.85
// evidence: rewritten in the phase-4 review from objdump -d 0x4aa5f0..0x4aa6a5. The loop walks
// player_data with an inline 0x10 byte iterator (data, index 0, none, data XOR 'iter', the same
// four dwords src/ai/actor_squad_action_execute.c builds), collects every other player on the
// local player team that drives a unit into a 16 entry stack array (the iterator +0x08 handle),
// then calls hud_waypoint_draw_one @0x4aa440 once per collected handle, in EAX. The earlier
// rewrite dropped the self exclusion, the handle array and the argument, drawing player 0 N
// times. The team is read before the -1 test, as in the binary (a read of slot 0xffff that is
// never used when there is no local player).
// register convention: no parameters; the local player comes from current_local_player_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern int16_t current_local_player_index; // 0x007c3108
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480

extern void hud_waypoint_draw_one(datum_index player_index); // 0x4aa440, blam-cc: EAX
extern void *data_iterator_next(data_iterator *iterator);    // 0x4d05d0, blam-cc: EDI

// Draws a waypoint over every teammate of the local player that currently drives a unit.
void hud_waypoint_draw_all_for_player(void)
{
    datum_index local_player;
    int32_t team;
    struct {
        data_array *data;
        int32_t next_index;
        datum_index index;
        uint32_t signature;
    } iterator;
    datum_index teammates[16];
    int32_t count = 0;
    int32_t i;
    player *entry;

    if (current_local_player_index == -1 || current_local_player_index >= 1) {
        local_player = (datum_index)-1;
    } else {
        local_player = local_player_globals->local_players[current_local_player_index];
    }
    team = ((player *)((uint8_t *)player_data->data + (local_player & 0xffff) * sizeof(player)))->team;
    if (local_player == (datum_index)-1) {
        return;
    }

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)player_data ^ 0x69746572;
    for (entry = (player *)data_iterator_next((data_iterator *)&iterator); entry != 0;
         entry = (player *)data_iterator_next((data_iterator *)&iterator)) {
        if (local_player != iterator.index && entry->team == team && entry->unit != (datum_index)-1) {
            teammates[count] = iterator.index;
            count++;
        }
    }

    for (i = 0; i < count; i++) {
        hud_waypoint_draw_one(teammates[i]);
    }
}

#if 0
Original Ghidra decompilation (0x4aa5f0):

void FUN_004aa5f0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined4 auStack_40 [16];

  if ((DAT_007c3108 == -1) || (0 < DAT_007c3108)) {
    uVar3 = 0xffffffff;
  }
  else {
    uVar3 = *(uint *)(DAT_0087a478 + 4 + DAT_007c3108 * 4);
  }
  iVar4 = *(int *)((uVar3 & 0xffff) * 0x200 + 0x20 + *(int *)(DAT_0087a480 + 0x34));
  if (uVar3 != 0xffffffff) {
    iVar2 = 0;
    iVar1 = data_iterator_next();
    while (iVar1 != 0) {
      if (((uVar3 != 0xffffffff) && (iVar4 == *(int *)(iVar1 + 0x20))) &&
         (*(int *)(iVar1 + 0x34) != -1)) {
        auStack_40[iVar2] = 0xffffffff;
        iVar2 = iVar2 + 1;
      }
      iVar1 = data_iterator_next();
    }
    iVar4 = 0;
    if (0 < iVar2) {
      do {
        FUN_004aa440();
        iVar4 = iVar4 + 1;
      } while (iVar4 < iVar2);
    }
  }
  return;
}
#endif
