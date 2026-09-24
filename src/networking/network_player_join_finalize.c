// network_player_join_finalize  (Ghidra: network_player_join_finalize, already named)
// address 0x4d9e30, size 156 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Creates the game-object datum for a
// player once their connection has fully joined, marking it as the local player when
// applicable"). `unaff_EDI` is a word-indexed client pointer (word 0x58a / byte 0xb14 is
// &client->session, matching the other word-indexed functions in this cluster); word 0x76d
// (byte 0xeda) is state; `in_EAX+0x1f` matches network_player_entry::slot_index.
// register convention: client in EDI (unaff_EDI); entry (a network_player_entry*) in EAX
// (unaff_EAX). Confirmed by objdump: `mov esi,eax` at 0x4d9e32, before the first call, saves it
// across both the validate and add calls; the SAME (unmodified) value is read again at
// 0x4d9e62 (`movsx ecx,[esi+0x1f]`, entry->slot_index) after the add call returns -- so entry is
// a genuine caller-supplied pointer, not (as this file previously guessed)
// network_player_entry_add's own return value.
// blam-cc: EDI -> client, EAX -> entry
// FIXED (register inputs, objdump): EAX carries entry (read at 0x4d9e32, mov esi,eax); it was
// missing, and the body had synthesized `entry` from network_player_entry_add's return instead.
// objdump also shows network_player_entry_validate (0x4d9e36) and network_player_entry_add
// (0x4d9e4c) both take this same entry pointer via EAX (validate: EAX only; add: EAX -> entry,
// stack -> &client->session), and that network_player_entry_add itself only ever returns a
// plain bool in AL (never re-read as a pointer) -- resolving the two UNSURE notes below about
// those two calls' arguments.
// UNSURE: `update_server_queue_create_entry`, `network_channel_key_open`, `datum_new_at_index_with_salt` and
// `game_set_local_player` are all called with no visible arguments; none are in this task's
// address range, so their signatures below are placeholders reflecting only that they are
// called, not what they take. (network_channel_key_open in particular is called with a computed
// per-slot address, client + 0xcb6 + slot_index*0x20, not with no arguments -- out of scope for
// this pass since it does not involve the EAX register this fix addresses.)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, EAX -> entry
extern char network_player_entry_add(network_player_entry *entry, network_game_session *session); // 0x4de4e0, EAX -> entry, stack -> session
extern char network_channel_key_open(void); // 0x4de870, UNSURE argument; not in this batch
extern int32_t player_data_iterator_advance(int16_t step_count); // 0x4d98f0
extern void game_set_local_player(void); // 0x474d50, UNSURE argument
extern void datum_new_at_index_with_salt(void); // 0x4d03d0, UNSURE argument
extern void update_server_queue_create_entry(void); // 0x472c90, UNSURE argument; not in this batch

// blam-cc: EDI -> client, EAX -> entry
char network_player_join_finalize(network_client_globals *client, network_player_entry *entry)
{
    char ok;
    int8_t slot_index;
    uint16_t *client_words;

    ok = network_player_entry_validate(entry);
    if (ok == 0) {
        return 0;
    }

    ok = network_player_entry_add(entry, &client->session);
    if (ok != 0 && client->state == 3) { // UNSURE: live connection-mode value, not padding
        slot_index = entry->slot_index;
        ok = network_channel_key_open();
        if (ok == 0) {
            return 0;
        }

        player_data_iterator_advance(client->session.players[(uint8_t)slot_index].slot_index);

        client_words = (uint16_t *)client;
        if ((int8_t)client_words[(int32_t)slot_index * 0x10 + 0x669] == (uint32_t)*client_words) {
            game_set_local_player();
        }
        datum_new_at_index_with_salt();
        if (network_server != 0) {
            update_server_queue_create_entry();
        }
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x4d9e30):

char network_player_join_finalize(void)

{
  char cVar1;
  char cVar2;
  int in_EAX;
  ushort *unaff_EDI;

  cVar2 = FUN_004de9f0();
  if (cVar2 == '\0') {
    return '\0';
  }
  cVar2 = FUN_004de4e0(unaff_EDI + 0x58a);
  if ((cVar2 != '\0') && (unaff_EDI[0x76d] == 3)) {
    cVar1 = *(char *)(in_EAX + 0x1f);
    cVar2 = FUN_004de870();
    if (cVar2 == '\0') {
      return '\0';
    }
    FUN_004d98f0((int)*(char *)((int)unaff_EDI + cVar1 * 0x20 + 0xcd5));
    if ((int)(char)unaff_EDI[cVar1 * 0x10 + 0x669] == (uint)*unaff_EDI) {
      game_set_local_player();
    }
    datum_new_at_index_with_salt();
    if (DAT_0071c2d4 != 0) {
      FUN_00472c90();
    }
  }
  return cVar2;
}
#endif
