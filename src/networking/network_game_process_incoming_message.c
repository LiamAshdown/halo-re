// network_game_process_incoming_message  (Ghidra: network_game_process_incoming_message,
// already named)
// address 0x4e1c60, size 614 bytes
// name confidence: 0.7   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Top-level decoder/dispatcher for incoming
// 'network game' protocol messages, decoding each message with the network-game message group
// and routing it by type byte." The bitstream-header check (`(*record & 3) == 0 && ((*record
// >> 2) & 3) == 3`) matches network_game_message_decode_dispatch.c's identical check; `in_ECX`'s
// offset+0xe matches network_machine::flags/unknown_0f, and `param_1` is passed straight through
// to FUN_004e21d0 (this batch), whose own header confirms it as `server`.
// register convention: EAX = length, ECX = machine, EDX = record, stack = server. Verified case
// by case against the raw switch table below (Ghidra's own decompile groups two case labels
// under one call for two pairs -- `case 0x14: case 0x25:` both call FUN_004e26a0, `case 0x1a`
// alone calls FUN_004e2700 -- these are trusted over the low-confidence per-function summaries
// in networking_functions.md, three of which had their type numbers swapped; see the affected
// files' own UNSURE notes).
//   // blam-cc: EAX -> length, ECX -> machine, EDX -> record, stack -> server
// UNSURE (major): every case in the switch below that Ghidra shows calling its handler with no
// visible arguments is modelled the same way network_game_message_decode_dispatch.c models its
// own 18-handler switch: through a raw zero-argument function-pointer cast, matching Ghidra's
// literal CALL with whatever registers happen to already be live, rather than forcing each
// handler's independently-reconstructed (and non-uniform) parameter list onto this dispatcher.
// UNSURE: cases 1 and 0x1b decode and act inline rather than delegating to a named handler;
// FUN_004e2110 (this batch) is called with the address of the just-decoded record, which is the
// one case where a real argument is visible; FUN_004dff70 (network_game_client_apply_position_update,
// prior batch) is called with none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, UNSURE name; see other files' notes
extern data_packet_group network_game_messages_group; // 0x006994f8

extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, void *out_a, void *out_b, int32_t expected_class); // 0x4d09d0
extern uint32_t network_game_message_handle_keepalive(int32_t *record); // 0x4e2110, this batch
extern uint32_t network_game_server_handle_join_password(network_server_globals *server); // 0x4e21d0, case 0xe
extern uint32_t network_game_server_handle_join_confirm(void);    // 0x4e2400, this batch, case 0xf
extern uint32_t network_game_message_handle_settings_relay(void); // 0x4e24d0, this batch, case 0x10
extern uint32_t network_game_message_handle_player_count_broadcast(void); // 0x4e2530, case 0x11
extern uint32_t network_game_message_handle_player_entry_update(void);    // 0x4e2580, case 0x12
extern uint32_t network_game_message_handle_handshake_forward(void);      // 0x4e25e0, case 0x13
extern uint32_t network_game_message_handle_retry_schedule(void);         // 0x4e26a0, case 0x14/0x25
extern uint32_t network_game_message_handle_build_version(void);          // 0x4e2630, case 0x15
extern uint32_t network_game_server_handle_info_request(void);            // 0x4e2700, case 0x1a
extern void network_game_client_apply_position_update(void);              // 0x4dff70, case 0x1b, prior batch
extern uint32_t network_game_client_handle_map_data(network_server_globals *server); // 0x4e2790, case 0x1c
extern uint32_t network_game_client_handle_settings_relay(void);          // 0x4e2810, case 0x1d
extern uint32_t network_game_client_handle_retry_schedule(void);          // 0x4e2870, case 0x1e
extern uint32_t network_game_message_handle_settings_relay_role2(void);   // 0x4e28d0, case 0x23
extern uint32_t network_game_message_handle_join_finalize_ack_role2(void); // 0x4e2930, case 0x24

