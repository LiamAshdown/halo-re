// network_game_message_decode_replicated_command  (Ghidra: FUN_004dc410; named per this rewrite)
// address 0x4dc410, size 147 bytes
// name confidence: 0.25   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes an in-game message and, only when acting
// as host, forwards its two payload values to a follow-up handler -- consistent with the host
// applying a command replicated by a client." That description does not match the code as
// decoded: the follow-up call only runs when network_game_mode == 1, and types/networking.h
// documents mode 1 as "client", not host (0 local, 1 client, 2 host, 3 replay). Kept neutral in
// this rewrite's name pending resolution; the exact code below is preserved either way. On
// success it decodes an 8-byte payload (packet class 6, not 4 like this cluster's other
// handlers) and forwards its two dwords to network_client_timer_schedule ("Schedules a delayed network
// event/timer to fire after a given number of milliseconds").
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared EAX->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
// UNSURE: network_client_timer_schedule's own decompile (0x4d9ed0) shows an elided unaff_ESI it uses for
// client->unknown_ee4-region timer fields (see that function's own offsets, +0xee8/+0xef0/+0xee4/
// +0xeec/+0xef4, all inside types/networking.h's network_client_globals.unknown_ee4[11]); this
// call site shows only the two stack arguments Ghidra's own network_client_timer_schedule signature declares, so
// `client` is supplied for the elided ESI slot by the same reasoning applied throughout this
// batch.

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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode; // 0x00719720
extern data_packet_group network_game_messages_group; // 0x006994f8


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6
extern void network_client_timer_schedule(int32_t delay_ms, int32_t context, network_client_globals *client); // stack x2, ESI client // 0x4d9ed0, elided ESI

// blam-cc: EAX -> client
int32_t network_game_message_decode_replicated_command(network_client_globals *client, uint8_t *param_1,
    int32_t param_2, int32_t *param_3)
{
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[2];

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    if (client->state == 4 &&
        data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
            decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 6) != 0) {
        if (network_game_mode != 1) {
            return 1;
        }
        network_client_timer_schedule((int32_t)decoded_body[0], (int32_t)decoded_body[1], client);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dc410):

undefined4 FUN_004dc410(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int in_EAX;
  undefined1 local_24 [4];
  undefined4 local_20;
  undefined4 local_1c;
  int local_18;

  FUN_004dd390();
  if (local_18 != *param_3) {
    return 1;
  }
  if ((*(short *)(in_EAX + 0xeda) == 4) &&
     (cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,&local_20,param_1 + 2,local_24,
                         &param_3,6), cVar1 != '\0')) {
    if (DAT_00719720 != 1) {
      return 1;
    }
    FUN_004d9ed0(local_20,local_1c);
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
