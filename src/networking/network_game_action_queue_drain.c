// network_game_action_queue_drain  (Ghidra: FUN_004db870; renamed, no prior name)
// address 0x4db870, size 303 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.2 (LOW -- several unresolved data-flow gaps; see
// UNSURE notes))
// evidence: out/phase4/networking_functions.md summary ("Drains and applies a bounded run of
// queued network-game actions, one per iteration, via the action dispatcher"). Ghidra's own
// decompile carries a "Restarted to delay deadcode elimination for space: stack" warning.
// register convention: Ghidra's own recovered stack parameters (client, a bound value, the
// expected-sequence pointer). // blam-cc: stack -> client, bound, expected_sequence
// UNSURE (major): `local_98`/`local_97` (this file's `type_flag_a`/`type_flag_b`) are tested
// every loop iteration but never assigned anywhere in Ghidra's visible output except to 0 at the
// top of each iteration -- meaning, as decompiled, they can never be nonzero and the "success"
// branch of the loop body is unreachable. This is the same class of information loss already
// flagged in player_data_iterator_advance.c and network_disconnect_notify_dropped_machines.c
// (almost certainly set by message_delta_decode_array_field or network_game_action_apply through registers/an
// implicit output Ghidra did not track), transcribed literally rather than invented.
// UNSURE: `param_2` (Ghidra's second stack parameter) is never read anywhere in the visible
// body, yet `local_ac` (this file's `bound`) is compared against a running count with no visible
// initializer either. Reconstructed as `bound = param_2`, the only value in scope that matches
// the "bounded run" framing in the summary; `local_9c` (`applied_count`) is initialized to 0,
// also not shown explicitly.
// UNSURE: `network_channel_remote_address_or_default`, `message_delta_decode_array_field` and `network_game_action_apply` are
// all called with no visible arguments; reconstructed per the established conventions of this
// cluster (client + a fresh decode_result for the guard; a fresh scratch "action" record,
// mirroring network_game_action_apply's own established EAX->action_entry convention, for the
// other two).
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db870..0x4db99f): the arguments are
// the client, the item's bit stream and the sender address (not a bound and a sequence). The channel's remote
// address (client +0xadc, 0x4dd390) must match the sender; then a 0x34-byte decode state is begun on the stream
// (message_delta_decode_begin: EAX state, EDI stream) and the decode context is built: context[0] = the state,
// context[1..16] zero, context[0x11] = a zeroed 0x80-byte action record. Each message_delta_decode_array_field
// (EAX context) that succeeds is applied (network_game_action_apply: EAX context, ECX client); the state's +0x18
// counts them; the run continues only while both state flags +0x1c/+0x1d came back 1, and ends with 1 once the
// count passes the state's +0x08. Any other end notifies dropped machines (EBX client) and returns 0.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, EAX, ECX
extern int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream); // 0x4ec490, EAX, EDI
extern int32_t message_delta_decode_array_field(void **context); // 0x4ec510, EAX
extern void network_game_action_apply(void **context, network_client_globals *client); // 0x4da320, EAX, ECX
extern void network_disconnect_notify_dropped_machines(network_client_globals *client); // 0x4d9340, EBX

char network_game_action_queue_drain(network_client_globals *client, bit_stream *stream, const uint32_t *sender)
{
    network_resolved_address remote;
    union {
        message_delta_decode_state state;
        uint8_t bytes[0x34];
    } state;
    uint8_t record[0x80];
    void *context[0x12];
    char result = 0;

    network_channel_remote_address_or_default(client->channel, &remote);
    if (*(uint32_t *)&remote == *sender && (char)message_delta_decode_begin(&state.state, stream) != 0) {
        memset(record, 0, sizeof(record));
        memset(&context[1], 0, 0x40);
        context[0] = &state;
        context[0x11] = record;
        state.bytes[0x1d] = 0;
        state.bytes[0x1c] = 0;
        for (;;) {
            uint8_t *current;

            if ((char)message_delta_decode_array_field(context) == 0) {
                result = 0;
                break;
            }
            network_game_action_apply(context, client);
            current = (uint8_t *)context[0];
            result = current[0x1c] == 1 && current[0x1d] == 1;
            ++*(int32_t *)(current + 0x18);
            memset(&context[1], 0, 0x40);
            current[0x1c] = 0;
            current[0x1d] = 0;
            if (result != 1) {
                break;
            }
            if (*(int32_t *)(current + 0x18) > *(int32_t *)(current + 0x08)) {
                return result;
            }
        }
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}

#if 0
Original Ghidra decompilation (0x4db870):

/* WARNING: Restarted to delay deadcode elimination for space: stack */

char FUN_004db870(undefined4 param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  char local_119;
  undefined4 local_114 [16];
  undefined1 *local_d4;
  int local_cc;
  int local_ac;
  int local_9c;
  char local_98;
  char local_97;
  undefined1 local_80;
  undefined4 local_7f;

  FUN_004dd390();
  if ((local_cc == *param_3) && (cVar1 = FUN_004ec490(), cVar1 != '\0')) {
    local_80 = 0;
    puVar3 = &local_7f;
    for (iVar2 = 0x1f; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    *(undefined2 *)puVar3 = 0;
    *(undefined1 *)((int)puVar3 + 2) = 0;
    puVar3 = local_114;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    local_d4 = &local_80;
    while( true ) {
      local_97 = '\0';
      local_98 = '\0';
      cVar1 = FUN_004ec510();
      if (cVar1 == '\0') {
        local_119 = '\0';
        goto LAB_004db984;
      }
      FUN_004da320();
      if ((local_98 == '\x01') && (local_97 == '\x01')) {
        local_119 = '\x01';
      }
      else {
        local_119 = '\0';
      }
      local_9c = local_9c + 1;
      puVar3 = local_114;
      for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = 0;
        puVar3 = puVar3 + 1;
      }
      local_98 = '\0';
      local_97 = '\0';
      if (local_119 != '\x01') break;
      if (local_ac < local_9c) {
        return '\x01';
      }
    }
    if (local_119 != '\0') {
      return local_119;
    }
  }
  else {
    local_119 = '\0';
  }
LAB_004db984:
  FUN_004d9340();
  return local_119;
}
#endif