// blam-cc: EAX -> length, ECX -> machine, EDX -> record, stack -> server
uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine, uint16_t *record, network_server_globals *server)
{
    uint8_t type_byte;
    uint16_t machine_flags_word;
    uint8_t machine_flags;

    if ((*record & 3) != 0 || ((*record >> 2) & 3) != 3) {
        return 1;
    }

    type_byte = ((uint8_t *)record)[(int16_t)length - 1];
    machine_flags_word = *(uint16_t *)((uint8_t *)machine + 0xe);
    machine_flags = (uint8_t)machine_flags_word;

    if (((machine_flags >> 1) & 1) == 0 && type_byte != 0x0e &&
        !(((machine_flags >> 4) & 1) != 0 && type_byte == 1)) {
        return 1;
    }

    switch (type_byte) {
    case 1:
        if (network_disconnect_timeout_flag != 0) {
            int32_t decoded_value;
            int16_t out_a;

            if (data_packet_group_decode_packet(&network_game_messages_group, &decoded_value,
                                                 (uint8_t *)(record + 1), &out_a, &type_byte, 0) != 0) {
                network_game_message_handle_keepalive(&decoded_value);
                return 1;
            }
        }
        break;
    case 0x0e:
        return network_game_server_handle_join_password(server);
    case 0x0f:
        return network_game_server_handle_join_confirm();
    case 0x10:
        return network_game_message_handle_settings_relay();
    case 0x11:
        return network_game_message_handle_player_count_broadcast();
    case 0x12:
        return network_game_message_handle_player_entry_update();
    case 0x13:
        return network_game_message_handle_handshake_forward();
    case 0x14:
    case 0x25:
        return network_game_message_handle_retry_schedule();
    case 0x15:
        return network_game_message_handle_build_version();
    case 0x1a:
        return network_game_server_handle_info_request();
    case 0x1b:
        if (server->unknown_004 == 1) {
            uint8_t decoded_body[4];
            int32_t out_a;
            int16_t out_b;

            if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body,
                                                 (uint8_t *)(record + 1), &out_a, &out_b, 5) != 0) {
                network_game_client_apply_position_update();
            }
        }
        break;
    case 0x1c:
        return network_game_client_handle_map_data(server);
    case 0x1d:
        return network_game_client_handle_settings_relay();
    case 0x1e:
        return network_game_client_handle_retry_schedule();
    case 0x23:
        return network_game_message_handle_settings_relay_role2();
    case 0x24:
        return network_game_message_handle_join_finalize_ack_role2();
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e1c60), from tools/pack.py 0x4e1c60:

undefined4 network_game_process_incoming_message(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined4 uVar2;
  byte bVar3;
  int in_ECX;
  ushort *in_EDX;
  char local_38 [4];
  uint local_34;
  int local_30;
  int local_2c;
  undefined1 local_28 [40];

  if ((((*in_EDX & 3) == 0) && (bVar3 = (byte)*in_EDX >> 2 & 3, bVar3 != 1)) && (bVar3 == 3)) {
    local_38[0] = *(char *)((short)in_EAX + -1 + (int)in_EDX);
    local_34 = (uint)*(ushort *)(in_ECX + 0xe);
    bVar3 = (byte)*(ushort *)(in_ECX + 0xe);
    if ((((bVar3 >> 1 & 1) != 0) || (local_38[0] == '\x0e')) ||
       (((bVar3 >> 4 & 1) != 0 && (local_38[0] == '\x01')))) {
      switch(local_38[0]) {
      case '\x01':
        if (DAT_0071c2dc != '\0') {
          local_30 = in_EAX + -2;
          cVar1 = data_packet_group_decode_packet
                            (&PTR_s_network_game_messages_group_006994f8,&local_2c,in_EDX + 1,
                             &local_34,local_38,0);
          if (cVar1 != '\0') {
            FUN_004e2110(&local_2c);
            return 1;
          }
        }
        break;
      case '\x0e':
        uVar2 = FUN_004e21d0(param_1);
        return uVar2;
      case '\x0f':
        uVar2 = FUN_004e2400();
        return uVar2;
      case '\x10':
        uVar2 = FUN_004e24d0();
        return uVar2;
      case '\x11':
        uVar2 = FUN_004e2530();
        return uVar2;
      case '\x12':
        uVar2 = FUN_004e2580();
        return uVar2;
      case '\x13':
        uVar2 = FUN_004e25e0();
        return uVar2;
      case '\x14':
      case '%':
        uVar2 = FUN_004e26a0();
        return uVar2;
      case '\x15':
        uVar2 = FUN_004e2630();
        return uVar2;
      case '\x1a':
        uVar2 = FUN_004e2700();
        return uVar2;
      case '\x1b':
        if (*(short *)(param_1 + 4) == 1) {
          local_2c = in_EAX + -2;
          cVar1 = data_packet_group_decode_packet
                            (&PTR_s_network_game_messages_group_006994f8,local_28,in_EDX + 1,
                             local_38,&local_34,5);
          if (cVar1 != '\0') {
            FUN_004dff70();
          }
        }
        break;
      case '\x1c':
        uVar2 = FUN_004e2790(param_1);
        return uVar2;
      case '\x1d':
        uVar2 = FUN_004e2810();
        return uVar2;
      case '\x1e':
        uVar2 = FUN_004e2870();
        return uVar2;
      case '#':
        uVar2 = FUN_004e28d0();
        return uVar2;
      case '$':
        uVar2 = FUN_004e2930();
        return uVar2;
      }
    }
  }
  return 1;
}
#endif
