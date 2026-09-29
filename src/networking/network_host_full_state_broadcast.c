// network_host_full_state_broadcast  (Ghidra: FUN_004df510; named per this rewrite)
// address 0x4df510, size 302 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "The first time it runs for a round, sends a
// type-0x21 packet to every channel entry flagged as needing a full state refresh, staggering
// each send's embedded timestamp by 100ms." host->unknown_a0e (the "run once" latch, cleared
// here) and host->game_over (+0xa0f, cleared here) match types/networking.h; the machines[]
// iteration (stride 0x60, byte offset +0x3c6 == machines[0]+0xe == flags) matches
// network_game_client_game_settings_updated.c's own machines[] loop over the same field.
// UNSURE: bit4 (0x10) of machine->flags is not named in network_machine_flags; kept as a literal
// bit test. data_packet_group_encode_packet is called with a minimal, call-site-local prototype
// (see network_send_join_request_packet.c's precedent), not the fuller src/memory
// reconstruction.
// register convention: fully recovered cdecl (host is the only parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern uint16_t network_challenge_packet_block; // 0x006b7f98, UNSURE identity: packed header word
extern uint8_t network_broadcast_body[1536]; // 0x006b7f9a, UNSURE identity/size: encoded body buffer

extern int32_t data_packet_group_encode_packet(uint8_t *buffer, int32_t *capacity, int32_t packet_type, int32_t version); // 0x4d0ae0; UNSURE, see header
extern char network_session_send_to_machine(int32_t a, void *packet, int32_t byte_count, int32_t b,
    int32_t c, int32_t d, int32_t e); // 0x4e1930, outside this batch

void network_host_full_state_broadcast(network_server_globals *host)
{
    int32_t timestamp;
    int32_t i;
    network_machine *machine;
    uint8_t encode_buffer[0x600];
    int32_t capacity;
    int32_t tick;
    char encode_ok;
    uint32_t byte_count;

    if (host->full_state_broadcast_pending == 1) {
        host->full_state_broadcast_pending = 0;
        host->game_over = 0;
        timestamp = 1000;
        for (i = 0; i < 16; i++) {
            machine = &host->machines[i];
            if ((machine->flags & 2) != 0 || (machine->flags & 0x10) != 0) {
                tick = timestamp;
                capacity = 0x600;
                encode_ok = (char)data_packet_group_encode_packet(encode_buffer, &capacity, 0x21, 1);
                (void)tick;
                if (encode_ok != 0) {
                    network_challenge_packet_block = ((int16_t)capacity + 2) * 0x10 | 0xc;
                    memcpy(network_broadcast_body, encode_buffer, (uint32_t)capacity & 0xffff);
                    byte_count = (uint32_t)(network_challenge_packet_block >> 4) << 3;
                    if (network_session_send_to_machine(0, &network_challenge_packet_block, byte_count, 1, 0, 0, 3) != 0) {
                        timestamp = timestamp + 100;
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4df510):

/* WARNING: Type propagation algorithm not settling */

void FUN_004df510(int param_1)

{
  char cVar1;
  uint uVar2;
  undefined2 *puVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int local_614;
  int local_610;
  uint local_60c [3];
  undefined4 local_600 [384];

  if (*(char *)(param_1 + 0xa0e) == '\x01') {
    *(undefined1 *)(param_1 + 0xa0e) = 0;
    *(undefined1 *)(param_1 + 0xa0f) = 0;
    local_614 = 1000;
    puVar3 = (undefined2 *)(param_1 + 0x3c6);
    local_610 = 0x10;
    do {
      if ((((byte)*puVar3 >> 1 & 1) != 0) || (((byte)*puVar3 >> 4 & 1) != 0)) {
        local_60c[1] = 0;
        local_60c[2] = local_614;
        local_60c[0] = 0x600;
        cVar1 = data_packet_group_encode_packet(local_60c + 1,local_60c,0x21,1);
        if (cVar1 != '\0') {
          DAT_006b7f98 = ((short)local_60c[0] + 2) * 0x10 | 0xc;
          puVar4 = local_600;
          puVar5 = &DAT_006b7f9a;
          for (uVar2 = (local_60c[0] & 0xffff) >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
            *puVar5 = *puVar4;
            puVar4 = puVar4 + 1;
            puVar5 = puVar5 + 1;
          }
          for (uVar2 = local_60c[0] & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
            *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
            puVar4 = (undefined4 *)((int)puVar4 + 1);
            puVar5 = (undefined4 *)((int)puVar5 + 1);
          }
          cVar1 = network_session_send_to_machine
                            (0,&DAT_006b7f98,(uint)(DAT_006b7f98 >> 4) << 3,1,0,0,3);
          if (cVar1 != '\0') {
            local_614 = local_614 + 100;
          }
        }
      }
      puVar3 = puVar3 + 0x30;
      local_610 = local_610 + -1;
    } while (local_610 != 0);
  }
  return;
}
#endif
