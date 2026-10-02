// network_server_advance_connect_state  (Ghidra: FUN_004df290; renamed by the review pass
// from network_machine_advance_connect_state -- see the register-convention note below)
// address 0x4df290, size 72 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Advances the connection state from 1 to 2 and
// flushes any pending challenge packet built by network_prepare_challenge_packet."
// REVIEW PASS 2026-09-20: the ESI argument is the SERVER, not a machine. The binary passes it
// straight through as network_session_broadcast_to_all ECX (0x4df2cc `mov ecx,esi`), and that
// callee immediately computes `esi = ecx + 0x3b8`, which is network_server_globals::machines --
// so ECX, and therefore this function ESI, is a network_server_globals *. The state word this
// function flips is at server+0x04.
// Also: the binary pushes only SIX stack arguments to 0x4e19c0 (0x4df2bb..0x4df2ca) and passes
// the encoded size in EAX (0x4df2b8..0x4df2c7, `(packet[0] >> 4) << 3`). Ghidra renders that
// register value as a seventh stack argument; it is dropped here. See src/networking/README.md,
// open question 2, for the wider EAX-argument gap on 0x4e19c0.
// UNSURE: the state word is read/written at server+4; accessed here as a 16-bit value via cast,
// matching Ghidra own `short` view.
// register convention: server in ESI (unaff_ESI). blam-cc: ESI -> server

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

// blam-cc: ESI -> server
void network_server_advance_connect_state(network_server_globals *server)
{
    int32_t challenge_packet;
    uint32_t challenge_payload[4]; // UNSURE: the caller-frame scratch EDX points at; its
                                   //         contents are not visible in the decompilation

    if (*(int16_t *)((uint8_t *)server + 4) == 1) {
        *(int16_t *)((uint8_t *)server + 4) = 2;
        // 0x4df29b: eax = 0x19; 0x4df298: edx = the staged connect-state scratch.
        challenge_packet = (int32_t)network_prepare_challenge_packet(0x19, challenge_payload);
        if (challenge_packet != 0) {
            network_session_broadcast_to_all(server, 0, (void *)(uintptr_t)challenge_packet,
                1, 0, 1, 3);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4df290):

void FUN_004df290(void)

{
  int iVar1;
  int unaff_ESI;
  undefined4 uVar2;

  if (*(short *)(unaff_ESI + 4) == 1) {
    uVar2 = 0;
    *(undefined2 *)(unaff_ESI + 4) = 2;
    iVar1 = network_prepare_challenge_packet();
    if (iVar1 != 0) {
      FUN_004e19c0(0,iVar1,1,0,1,3,uVar2);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
