// network_game_broadcast_state_snapshot  (Ghidra: FUN_004e1b50; named per this rewrite)
// address 0x4e1b50, size 156 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Packages a small fixed-size game-state
// structure into message type 0x17 and broadcasts it to all established machines." Follows the
// exact same data_packet_group_encode_packet / network_message_block_build / broadcast idiom as
// src/networking/network_game_settings_broadcast_send.c (message type 0x18), confirmed here by
// disassembly (objdump -d -M intel, bin/halo.exe):
//   4e1b59: mov esi,eax                 ; ESI = record (EAX in), kept resident across the call
//   4e1b9e: mov eax,0x6b7f98            ; network_message_block_build(dest=network_challenge_packet_block,
//   4e1ba3: lea ecx,[esp+0x2c]          ;   buffer=encoded body, flags(dl)=3, length=edx)
//   4e1ba7: mov dl,0x3
//   4e1ba9: call 0x440350
//   4e1bc3: mov ecx,[esp+0x640]         ; reloads the CALLER's original ESI (saved by this
//                                       ; function's own `push esi` in its prologue, at
//                                       ; exactly this stack depth) as the `server` argument
//                                       ; to network_session_broadcast_to_all -- i.e. `server`
//                                       ; arrives in ESI, is never touched by this function's
//                                       ; body (ESI is repurposed for `record` instead), and is
//                                       ; recovered from the saved-register stack slot.
// register convention: EAX = record (const uint32_t[8] *), ESI = server (implicit passthrough,
// recovered from the prologue's own register-save slot, not read directly in the C body).
//   // blam-cc: EAX -> record, ESI -> server
// UNSURE: the broadcast's remaining fixed arguments (0, 1, 0, 1, 3) are transcribed literally
// from both Ghidra's decompile and the disassembly above; their individual meanings are not
// independently re-derived here (see network_session_broadcast_to_all.c's own UNSURE notes for
// what little is known about that parameter list).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, int32_t *capacity, int32_t packet_type, int32_t version); // 0x4d0ae0
extern uint16_t *network_message_block_build(uint16_t *dest, uint32_t *buffer, uint8_t flags, uint32_t length); // 0x440350, this module
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

// blam-cc: EAX -> record, ESI -> server
// Copies an 8-dword game-state record into a scratch buffer, encodes it as message type 0x17,
// and broadcasts the encoded block to every established machine in the session.
uint32_t network_game_broadcast_state_snapshot(const uint32_t *record, network_server_globals *server)
{
    uint32_t buffer[8];
    int32_t capacity;
    int32_t i;
    uint16_t *encoded;

    for (i = 0; i < 8; i = i + 1) {
        buffer[i] = record[i];
    }
    capacity = 0x600;
    if (data_packet_group_encode_packet((uint8_t *)buffer, &capacity, 0x17, 1) != 0) {
        encoded = network_message_block_build(network_challenge_packet_block, buffer, 3, (uint32_t)capacity);
        if (encoded != 0) {
            return network_session_broadcast_to_all(server, 0, encoded, 1, 0, 1, 3);
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e1b50), from tools/pack.py 0x4e1b50:

uint FUN_004e1b50(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_624;
  undefined4 local_620 [392];

  puVar3 = local_620;
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *in_EAX;
    in_EAX = in_EAX + 1;
    puVar3 = puVar3 + 1;
  }
  local_624 = 0x600;
  uVar1 = data_packet_group_encode_packet(local_620,&local_624,0x17,1);
  if ((char)uVar1 != '\0') {
    iVar2 = FUN_00440350(local_624);
    uVar1 = 0;
    if (iVar2 != 0) {
      uVar1 = FUN_004e19c0(0,iVar2,1,0,1,3);
      return uVar1;
    }
  }
  return uVar1 & 0xffffff00;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirming the register convention and the two
under-attributed calls:
  4e1b59: mov esi,eax                 ; ESI = record
  4e1b9e: mov eax,0x6b7f98
  4e1ba3: lea ecx,[esp+0x2c]
  4e1ba7: mov dl,0x3
  4e1ba9: call 0x440350               ; network_message_block_build(0x6b7f98, buffer, 3, length)
  4e1bc3: mov ecx,DWORD PTR [esp+0x640] ; server, from the saved-ESI prologue slot
  4e1bd2: call 0x4e19c0               ; network_session_broadcast_to_all(server, 0, block, 1, 0, 1, 3)
#endif
