// network_host_send_scenario_announcement  (Ghidra: FUN_004df1c0; named per this rewrite)
// address 0x4df1c0, size 204 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Sends the one-time scenario/challenge
// announcement packets (via FUN_004ec940/FUN_004e19c0 and network_prepare_challenge_packet) the first time it is
// called for this game, then latches a done flag." host->unknown_9f9/unknown_9b8 match
// network_game_server_host_new.c's established offsets on network_server_globals.
// UNSURE: message_delta_encode_message's parameter shapes are inferred purely from this call
// site; declared generically.
// register convention: host in ESI (unaff_ESI). blam-cc: ESI -> host

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *message_delta_definition_table; // 0x00871de0, UNSURE identity, see
    // update_server_send_update.c
extern network_server_globals *network_server; // 0x0071c2d4

extern void message_delta_parameters_protocol_send_update(void); // 0x4ebf50, outside this batch
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module

// blam-cc: ESI -> host
int32_t network_host_send_scenario_announcement(network_server_globals *host)
{
    int32_t result;
    void *payload;
    int32_t encode_result;
    int32_t challenge_packet;
    uint32_t challenge_payload[4]; // UNSURE: the caller-frame scratch EDX points at; its
                                   //         contents are not visible in the decompilation

    result = 1;
    if (host->unknown_9f9 == 0) {
        message_delta_parameters_protocol_send_update();
        payload = (uint8_t *)host + 8;
        encode_result = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, &payload, 0, 1, 0);
        if (encode_result > 0) {
            network_session_broadcast_to_all(network_server, 1, &message_delta_definition_table, 1, 0, 1, 3);
        }
        result = encode_result > 0;
        if (encode_result > 0) {
            // 0x4df240: eax = 0x0a; 0x4df23c: edx = the staged announcement scratch.
            challenge_packet = (int32_t)network_prepare_challenge_packet(0x0a, challenge_payload);
            if (challenge_packet != 0) {
                if (network_session_broadcast_to_all(network_server, 0, (void *)(uint32_t)challenge_packet, 1, 0, 1, 3) != 0) {
                    host->unknown_9f9 = 1;
                    result = 1;
                }
            }
        }
    }
    host->unknown_9b8 = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x4df1c0):

bool FUN_004df1c0(void)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  int unaff_ESI;
  void *local_c;
  undefined4 local_8;
  undefined4 local_4;

  bVar1 = true;
  if (*(char *)(unaff_ESI + 0x9f9) == '\0') {
    local_8 = 0;
    message_delta_parameters_protocol_send_update();
    local_c = (void *)(unaff_ESI + 8);
    local_4 = 0;
    iVar3 = message_delta_encode_message(0,0x21,0,&local_c,0,1,'\0');
    if (0 < iVar3) {
      FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
    }
    bVar1 = 0 < iVar3;
    if (0 < iVar3) {
      iVar3 = network_prepare_challenge_packet();
      if (iVar3 != 0) {
        cVar2 = FUN_004e19c0(0,iVar3,1,0,1,3);
        if (cVar2 != '\0') {
          *(undefined1 *)(unaff_ESI + 0x9f9) = 1;
          bVar1 = true;
        }
      }
    }
  }
  *(undefined4 *)(unaff_ESI + 0x9b8) = 0;
  return bVar1;
}
#endif
