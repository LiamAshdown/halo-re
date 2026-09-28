// network_client_drain_queued_updates  (Ghidra: FUN_004e1f40; named per this rewrite, matching
// the name src/networking/network_channel_dispatch_bitstream_unit.c's own extern already chose
// for this address)
// address 0x4e1f40, size 288 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.25)
// evidence: out/phase4/networking_functions.md: "Drains the client's queued network-game
// update messages, applying each one according to its message-type tag."
// register convention: EBX = machine (network_machine *, implicit passthrough -- needed only
// for the type-0xd case, which forwards it to network_game_client_apply_received_update per
// that file's own EBX -> machine convention), stack = param_1 (network_server_globals *,
// forwarded unchanged to the type-0x34 handler, which reads it as `server`), stack = param_2
// (opaque, forwarded unchanged to two foreign handlers).
//   // blam-cc: EBX -> machine, stack -> param_1, param_2
// TYPES-GAP CLOSED 2026-09-20 (review pass): the queued-message record message_delta_decode_array_field fills is
// types/networking.h message_delta_decode_state. The local typedef this file used to carry has
// been folded into the header, where it now also covers decode_context slot 0 in the
// 0x4e5390..0x4e6510 client update family. The join is message_delta_read_changed_subfields
// (0x4ed1d0), which takes the same record in EDI and reads message_type at +0x04 to index
// message_delta_definitions and the bit cursor at +0x10 -- the same +0x04 this function switches
// on. Ghidra split it into local_bc[28] + local_a0 + local_9f only because it could not prove
// they are one object.
// UNSURE (major): message_delta_decode_begin and message_delta_decode_array_field (both foreign, message-delta protocol) are
// called with zero visible arguments; FUN_004aabd0, FUN_00470810 and
// network_server_handle_rcon_request are likewise called with fewer arguments than a real
// implementation would need. All are declared and called exactly as Ghidra shows.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e1f40..0x4e2060, jump tables 0x4e2078 /
// 0x4e2060): the host twin of network_game_action_queue_drain. Arguments: the item's stream in ECX, then (server,
// machine) on the stack. A 0x34-byte decode state is begun on the stream (EAX state, EDI stream) and the decode
// context built (context[0] the state, [1..16] zero, [0x11] a 0x80-byte record); each decoded item of type 0xd
// (network_game_client_apply_received_update: EBX machine, stack server, context), 0xf (chat_server_relay_incoming_
// message: EAX context, stack machine), 0x1a (game_engine_update_lead_change_state: EAX context, stack machine),
// 0x34 (network_game_message_handle_ping_timestamp: EAX context, stack server) or 0x36
// (network_server_handle_rcon_request: EAX machine, EDX context) is applied; the run continues while both state
// flags +0x1c/+0x1d came back 1 and the count (+0x18) has not passed +0x08. Returns the last result.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream); // 0x4ec490, EAX, EDI
extern int32_t message_delta_decode_array_field(void **context); // 0x4ec510, EAX
extern void network_game_client_apply_received_update(network_machine *machine, uint32_t server, void **message); // 0x4e0280
extern void chat_server_relay_incoming_message(void **context, network_machine *machine); // 0x4aabd0
extern void game_engine_update_lead_change_state(void **envelope, uint8_t *message); // 0x470810
extern uint32_t network_game_message_handle_ping_timestamp(int32_t **message, network_server_globals *server); // 0x4e20b0
extern void network_server_handle_rcon_request(network_player_entry *client, void *message); // 0x4e4f00

char network_client_drain_queued_updates(network_server_globals *server, network_machine *machine, bit_stream *stream)
{
    union {
        message_delta_decode_state state;
        uint8_t bytes[0x34];
    } state;
    uint8_t record[0x80];
    void *context[0x12];
    char result;

    result = (char)message_delta_decode_begin(&state.state, stream);
    if (result != 1) {
        return result;
    }
    memset(&context[1], 0, 0x40);
    context[0] = &state;
    context[0x11] = record;
    state.bytes[0x1d] = 0;
    state.bytes[0x1c] = 0;
    for (;;) {
        uint8_t *current;

        if ((char)message_delta_decode_array_field(context) == 0) {
            return 0;
        }
        current = (uint8_t *)context[0];
        switch (*(int32_t *)(current + 4)) {
        case 0x0d: network_game_client_apply_received_update(machine, (uint32_t)server, context); break;
        case 0x0f: chat_server_relay_incoming_message(context, machine); break;
        case 0x1a: game_engine_update_lead_change_state(context, (uint8_t *)machine); break;
        case 0x34: network_game_message_handle_ping_timestamp((int32_t **)context, server); break;
        case 0x36: network_server_handle_rcon_request((network_player_entry *)machine, context); break;
        }
        result = current[0x1c] == 1 && current[0x1d] == 1;
        ++*(int32_t *)(current + 0x18);
        memset(&context[1], 0, 0x40);
        current[0x1c] = 0;
        current[0x1d] = 0;
        if (result != 1) {
            return result;
        }
        if (*(int32_t *)((uint8_t *)context[0] + 0x18) > *(int32_t *)((uint8_t *)context[0] + 0x08)) {
            return result;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e1f40), from tools/pack.py 0x4e1f40:

void FUN_004e1f40(undefined4 param_1,undefined4 param_2)

{
  bool bVar1;
  undefined1 *puVar2;
  char cVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined1 *local_108;
  undefined4 local_104 [16];
  undefined1 *local_c4;
  undefined1 local_bc [28];
  undefined1 local_a0;
  undefined1 local_9f;
  undefined1 local_88 [132];

  cVar3 = FUN_004ec490();
  if (cVar3 == '\x01') {
    puVar5 = local_104;
    for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    local_108 = local_bc;
    local_c4 = local_88;
    local_9f = 0;
    local_a0 = 0;
    do {
      cVar3 = FUN_004ec510();
      puVar2 = local_108;
      bVar1 = false;
      if (cVar3 != '\0') {
        switch(*(undefined4 *)(local_108 + 4)) {
        case 0xd:
          network_game_client_apply_received_update(param_1,&local_108);
          break;
        case 0xf:
          FUN_004aabd0(param_2);
          break;
        case 0x1a:
          FUN_00470810(param_2);
          break;
        case 0x34:
          FUN_004e20b0(param_1);
          break;
        case 0x36:
          network_server_handle_rcon_request();
        }
        if ((puVar2[0x1c] == '\x01') && (puVar2[0x1d] == '\x01')) {
          bVar1 = true;
        }
        else {
          bVar1 = false;
        }
        *(int *)(puVar2 + 0x18) = *(int *)(puVar2 + 0x18) + 1;
        puVar5 = local_104;
        for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar5 = 0;
          puVar5 = puVar5 + 1;
        }
        puVar2[0x1c] = 0;
        puVar2[0x1d] = 0;
      }
    } while ((bVar1) && (*(int *)(local_108 + 0x18) <= *(int *)(local_108 + 8)));
  }
  return;
}
#endif
