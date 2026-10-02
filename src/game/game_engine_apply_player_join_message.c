// game_engine_apply_player_join_message  (Ghidra: FUN_004778c0; renamed -- out/phase4/
// game_functions.md's own summary, "Validates a player action request and creates/updates a
// tracking datum for it, queuing the result for processing", undersells what the body actually
// does: it is the network handler that creates (or re-resolves) a player datum for an incoming
// join/team message and hands it to game_engine_player_new_life)
// address 0x4778c0, size 265 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x4778c0
//   --stop-address=0x4779d0): the `*(int*)*envelope` gate and the message_delta_decode_compound_field_staged rejection path
//   are the same "envelope" shape already established in
//   game_engine_client_apply_team_assignment.c (0x470a10); the closing writes to player+0x20/
//   +0x66/+0x68/+0x28/+0x24 match types/game.h player::team/team_index/kill_streak/
//   interaction_type/interaction_object exactly, and the trailing call is to the already-named
//   game_engine_player_new_life (0x45c440), which types/game.h documents as taking one player
//   handle argument -- confirming `local_c` (not the resolved pointer `iVar2`) is that handle.
//   `local_player_set_controlled_unit`'s sibling accessor local_player_to_player_index/
//   game_set_local_player (0x474d50) is the already-named function called when the new/found
//   player's player::local_player_index (+0x02) is not -1.
// register convention: a network message envelope in EAX (in_EAX, `**envelope == 0` gates
//   acceptance exactly like game_engine_client_apply_team_assignment's envelope).
//   // blam-cc: EAX -> envelope
// UNSURE: message_delta_decode_compound_field's out-parameter layout (three dwords, `join_key`/`hash_value`/`team`,
//   inferred purely from how each one is used afterward -- no header names this message);
//   network_channel_key_close's EAX argument (visible only in the disassembly, not in Ghidra's own
//   pseudocode) is the same per-machine "player identifier record" pointer types/game.h's
//   player-struct notes describe at player+0x48, computed here as
//   (network_server ? network_server+8 : network_client ? network_client+0xb14 : 0) +
//   join_key_slot*0x20 + 0x1a2, but that record's own field layout is not resolved past what
//   game.h already documents; network_index_cache_insert_if_free's real argument list (it is called here with more
//   live registers than its single established stack parameter, per
//   unit_network_create_update_apply.c); DAT_006f7ed0's identity as the data_array passed to
//   datum_new_at_index_with_salt (kept as a raw extern, distinct from player_data 0x0087a480);
//   game_engine_player_profile_cache_add's role (called immediately after the datum is created, with the new handle).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;         // 0x0087a480
extern uint8_t *network_server;        // 0x0071c2d4
extern uint8_t *network_client;         // 0x0071c2d8
extern data_array *update_client_queues; // 0x006f7ed0, UNSURE identity, see header note
extern uint8_t join_message_table[];    // 0x00687500, UNSURE identity (a .data table network_index_cache_insert_if_free
                                        //   receives in EAX at this one call site)

extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, rejection path, not in this batch; blam-cc:
    // EAX -> event, per the established signature in
    // game_engine_handle_kill_feed_network_event.c (0x4609d0)
extern uint8_t message_delta_decode_compound_field(void *event, void *out_message); // 0x4ec590, not in this batch;
    // blam-cc: EAX -> event, ECX -> out_message (a leading identifier-table slot byte, then the
    // 3 dwords: join_key, hash_value, team -- see the message layout below), per the same
    // established signature
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, memory module;
    // UNSURE exact signature, called here with only the handle visibly reused across sites
extern uint8_t network_channel_key_close(void *identifier_record); // 0x4de8c0, not in this batch;
    // blam-cc: EAX -> identifier_record (see header UNSURE note)
extern void network_index_cache_insert_if_free(uint32_t hash_value, datum_index player_handle,
    void *table); // 0x4e9cd0, not in this module. CORRECTED by review: objdump 0x477937..0x477945
    // shows "push [esp+0x18]" (hash_value), "mov ecx,[esp+0x14]" (join_key) and
    // "mov eax,0x687500" all live at the call. blam-cc: EAX -> table, ECX -> player_handle,
    // stack -> hash_value. UNSURE what the 0x00687500 table is.
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array); // 0x4d03d0
extern void game_engine_player_profile_cache_add(uint32_t player_handle); // 0x466c60, not in this batch
extern void game_set_local_player(datum_index player_handle,
    int16_t local_player_index); // 0x474d50, this module. CORRECTED by review: objdump
    // 0x477991 is "movsx si,BYTE PTR [ebp+0x1d]" -- a SIGNED byte out of the sender's
    // identifier record (EBP, the same 0x1a2-based pointer network_channel_key_close got) -- and 0x477996 is
    // "mov ecx,[esp+0x14]", the message's join_key. blam-cc: ECX -> player_handle,
    // SI -> local_player_index
