// hud_post_item_message  (Ghidra: FUN_004ae350, renamed in the phase-4 review)
// address 0x4ae350, size 166 bytes
// name confidence: 0.5 (chosen)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4ae350..0x4ae3f5 in the phase-4 review (the first rewrite
// had no arguments). In a local game (network_game_mode 0) it is hud_add_item_message
// (0x4ae400) with the register arguments passed through: EAX count becomes the stack count, the
// first stack argument becomes EAX (local player index). Otherwise it builds the 8 byte
// hud_item_message {ECX source, DL kind, AX count} and a two pointer item list {&payload, NULL},
// encodes message type 6 into the buffer at 0x00871de0 (EAX, EDX size 0x7ff8) and, when the
// machine of the second stack argument (a byte, network_machine_find_by_id with ESI network_server and EDI
// the id) exists, sends it to that machine (network_session_send_to_machine, EAX machine id,
// ESI network_server).
// register convention: EAX count, ECX source, DL kind; two stack arguments.
//   // blam-cc: count -> EAX, source -> ECX, kind -> DL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t network_game_mode;              // 0x00719720, 0 local, 1 client, 2 host
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0, UNSURE name: shared encode buffer

extern void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind,
                                 int16_t count); // 0x4ae400, blam-cc: EAX local_player_index, ECX source, BL source_kind
extern int32_t message_delta_encode_message(void *buffer, int32_t buffer_size, int32_t flag, int32_t message_type,
                                            int32_t changed_offset, void **items, int32_t type_offset,
                                            int32_t count, char force_changed); // 0x4ec940, blam-cc: EAX buffer, EDX buffer_size
extern network_machine *network_machine_find_by_id(network_server_globals *server, int16_t machine_id); // 0x4e0810, blam-cc: ESI server, EDI machine_id
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, int32_t unknown_0,
                                               void *data, int32_t bits, int32_t reliable, int32_t unknown_a,
                                               int32_t unknown_b, int32_t priority); // 0x4e1930, blam-cc: EAX machine_id, ESI server

// blam-cc: count -> EAX, source -> ECX, kind -> DL
// Posts an item pickup message for a local player, over the network when the game is networked.
void hud_post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index,
                           int8_t machine_id)
{
    hud_item_message payload;
    void *items[2];
    int32_t bits;
    network_machine *machine;

    if (network_game_mode == 0) {
        hud_add_item_message(local_player_index, source, kind, count);
        return;
    }
    payload.item_definition = source;
    payload.count = count;
    items[0] = &payload;
    payload.kind = kind;
    items[1] = 0;
    bits = message_delta_encode_message(network_message_scratch, 0x7ff8, 0, 6, 0, items, 0, 1, 0);
    if (bits <= 0) {
        return;
    }
    machine = network_machine_find_by_id(network_server, (int16_t)machine_id);
    if (machine != 0 && machine->machine_id != -1) {
        network_session_send_to_machine(machine->machine_id, network_server, 1, network_message_scratch, bits, 1, 0,
                                        1, 3);
    }
}

#if 0
Original Ghidra decompilation (0x4ae350):

void FUN_004ae350(void)

{
  int iVar1;
  int iVar2;
  undefined1 *local_10;
  undefined4 local_c;
  undefined1 local_8 [8];

  if (DAT_00719720 != 0) {
    local_10 = local_8;
    local_c = 0;
    iVar1 = message_delta_encode_message(0,6,0,&local_10,0,1,'\0');
    if (((0 < iVar1) && (iVar2 = FUN_004e0810(), iVar2 != 0)) && (*(short *)(iVar2 + 0xc) != -1)) {
      network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,1,3);
    }
    return;
  }
  FUN_004ae400();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
