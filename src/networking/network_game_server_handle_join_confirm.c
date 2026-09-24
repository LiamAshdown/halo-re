// network_game_server_handle_join_confirm  (Ghidra: FUN_004e2400; named per this rewrite)
// address 0x4e2400, size 200 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Handles message type 0xf, the next step of
// the join handshake after FUN_004e21d0, finalizing or retrying the connection." `in_ECX`'s
// offset+4 test matches network_server_globals::unknown_004 exactly as in
// network_game_server_handle_join_password.c; `in_EAX`'s offset+0xc matches
// network_machine::machine_id.
// register convention: EAX = machine (network_machine *, implicit), ECX = server
// (network_server_globals *, implicit), EDX = buffer (implicit) -- all three follow the same
// fully-implicit passthrough pattern this whole message-dispatch cluster uses (see
// network_game_process_incoming_message.c).
//   // blam-cc: EAX -> machine, ECX -> server, EDX -> buffer
// UNSURE (major): network_game_session_finalize_and_add_player, network_server_notify_or_resend_challenge
// and network_game_broadcast_player_set_changed are each called here with zero visible
// arguments in Ghidra's own decompile; declared and called with no arguments, matching Ghidra
// literally, rather than inventing plausible values for their established (non-empty)
// parameter lists documented in their own files.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern char network_game_session_finalize_and_add_player(void); // 0x4df840, this module,
    // called here with no visible arguments (UNSURE, see header)
extern void network_server_notify_or_resend_challenge(void); // 0x4e0af0, this module,
    // called here with no visible arguments (UNSURE, see header)
extern char network_game_broadcast_player_set_changed(void); // 0x4e1bf0, this module,
    // called here with no visible arguments (UNSURE, see header)
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0
extern char network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930

// blam-cc: EAX -> machine, ECX -> server, EDX -> buffer
// Decodes the join-confirm payload; on failure to finalize the player
// (network_game_session_finalize_and_add_player), notifies/resends the challenge instead.
// On success, broadcasts the updated player set and, if accepted, sends a final accept packet
// back to the joining machine.
uint32_t network_game_server_handle_join_confirm(network_machine *machine, network_server_globals *server, uint8_t *buffer)
{
    uint8_t decoded_body[32];
    int16_t out_a, out_b;

    if (server->unknown_004 != 0 && server->unknown_004 != 1) {
        return 1;
    }
    if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                         &out_a, &out_b, 3) == 0) {
        return 1;
    }
    if (network_game_session_finalize_and_add_player() == 0) {
        network_server_notify_or_resend_challenge();
    } else if (network_game_broadcast_player_set_changed() != 0) {
        uint32_t payload = 0;
        uint16_t *packet = network_prepare_challenge_packet(0, &payload); // UNSURE: message type inferred as 0 (local_24 zeroed before the call)
        if (packet != 0 && machine->machine_id != -1) {
            network_session_send_to_machine(0, packet, (int32_t)(*packet >> 4) << 3, 1, 0, 1, 3);
            return 1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2400), from tools/pack.py 0x4e2400:

undefined4 FUN_004e2400(void)

{
  char cVar1;
  int in_EAX;
  ushort *puVar2;
  int in_ECX;
  int in_EDX;
  undefined1 local_28 [4];
  undefined4 local_24;
  undefined1 local_20 [32];

  if ((*(short *)(in_ECX + 4) == 0) || (*(short *)(in_ECX + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,&local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      cVar1 = FUN_004df840();
      if (cVar1 == '\0') {
        FUN_004e0af0();
      }
      else {
        cVar1 = FUN_004e1bf0();
        if (cVar1 != '\0') {
          local_24 = 0;
          puVar2 = (ushort *)network_prepare_challenge_packet();
          if ((puVar2 != (ushort *)0x0) && (*(short *)(in_EAX + 0xc) != -1)) {
            network_session_send_to_machine(0,puVar2,(uint)(*puVar2 >> 4) << 3,1,0,1,3);
            return 1;
          }
        }
      }
    }
  }
  return 1;
}
#endif
