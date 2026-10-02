// game_engine_queue_status_sound_message  (Ghidra: FUN_0046bbd0; named per its summary)
// address 0x46bbd0, size 199 bytes
// VERIFIED against disassembly 0x46bbd0..0x46bc97 (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Requests/sends a multiplayer-sound status message
//   (event id 0x19) to all machines or a specific one, gated by flags from the sound-target
//   lookup"); message_delta_encode_message / network_session_broadcast_to_flagged / network_session_send_to_machine.
// register convention: sound index in EAX, recipient player handle in ECX (-1 = everybody).
//   // blam-cc: EAX -> sound_index, ECX -> recipient_player
// FIXED 2026-09-30 (disassembly): the draft took a single "machine index" and encoded an all-zero payload. The
// original encodes the SOUND INDEX (EAX, stored to a local dword at 0x46bbe4) as event 0x19, then for a specific player
// looks the machine up as player_data[handle & 0xffff] (stride 0x200) byte +0x64 (movsx) via
// network_machine_find_by_id(ESI = network_server, EDI = machine id), and sends with
// network_session_send_to_machine(EAX = machine id, ESI = server, ...).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, network_server_globals *server, int32_t status_bit,
    void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server,
    uint32_t status_bit, void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a,
    char force, uint32_t priority); // 0x4e1930, EAX machine_id, ESI server
extern network_machine *network_machine_find_by_id(network_server_globals *server, int32_t machine_id); // 0x4e0810, ESI, EDI

// blam-cc: EAX -> sound_index, ECX -> recipient_player
// Encodes the multiplayer-sound status event (0x19) carrying the sound index and either broadcasts it (recipient -1) or,
// when the recipient's machine record has flag bits 1 and 2 set, sends it to that machine only.
void game_engine_queue_status_sound_message(int32_t sound_index, datum_index recipient_player)
{
    int32_t payload = sound_index;
    void *items[2];
    int32_t encoded_bits;

    items[0] = &payload;
    items[1] = 0;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x19, 0, items, 0, 1, 0);
    if (encoded_bits > 0) {
        if (recipient_player == (datum_index)0xffffffff) {
            network_session_broadcast_to_flagged(encoded_bits, network_server, 1, network_message_scratch, 1, 0, 0, 3);
        } else {
            int32_t machine_id = (int8_t)*((uint8_t *)player_data->data + (recipient_player & 0xffff) * 0x200 + 0x64);
            network_machine *machine = network_machine_find_by_id(network_server, machine_id);

            if (machine != 0) {
                uint8_t flags = machine->flags;

                if ((flags >> 1 & 1) != 0 && (flags >> 2 & 1) != 0) {
                    network_session_send_to_machine(machine_id, network_server, 1, network_message_scratch, encoded_bits, 1, 0, 0, 3);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
