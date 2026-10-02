// network_game_client_decode_join_finalize_message  (Ghidra: FUN_004dc090; renamed, no prior
// name)
// address 0x4dc090, size 130 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes a join-handshake message (type
// 0xa) and forwards it to a secondary validation/processing routine, propagating its
// success/failure result"); the "secondary routine" is network_connection_finalize_join.c
// (0x4d9960, this task's batch), called with the client pointer.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    // 0x4d09d0, blam-cc: EAX -> remaining_length, stack -> group, decoded_body, buffer, out_type,
    //           out_version_used, expected_class
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t network_connection_finalize_join(uint16_t *connection); // 0x4d9960, this batch

// blam-cc: ESI -> client, stack -> buffer, length, sender_address
// FIXED (step 1, objdump -d 0x4dc090..0x4dc111): stack (buffer, length, sender address); another sender -> 1; not
// joining, or the (class 2) message does not decode -> 0; else network_connection_finalize_join's result.
int32_t network_game_client_decode_join_finalize_message(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const uint32_t *sender_address)
{
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address) {
        return 1;
    }
    if (client->state != k_network_client_state_joining) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) == 0) {
        return 0;
    }
    return (uint8_t)network_connection_finalize_join((uint16_t *)client);
}


#if 0
Original Ghidra decompilation (0x4dc090):

int FUN_004dc090(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int iVar2;
  ushort *unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if (local_18 != *param_3) {
    return 1;
  }
  if (unaff_ESI[0x76d] == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                       &param_3,2);
    if (cVar1 != '\0') {
      iVar2 = network_connection_finalize_join(unaff_ESI);
      return iVar2;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
