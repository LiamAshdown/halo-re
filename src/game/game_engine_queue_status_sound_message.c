// game_engine_queue_status_sound_message  (Ghidra: FUN_0046bbd0; named per its summary)
// address 0x46bbd0, size 199 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Requests/sends a multiplayer-sound status message
//   (event id 0x19) to all machines or a specific one, gated by flags from the sound-target
//   lookup"); message_delta_encode_message / network_session_broadcast_to_flagged / network_session_send_to_machine
//   already established call shapes elsewhere in this module.
// register convention: machine index in in_ECX.
//   // blam-cc: ECX -> machine_index
// UNSURE: network_machine_find_by_id's exact signature/identity (a per-machine network-session record lookup,
//   judging by the flags word read at its result + 0xe).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t shared_hud_text_draw_state; // 0x00871de0
extern uint8_t *network_session;           // 0x0071c2d4

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930
extern void *network_machine_find_by_id(int32_t machine_index); // 0x4e0810, UNSURE exact signature; a
    // per-machine network-session record lookup

// blam-cc: ECX -> machine_index
// Encodes an empty event-0x19 status message and, if the encoder produced a positive bit
// length, either broadcasts it (machine_index == -1) or, when the target machine's record shows
// both bit 1 and bit 2 of its flags word (offset 0xe) set, sends it directly to that machine.
void game_engine_queue_status_sound_message(int32_t machine_index)
{
    uint8_t payload[4] = {0, 0, 0, 0};
    uint8_t *payload_ptr = payload;
    int32_t encoded_bits;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x19, 0, (void **)&payload_ptr, 0, 1, 0);
    if (encoded_bits > 0) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(network_server_pointer, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
        } else {
            void *machine = network_machine_find_by_id(machine_index);
            if (machine != (void *)0) {
                uint8_t flags = (uint8_t)*(int16_t *)((uint8_t *)machine + 0xe);
                if ((flags >> 1 & 1) != 0 && (flags >> 2 & 1) != 0) {
                    network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x46bbd0), from tools/pack.py 0x46bbd0:

void FUN_0046bbd0(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int in_ECX;
  undefined1 local_c [4];
  undefined1 *local_8;
  undefined4 local_4;

  local_8 = local_c;
  local_4 = 0;
  iVar2 = message_delta_encode_message(0,0x19,0,&local_8,0,1,'\0');
  if (0 < iVar2) {
    if (in_ECX == -1) {
      FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
      return;
    }
    iVar3 = FUN_004e0810();
    if (((iVar3 != 0) && (bVar1 = (byte)*(undefined2 *)(iVar3 + 0xe), (bVar1 >> 1 & 1) != 0)) &&
       ((bVar1 >> 2 & 1) != 0)) {
      network_session_send_to_machine(1,&DAT_00871de0,iVar2,1,0,0,3);
    }
  }
  return;
}
#endif