extern void game_engine_player_new_life(uint32_t player_handle); // 0x45c440, this module

// blam-cc: EAX -> envelope
// Decodes an incoming player join/team message; if a player datum already exists for the
// message's join_key, reuses it, otherwise (once network_channel_key_close approves the sender's identifier
// record) allocates a fresh player datum at that key with datum_new_at_index_with_salt. Either
// way, stamps the message's team onto the player, promotes it to the local player if its
// local_player_index says so, resets its interaction/kill-streak state, and forwards it to
// game_engine_player_new_life.
void game_engine_apply_player_join_message(void **envelope)
{
    struct { uint8_t slot_index; uint8_t pad[3]; uint32_t join_key; uint32_t hash_value;
        uint32_t team; } message;
    player *p;

    if (*(int32_t *)*envelope != 0) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }
    if (!message_delta_decode_compound_field(envelope, &message)) {
        return;
    }

    // UNSURE: see header note -- the per-machine identifier-record table base is
    // network_server+8 when a session exists, else network_client+0xb14, else NULL. The
    // original computes this pointer (EBP) unconditionally at 0x477913, BEFORE the first
    // datum_get, and both the network_channel_key_close check and the game_set_local_player call below read
    // through it, so it is hoisted here rather than scoped to the not-found branch.
    {
    uint8_t *table_base = (network_server != 0) ? (network_server + 8) :
        ((network_client != 0) ? (network_client + 0xb14) : 0);
    uint8_t *identifier_record = table_base + (uint32_t)message.slot_index * 0x20 + 0x1a2;

    p = (player *)datum_get((datum_index)message.join_key, player_data);
    if (p == 0) {
        if (network_channel_key_close(identifier_record) != 1) {
            return;
        }
        network_index_cache_insert_if_free(message.hash_value, message.join_key, join_message_table);
        p = (player *)datum_get((datum_index)message.join_key, player_data);
        datum_new_at_index_with_salt((datum_index)message.join_key, update_client_queues);
        game_engine_player_profile_cache_add(message.join_key);
        if (p == 0) {
            return;
        }
    }

    p->team = (int32_t)message.team;
    p->team_index = (int8_t)message.team;
    if (p->local_player_index != -1) {
        // The local-player index comes from the SENDER'S identifier record, not from the
        // player datum: byte 0x1d, sign-extended.
        game_set_local_player((datum_index)message.join_key,
            (int16_t)*(int8_t *)(identifier_record + 0x1d));
    }
    p->kill_streak[0] = 0;
    p->kill_streak[1] = 0;
    p->interaction_type = 0;
    p->interaction_object = (datum_index)0xffffffff;

    game_engine_player_new_life(message.join_key);
    }
}

#if 0
Original Ghidra decompilation (0x4778c0), from tools/pack.py 0x4778c0:

void FUN_004778c0(void)

{
  char cVar1;
  undefined4 *in_EAX;
  int iVar2;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  cVar1 = FUN_004ec590();
  if (cVar1 == '\0') {
    return;
  }
  iVar2 = datum_get();
  if (iVar2 == 0) {
    cVar1 = FUN_004de8c0();
    if (cVar1 != '\x01') {
      return;
    }
    FUN_004e9cd0(local_8);
    iVar2 = datum_get();
    datum_new_at_index_with_salt();
    FUN_00466c60();
    if (iVar2 == 0) {
      return;
    }
  }
  *(undefined4 *)(iVar2 + 0x20) = local_4;
  *(undefined1 *)(iVar2 + 0x66) = (undefined1)local_4;
  if (*(short *)(iVar2 + 2) != -1) {
    game_set_local_player();
  }
  *(undefined4 *)(iVar2 + 0x68) = 0;
  *(undefined2 *)(iVar2 + 0x28) = 0;
  *(undefined4 *)(iVar2 + 0x24) = 0xffffffff;
  FUN_0045c440(local_c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
