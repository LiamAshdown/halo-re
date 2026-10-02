// network_game_session_reset  (Ghidra: network_channel_table_initialize, already named --
// RENAMED here, see below)
// address 0x4de470, size 105 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: every field this function touches matches types/networking.h's network_game_session
// and network_player_entry exactly: the first zero-loop is 0xec dwords == 0x3b0 bytes == the
// whole session; the byte at +0x19d (set to 0x10/16) is maximum_players ("initialized to 16" per
// the header); the word at +0x1a0 (zeroed) is player_count; the dword at +0xeb dwords == byte
// +0x3ac is unknown_3ac ("copied from 0x0071c2c1" per the header, and DAT_0071c2c1 is exactly
// what this function reads); and the 16-entry, stride-0x20 loop starting at byte +0x1bf writes
// -1/0xff into player[i].machine_index/machine_player_index/unknown_1e/slot_index (all
// documented "0xff/free when unused"), 0xffff into color_index/unknown_1a ("0xffff when
// unused"), and a zero word at player[i].name[0] -- i.e. every one of the 16 network_player_entry
// rows reset to its documented empty state.
// RENAMED: a previous naming pass called this network_channel_table_initialize, evidently
// reading the per-player reset loop as a distinct "channel key table"; the field-by-field match
// above shows it is squarely a network_game_session reset (chiefly the player table), so this
// rewrite renames it. Both names describe the same 16-entry, stride-0x20 loop.
// register convention: session in EDX (in_EDX). blam-cc: EDX -> session

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_channel_table_default_flag; // 0x0071c2c1, UNSURE name; copied into
    // session->unknown_3ac by this function, matching the header's own note on that field

// blam-cc: EDX -> session
// Zeroes the whole session, then explicitly resets maximum_players to 16, player_count to 0,
// every player row to its documented empty state, and unknown_3ac from the global flag.
void network_game_session_reset(network_game_session *session)
{
    int32_t i;
    network_player_entry *player;

    memset(session, 0, sizeof(network_game_session));
    session->player_count = 0;
    for (i = 0; i < 16; i++) {
        player = &session->players[i];
        player->name[0] = 0;
        player->color_index = -1;
        player->icon_index = -1;
        player->machine_index = -1;
        player->machine_player_index = -1;
        player->team_index = -1;
        player->slot_index = -1;
    }
    session->maximum_players = 0x10;
    session->map_loaded = network_channel_table_default_flag != 0;
}

#if 0
Original Ghidra decompilation (0x4de470):

void network_channel_table_initialize(void)

{
  undefined1 *puVar1;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;
  bool bVar4;

  puVar3 = in_EDX;
  for (iVar2 = 0xec; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = in_EDX + 0x20;
  for (iVar2 = 0x21; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)(in_EDX + 0x68) = 0;
  puVar1 = (undefined1 *)((int)in_EDX + 0x1bf);
  iVar2 = 0x10;
  do {
    puVar1[-1] = 0xff;
    *puVar1 = 0xff;
    puVar1[1] = 0xff;
    puVar1[2] = 0xff;
    *(undefined2 *)(puVar1 + -0x1d) = 0;
    *(undefined2 *)(puVar1 + -5) = 0xffff;
    *(undefined2 *)(puVar1 + -3) = 0xffff;
    puVar1 = puVar1 + 0x20;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  bVar4 = DAT_0071c2c1 != '\0';
  *(undefined1 *)((int)in_EDX + 0x19d) = 0x10;
  *(bool *)(in_EDX + 0xeb) = bVar4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
