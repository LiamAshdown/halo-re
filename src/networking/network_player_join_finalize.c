// network_player_join_finalize  (Ghidra: network_player_join_finalize, already named)
// address 0x4d9e30, size 156 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Creates the game-object datum for a
// player once their connection has fully joined, marking it as the local player when
// applicable"). `unaff_EDI` is a word-indexed client pointer (word 0x58a / byte 0xb14 is
// &client->session, matching the other word-indexed functions in this cluster); word 0x76d
// (byte 0xeda) is state; `in_EAX+0x1f` matches network_player_entry::slot_index.
// register convention: client in EDI (unaff_EDI); `in_EAX` (a network_player_entry*) is a
// second elided register argument, reconstructed as network_player_entry_add's own return value (types/
// networking.h documents 0x4de4e0 as the player-table "add" routine, called two lines above
// with exactly the session pointer this function derives from EDI).
// blam-cc: EDI -> client; EAX -> newly-added player entry (network_player_entry_add's own return value)
// UNSURE: `network_player_entry_validate` (the player_entry validator, per types/networking.h) is called here
// with literally no visible argument and its result captured only via Ghidra's `cVar2`; unlike
// network_session_player_table_index_apply.c's call to the same function, no player_entry
// pointer is in scope yet at this point in the body (network_player_entry_add has not run), so no argument
// is reconstructed for it here -- left as Ghidra shows it, flagged rather than guessed.
// UNSURE: `update_server_queue_create_entry`, `network_channel_key_open`, `datum_new_at_index_with_salt` and
// `game_set_local_player` are all called with no visible arguments; none are in this task's
// address range, so their signatures below are placeholders reflecting only that they are
// called, not what they take.
// UNSURE: `network_player_entry_add`'s success flag (Ghidra's `cVar2`, AL) and its entry pointer (`in_EAX`,
// the full register) are almost certainly the same call's bool-success-in-AL /
// pointer-still-live-in-EAX pair, the same elision pattern documented throughout this codebase.
// Modeled here as a single pointer-returning function where NULL means failure, which folds the
// two observations into one consistent value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern char network_player_entry_validate(void); // 0x4de9f0, UNSURE: no argument reconstructed; see file header
extern network_player_entry *network_player_entry_add(network_game_session *session); // 0x4de4e0
extern char network_channel_key_open(void); // 0x4de870, UNSURE argument; not in this batch
extern int32_t player_data_iterator_advance(int16_t step_count); // 0x4d98f0
extern void game_set_local_player(void); // 0x474d50, UNSURE argument
extern void datum_new_at_index_with_salt(void); // 0x4d03d0, UNSURE argument
extern void update_server_queue_create_entry(void); // 0x472c90, UNSURE argument; not in this batch

// blam-cc: EDI -> client
char network_player_join_finalize(network_client_globals *client)
{
    char ok;
    network_player_entry *entry;
    int8_t slot_index;
    uint16_t *client_words;

    ok = network_player_entry_validate(); // UNSURE argument; see file header
    if (ok == 0) {
        return 0;
    }

    entry = network_player_entry_add(&client->session);
    ok = entry != 0; // UNSURE: folds network_player_entry_add's own bool-success-in-AL; see file header
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
