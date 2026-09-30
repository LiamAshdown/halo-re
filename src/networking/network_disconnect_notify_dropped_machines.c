// network_disconnect_notify_dropped_machines  (Ghidra: FUN_004d9340; renamed, no prior name)
// address 0x4d9340, size 104 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Displays a disconnect-notification
// error message for each machine that dropped from the session"). client+0xee0 matches
// types/networking.h's network_client_globals::dropped_notice_shown exactly.
// register convention: the client pointer arrives in EBX (unaff_EBX). // blam-cc: EBX -> client
// UNSURE (major, preserved exactly): as decompiled, the while loop can run at most once --
// after the first `display_error` call, the loop-continuation test can only ever re-select -1
// (it requires `player_index < 0`, but player_index was just set to 0 to enter the loop body at
// all), so despite the summary's "for each machine" framing this cannot iterate over more than
// one machine as written. This is the same class of decompiler information-loss already flagged
// in player_data_iterator_advance.c (player_data_iterator_advance) -- almost certainly a lost per-iteration
// update to the real index variable -- and is transcribed literally rather than reconstructed,
// per the task's no-invented-behaviour rule.
// UNSURE: `DAT_00697e78` (a one-shot "already notified" style gate) and the table read through
// `DAT_0087a478 + 4` are not declared anywhere in types/networking.h; named/typed generically
// below. `DAT_0087a478` sits immediately before types/networking.h's `player_data`
// (0x0087a480, owned by the game/objects modules), so it is very likely a distinct, smaller
// table rather than a stray offset into player_data itself.
// UNSURE: `display_error`'s signature is reconstructed purely from this one call site's literal
// argument shapes (int, int, bool, bool); not cross-checked against any other caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern uint8_t network_disconnect_notice_shown; // 0x00697e78, UNSURE name
extern uint8_t local_player_globals[8]; // 0x0087a478, UNSURE name/size; only +4 is read here
extern void display_error(int32_t code, int32_t player_index, uint8_t flag_a, uint8_t flag_b); // 0x498f20, UNSURE signature

// blam-cc: EBX -> client
void network_disconnect_notify_dropped_machines(network_client_globals *client)
{
    int16_t player_index_16;
    int32_t player_index;
    int32_t next;

    if (network_disconnect_notice_shown != 0) {
        return;
    }
    if (client->dropped_notice_shown == 0) {
        player_index = -1;
        if (*(int32_t *)&local_player_globals[4] != -1) {
            player_index = 0;
        }
        player_index_16 = (int16_t)player_index;
        while (player_index_16 != -1) {
            display_error(8, player_index, 1, 0);
            next = -1;
            if (*(int32_t *)&local_player_globals[4] != -1 && (int16_t)player_index < 0) {
                next = 0;
            }
            player_index = next;
            player_index_16 = (int16_t)next;
        }
    }
    client->dropped_notice_shown = 1;
}

#if 0
Original Ghidra decompilation (0x4d9340):

void FUN_004d9340(void)

{
  short sVar1;
  int iVar2;
  int unaff_EBX;
  int player_index;

  if (DAT_00697e78 == '\0') {
    if (*(char *)(unaff_EBX + 0xee0) == '\0') {
      player_index = -1;
      if (*(int *)(DAT_0087a478 + 4) != -1) {
        player_index = 0;
      }
      sVar1 = (short)player_index;
      while (sVar1 != -1) {
        display_error(8,player_index,'\x01','\0');
        iVar2 = -1;
        if ((*(int *)(DAT_0087a478 + 4) != -1) && ((short)player_index < 0)) {
          iVar2 = 0;
        }
        player_index = iVar2;
        sVar1 = (short)iVar2;
      }
    }
    *(undefined1 *)(unaff_EBX + 0xee0) = 1;
  }
  return;
}
#endif
