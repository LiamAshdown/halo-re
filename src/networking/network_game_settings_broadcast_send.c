// network_game_settings_broadcast_send  (Ghidra: FUN_004df0e0; named per this rewrite)
// address 0x4df0e0, size 209 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Packages a 32-byte game-settings record
// together with the current tick, encodes and broadcasts it as message type 0x18, and records
// the send via network_object_record_last_sender." Follows the same data_packet_group_encode_packet /
// network_message_block_build / FUN_004e19c0 broadcast idiom as network_prepare_challenge_packet.c and
// network_game_server_host_dispose.c's challenge-packet send.
// UNSURE: data_packet_group_encode_packet's own full signature (src/memory) does not line up
// with the 4 arguments visible here either; kept as a local minimal prototype, matching the
// precedent in network_send_join_request_packet.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <stdint.h>
#include <string.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern game_time_globals *game_time; // 0x006f1d6c

extern int32_t data_packet_group_encode_packet(uint8_t *buffer, int32_t *capacity, int32_t packet_type, int32_t version); // 0x4d0ae0; UNSURE, see header
extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length); // 0x440350, this module
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0
extern void network_object_record_last_sender(uint32_t round); // 0x4df900, outside this batch

uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record)
{
    uint8_t buffer[0x604];
    int32_t capacity;
    int32_t tick_plus_offset;
    uint32_t *dest;
    int32_t i;
    int32_t send_result;

    dest = (uint32_t *)buffer;
    for (i = 0; i < 8; i++) {
        dest[i] = record[i];
    }
    tick_plus_offset = game_time->game_time + 0x21;
    (void)tick_plus_offset; // UNSURE: not clearly consumed further in the visible decompile
    capacity = 0x600;
    if (data_packet_group_encode_packet(buffer, &capacity, 0x18, 1) != 0) {
        // FIXED in the review pass: see 0x4df147 -- eax = 0x6b7f98, ecx = the encoded
        // buffer, dl = 3, and the encoded length is pushed.
        send_result = (int32_t)network_message_block_build(network_challenge_packet_block,
                                                           (uint32_t *)buffer, 3, (uint32_t)capacity);
        if (send_result != 0) {
            network_session_broadcast_to_all(network_server, 0, (void *)(uintptr_t)send_result,
                1, 0, 0, 3);
            network_object_record_last_sender(round);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4df0e0):

undefined4 FUN_004df0e0(undefined4 param_1,undefined4 *param_2)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_628;
  undefined4 local_624 [8];
  int local_604;

  puVar3 = local_624;
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *param_2;
    param_2 = param_2 + 1;
    puVar3 = puVar3 + 1;
  }
  local_604 = *(int *)(DAT_006f1d6c + 0xc) + 0x21;
  local_628 = 0x600;
  cVar1 = data_packet_group_encode_packet(local_624,&local_628,0x18,1);
  if (cVar1 != '\0') {
    iVar2 = FUN_00440350(local_628);
    if (iVar2 != 0) {
      FUN_004e19c0(0,iVar2,1,0,0,3);
      FUN_004df900(param_1);
      return 1;
    }
  }
  return 0;
}
#endif
