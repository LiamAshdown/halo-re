// network_game_client_decode_state_update_chunk  (Ghidra: FUN_004dc190; named per this rewrite)
// address 0x4dc190, size 172 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes one of several similarly-structured
// synchronization-phase data chunks and aborts the connection attempt via a shared cleanup
// routine on failure." On success it forwards to network_game_state_update_receive, whose own summary
// ("Processes an incoming sequenced game-state update packet, growing the per-connection
// reassembly buffer as needed and handing the payload off for application") gives this handler
// its name. Only accepts the message while client->state == 3.
// register/parameter convention (UNSURE, reconstructed -- see network_channel_remote_address_or_default.c's
// header for why Ghidra shows the shared guard call with no visible arguments):
//   blam-cc: EAX -> client (network_client_globals *, the global `network_client`)
//   stack: param_1 -> buffer (message bytes; the 2-byte packet header is skipped by the call to
//          data_packet_group_decode_packet below), param_2 -> UNSURE: reused as the
//          `int16_t *remaining_length` register argument data_packet_group_decode_packet needs
//          in EAX and which is otherwise dead in this function's own body, param_3 ->
//          expected_sequence (compared against the guard's local decode-result record).
// data_packet_group_decode_packet is called here with only 6 of its 7 stack arguments visible
// (see its own file's header on the general EAX-hidden-argument problem); the 7th,
// out_version_used, is supplied as a literal 0/NULL, matching the pattern data_packet_group
// itself uses at its own elided call site; the "input" byte_stream is a throwaway local scratch
// for the same reason (its address is what Ghidra shows here, but with no prior write to it).
// UNSURE: client->state is declared as padding in types/networking.h, but this whole cluster of
// functions (0x4dc190..0x4dc4b0) reads/writes it as a live join-handshake state discriminant
// (values observed: 2, 3, 4); the header's field name could not be changed here (do not edit
// types/*.h), so it is used as declared with this note.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6
extern int8_t network_game_state_update_receive(network_client_globals *client, void *decoded_body); // 0x4d9d20, elided
    // register args unresolved; the visible params are this function's own best-guess mapping
extern void network_disconnect_notify_dropped_machines(network_client_globals *client); // 0x4d9340, elided register args unresolved

// blam-cc: EAX -> client
// Rejects the message unless the caller's expected sequence matches the guard's local
// decode-result record and the connection is currently in state 3; otherwise reports the
// message as consumed (1) without decoding it. On a successful decode it hands the payload to
// network_game_state_update_receive; if that fails, or if the message was rejected up front by class/type, it runs
// the shared disconnect-notification cleanup (network_disconnect_notify_dropped_machines).
char network_game_client_decode_state_update_chunk(network_client_globals *client, uint8_t *param_1,
    int32_t param_2, int32_t *param_3)
{
    char result;
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint8_t decoded_body[528]; // opaque destination record

    result = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if ((sender.address.ipv4 != *param_3) || (client->state != 3)) {
        return 1;
    }
    if (data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
            decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 4) != 0) {
        result = network_game_state_update_receive(client, decoded_body);
        if (result != 0) {
            return result;
        }
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}

#if 0
Original Ghidra decompilation (0x4dc190):

char FUN_004dc190(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int in_EAX;
  char local_231;
  undefined1 local_230 [4];
  undefined1 local_22c [4];
  int local_228;
  undefined1 local_210 [528];

  local_231 = '\0';
  FUN_004dd390();
  if ((local_228 != *param_3) || (*(short *)(in_EAX + 0xeda) != 3)) {
    return '\x01';
  }
  cVar1 = data_packet_group_decode_packet
                    (&PTR_s_network_game_messages_group_006994f8,local_210,param_1 + 2,local_230,
                     local_22c,4);
  if ((cVar1 != '\0') && (local_231 = FUN_004d9d20(), local_231 != '\0')) {
    return local_231;
  }
  FUN_004d9340();
  return local_231;
}
#endif
