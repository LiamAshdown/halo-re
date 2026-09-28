// network_game_broadcast_player_set_changed  (Ghidra: FUN_004e1bf0; named per this rewrite)
// address 0x4e1bf0, size 103 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Posts a type-0x21 game-engine event and, if
// accepted, broadcasts an associated update packet to the whole session -- used whenever the
// connected-player set changes." 0x00871de0 is types/networking.h's shared encode scratch
// buffer, already named network_object_update_scratch by
// src/networking/network_server_check_machine_timeout.c (same address, "shared with
// network_game_broadcast_team_object_updates.c").
// register convention: ECX = server (implicit passthrough, following the same pattern
// established by network_game_broadcast_state_snapshot.c's disassembly-verified ESI -> server
// forwarding for the sibling message-0x17 broadcaster), stack = param_1.
//   // blam-cc: ECX -> server, stack -> param_1
// UNSURE: `param_1` is only ever used to compute `&(param_1 + 8)`, itself only used as an
// address-of-a-local passed to message_delta_encode_message; its true type and the meaning of
// the +8 adjustment are not recoverable from this function alone.
// UNSURE: message_delta_encode_message's own destination is not visible in this call (no
// output buffer argument); the subsequent broadcast reads network_object_update_scratch
// directly, so the encoder is presumed to write there through a fixed/global convention this
// function does not itself set up.
// UNSURE: `server` being implicit (rather than a genuine parameter) is inferred by analogy to
// network_game_broadcast_state_snapshot.c's disassembly-verified case, not independently
// re-checked here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_object_update_scratch[0x7ff8]; // 0x00871de0

extern void message_delta_parameters_protocol_send_update(void); // 0x4ebf50
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

// blam-cc: ECX -> server, stack -> param_1
// Posts a game-engine event (message-delta type 0x21) built from `param_1 + 8`; if the encoder
// reports a positive bit count, broadcasts the shared encode-scratch buffer to every
// established machine.
// FIXED 2026-09-28 (networking call audit, from the disassembly 0x4e1bf0..0x4e1c57): the only argument is the
// session on the stack (its +8 is the record to encode; the game/ callers already pass one argument); the
// broadcast's server (ECX) is the global network_server (0x71c2d4), not a parameter.
extern network_server_globals *network_server; // 0x0071c2d4
uint32_t network_game_broadcast_player_set_changed(uint8_t *param_1)
{
    int32_t encoded_bits;
    void *record;

    message_delta_parameters_protocol_send_update();
    record = param_1 + 8;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, &record, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_broadcast_to_all(network_server, 1, network_object_update_scratch, 1, 0, 1, 3);
    }
    return 0 < encoded_bits;
}

#if 0
Original Ghidra decompilation (0x4e1bf0), from tools/pack.py 0x4e1bf0:

bool FUN_004e1bf0(void *param_1)

{
  int iVar1;

  message_delta_parameters_protocol_send_update();
  param_1 = (void *)((int)param_1 + 8);
  iVar1 = message_delta_encode_message(0,0x21,0,&param_1,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
  }
  return 0 < iVar1;
}
#endif
